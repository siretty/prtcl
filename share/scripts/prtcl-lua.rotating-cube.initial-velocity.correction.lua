local prtcl = require 'prtcl'
local ttype = prtcl.data.ttype
local rvec, rmat = prtcl.math.rvec, prtcl.math.rmat


--local unitcube = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')
--
--local floor = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitslab.obj')
--floor:scale(rvec.new { 1, 0, 1 })
--
--local wall = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitslab.obj')
--wall:scale(rvec.new { 2, 0, 0.2 })
--wall:rotate(math.pi / 2, rvec.new { 0, 0, 1 })
--wall:translate(rvec.new { 0, 2.025, 0 })
--
--local fluidsphere = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitsphere.obj')
--fluidsphere:scale(rvec.new { .25, .25, .25 })
--fluidsphere:translate(rvec.new { .5, .25, .5 })

local fluidcube = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')
fluidcube:scale(rvec.new { .4, .4, .4 })
fluidcube:translate(rvec.new { -.2, -.2, -.2 })


for _, scheme in ipairs(prtcl.schemes.get_scheme_names()) do
  print('SCHEME', scheme)
end


local function make_scheme(name)
  return prtcl.schemes.make('prtcl::schemes::' .. name .. '[T=f32, N=3, K=Cubic Spline]')
end

local schemes = {
  -- fluid handling
  density = make_scheme('density'),
  gravity = make_scheme('gravity'),
  iisph = make_scheme('iisph'),
  advect = make_scheme('symplectic_euler'),
  correction = make_scheme('correction'),

  -- various viscosity implementations
  --viscosity = make_scheme('viscosity'),
  implicit_viscosity = make_scheme('wkbb18_gc'),
  --implicit_viscosity = make_scheme('pt16'),
}

for name, scheme in pairs(schemes) do
  local f = io.open('output/' .. name .. '.prtcl', 'w')
  f:write(scheme:get_prtcl_source_code())
  io.close(f)
end

-- load all schemes, this will create all global / uniform / varying fields
local function load_schemes(model)
  for _, scheme in pairs(schemes) do
    scheme:load(model)
  end
end


local frames_per_second = 24
local seconds_per_frame = 1 / frames_per_second


local function readable_path(path)
  file = io.open(path, 'r')
  if file ~= nil then
    io.close(file)
    return true
  end
  return false
end

--local model = nil
--local model_path = 'output/model.6.bin'
--if readable_path(model_path) then
--  model = prtcl.data.model.load_native_binary(model_path)
--else
--  model = prtcl.data.model.new()
--end


model = prtcl.data.model.new()

local f = model:add_group("f", "fluid")
f:add_tag("dynamic")
f:add_tag("visible")


load_schemes(model)


cfg = {}


model.global:get_field("smoothing_scale"):set(0.025)
model.global:get_field("time_step"):set(0.001) -- :set(0.002)
model.global:get_field("fade_duration"):set(2 * seconds_per_frame)

model.global:get_field("gravity"):set(rvec.new { 0, 0, 0 })

if model.global:has_field("iisph_relaxation") then
  model.global:get_field("iisph_relaxation"):set(0.5)
end

f.uniform:get_field("rest_density"):set(1000)

if f.uniform:has_field("surface_tension") then
  f.uniform:get_field("surface_tension"):set(1)
end

-- for standard and wkbb18 viscosity
if f.uniform:has_field("dynamic_viscosity") then
  f.uniform:get_field("dynamic_viscosity"):set(1000000)
  --f.uniform:get_field("dynamic_viscosity"):set(100000)
  --f.uniform:get_field("dynamic_viscosity"):set(1000)
  --f.uniform:get_field("dynamic_viscosity"):set(10)
  --f.uniform:get_field("dynamic_viscosity"):set(0)
end

cfg.wkbb18 = { used = false }
if f.uniform:has_field("wkbb18_maximum_error") then
  cfg.wkbb18.used = true
  f.uniform:get_field("wkbb18_maximum_error"):set(0.0001)
  f.uniform:get_field("wkbb18_maximum_iterations"):set(100)
end

cfg.pt16 = { used = false }
if f.uniform:has_field("pt16_maximum_error") then
  cfg.pt16.used = true
  cfg.pt16.vorticity_diffusion_maximum_error = .001
  cfg.pt16.vorticity_diffusion_maximum_iterations = 500
  cfg.pt16.velocity_reconstruction_maximum_error = .001
  cfg.pt16.velocity_reconstruction_maximum_iterations = 500
  --[[ trying   - ]] f.uniform:get_field("strain_rate_viscosity"):set(0)
end


local nhood = prtcl.util.neighborhood.new()
nhood:load(model)
nhood:set_radius(2 * model.global:get_field("smoothing_scale"):get())
nhood:update()
nhood:permute(model)


local function save_frame(frame)
  f:save_vtk('output/f.' .. frame .. '.vtk')
  model:save_native_binary('output/model.' .. frame .. '.bin')
end


local schedule = prtcl.util.virtual_scheduler.new()


local current_frame = 0

-- schedule the frame handler
schedule:schedule_at(0, function(s, delay)
  current_frame = current_frame + 1 -- advance the frame counter first

  print('FRAME #' .. current_frame .. ' (DELAYED ' .. tostring(delay) .. ')')
  save_frame(current_frame)

  return s:reschedule_at(current_frame * seconds_per_frame)
end)


local function setup_fluid()
  fluidcube:sample_volume(f)

  -- compute fluid particle mass
  local h = model.global:get_field('smoothing_scale'):get()
  local rho0 = f.uniform:get_field('rest_density'):get() * 0.8
  local m = f.varying:get_field('mass')
  local x = f.varying:get_field('position')

  local x_avg = rvec.new{0, 0, 0}
  local m0 = h * h * h * rho0
  for i = 0, f.item_count - 1 do
    m:set(i, m0)
    x_avg = x_avg + x:get(i)
  end

  x_avg = (1 / f.item_count) * x_avg

  for i = 0, f.item_count - 1 do
    x:set(i, x:get(i) - x_avg)
  end
end

if f.item_count == 0 then
  setup_fluid()
end


for _, field_name in ipairs(model.global:field_names()) do
  local field = model.global:get_field(field_name)
  print('GLOBAL', field_name, field:get())
end

for _, group_name in ipairs(model:group_names()) do
  local group = model:get_group(group_name)
  print('GROUP', group_name)
  for _, field_name in ipairs(group.uniform:field_names()) do
    local field = group.uniform:get_field(field_name)
    print('GROUP', group_name, 'UNIFORM', field_name, field:get())
  end
  for _, field_name in ipairs(group.varying:field_names()) do
    print('GROUP', group_name, 'VARYING', field_name)
  end
end


load_schemes(model)

save_frame(0)


-- set initial velocity
local axis = rvec.new { 1, 0, 0 }
local x = f.varying:get_field('position')
local v = f.varying:get_field('velocity')
for i = 0, f.item_count - 1 do
  local position = x:get(i)
  local velocity = 5 * axis:cross(position)
  v:set(i, velocity)
end


local current_step, steps_since_permute = 0, 0
while schedule.clock.seconds <= 10 do
  current_step = current_step + 1
  steps_since_permute = steps_since_permute + 1

  local t = model.global:get_field("current_time")
  t:set(schedule.clock.seconds)

  if model.dirty then
    nhood:load(model)
    nhood:update(model)
    load_schemes(model)
    model.dirty = false
  else
    nhood:update(model)
    if steps_since_permute >= 8 then
      nhood:permute(model)
      model.dirty = false
      steps_since_permute = 0
    end
  end

  schemes.density:run_procedure('compute_density', nhood)

  schemes.correction:run_procedure('identity_gradient_correction', nhood)
  --schemes.correction:run_procedure('compute_gradient_correction', nhood)

  schemes.gravity:run_procedure('initialize_acceleration', nhood)

  if schemes.viscosity ~= nil then
    schemes.viscosity:run_procedure('accumulate_acceleration', nhood)
  end

  schemes.advect:run_procedure('integrate_velocity_with_hard_fade', nhood)

  local nprde = model.global:get_field('iisph_nprde')
  local aprde = model.global:get_field('iisph_aprde')

  schemes.iisph:run_procedure('setup', nhood)

  if nprde:get() >= 1 then
    local relative_aprde, max_pressure_iteration = 0, 0
    for pressure_iteration = 1, 50 do
      max_pressure_iteration = pressure_iteration

      aprde:set(0)
      schemes.iisph:run_procedure('iteration_pressure_acceleration', nhood)
      schemes.iisph:run_procedure('iteration_pressure', nhood)

      relative_aprde = aprde:get() / nprde:get()
      if pressure_iteration >= 2 and relative_aprde < 0.001 then break end
    end

    print('  IISPH #' .. tostring(max_pressure_iteration), relative_aprde)
  end

  schemes.advect:run_procedure('integrate_velocity_with_hard_fade', nhood)

  if cfg.wkbb18.used then
    schemes.implicit_viscosity:run_procedure('compute_diagonal', nhood)
    schemes.implicit_viscosity:run_procedure('accumulate_acceleration', nhood)
    print('  WKBB18 #' .. tostring(f.uniform:get_field('wkbb18_iterations'):get()))

    schemes.advect:run_procedure('integrate_velocity_with_hard_fade', nhood)
  end

  if cfg.pt16.used then
    schemes.implicit_viscosity:run_procedure('setup', nhood)
    f.uniform:get_field("pt16_maximum_error"):set(cfg.pt16.vorticity_diffusion_maximum_error)
    f.uniform:get_field("pt16_maximum_iterations"):set(cfg.pt16.vorticity_diffusion_maximum_iterations)
    schemes.implicit_viscosity:run_procedure('solve_vorticity_diffusion', nhood)
    print('  PT16 VORTICITY #' .. tostring(f.uniform:get_field('pt16_iterations'):get()))
    --schemes.implicit_viscosity:run_procedure('vorticity_preservation', nhood)
    f.uniform:get_field("pt16_maximum_error"):set(cfg.pt16.velocity_reconstruction_maximum_error)
    f.uniform:get_field("pt16_maximum_iterations"):set(cfg.pt16.velocity_reconstruction_maximum_iterations)
    schemes.implicit_viscosity:run_procedure('solve_velocity_reconstruction', nhood)
    print('  PT16 VELOCITY #' .. tostring(f.uniform:get_field('pt16_iterations'):get()))
  end

  schemes.advect:run_procedure('integrate_position', nhood)

  --[[
  local axis = rvec.new{1, 0, 0}
  local x = f.varying:get_field('position')
  local v = f.varying:get_field('velocity')
  for i = 0, f.item_count - 1 do
    local position = x:get(i)
    local velocity = axis:cross(position)
    v:set(i, velocity)
  end
  --]]

  schedule:get_clock():advance(model.global:get_field("time_step"):get())
  schedule:tick()
end
