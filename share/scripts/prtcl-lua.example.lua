local prtcl = require 'prtcl'
local ttype = prtcl.data.ttype
local rvec, rmat = prtcl.math.rvec, prtcl.math.rmat


local unitcube = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')

local floor = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitslab.obj')
floor:scale(rvec.new { 1, 0, 1 })

local wall = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitslab.obj')
wall:scale(rvec.new { 2, 0, 0.2 })
wall:rotate(math.pi / 2, rvec.new { 0, 0, 1 })
wall:translate(rvec.new { 0, 2.025, 0 })

local fluidsphere = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitsphere.obj')
fluidsphere:scale(rvec.new { .25, .25, .25 })
fluidsphere:translate(rvec.new { .5, .25, .5 })

local fluidcube = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')
--fluidcube:rotate(1.0, rvec.new{1, 0, 0})
fluidcube:scale(rvec.new { .4, .4, .4 })
fluidcube:translate(rvec.new { -.2, -.2, -.2 })
--fluidcube:translate(rvec.new { .3, .4, .3 })

--[[ buckling
local scene = 'buckling'
local fluidcolumn = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')
fluidcolumn:scale(rvec.new { 0.3, 4, 0.05 })
fluidcolumn:translate(rvec.new { 0, 0.025, 0 })
-- ]]

--[[ coiling
local scene = 'coiling'
local fluidcolumn = prtcl.geometry.triangle_mesh.from_obj_file('share/models/unitcube.obj')
fluidcolumn:scale(rvec.new { 0.05, 6, 0.05 })
fluidcolumn:translate(rvec.new { 0, 0.025, 0 })
-- ]]


for _, scheme in ipairs(prtcl.schemes.get_scheme_names()) do
  print('SCHEME', scheme)
end


local function make_scheme(name)
  return prtcl.schemes.make('prtcl::schemes::' .. name .. '[T=f32, N=3, K=Cubic Spline]')
end

local schemes = {
  -- boundary handling
  boundary = make_scheme('aiast12'),

  -- fluid handling
  density = make_scheme('density'),
  gravity = make_scheme('gravity'),
  iisph = make_scheme('iisph'),
  --surface_tension = make_scheme('aat13'),
  advect = make_scheme('symplectic_euler'),

  -- various viscosity implementations
  --viscosity = make_scheme('viscosity'),
  --implicit_viscosity = make_scheme('wkbb18'),
  implicit_viscosity = make_scheme('pt16'),

  -- rendering via particles
  horas = make_scheme('horas'),
}

-- load all schemes, this will create all global / uniform / varying fields
local function load_schemes(model)
  for _, scheme in pairs(schemes) do
    scheme:load(model)
  end
end


local frames_per_second = 24
local seconds_per_frame = 1 / frames_per_second


local model = prtcl.data.model.new()

local f = model:add_group("f", "fluid")
f:add_tag("dynamic")
f:add_tag("visible")

local b = model:add_group("b", "boundary")

local horasons = model:add_group("horasons", "horason")
horasons:add_tag("cannot_be_neighbor")


load_schemes(model)


cfg = {}


model.global:get_field("smoothing_scale"):set(0.025)
model.global:get_field("time_step"):set(0.001) -- :set(0.002)
model.global:get_field("fade_duration"):set(2 * seconds_per_frame)

model.global:get_field("gravity"):set(rvec.new { 0, 0, 0 })
--model.global:get_field("gravity"):set(rvec.new { 0, -9.81, 0.1 })
--model.global:get_field("gravity"):set(rvec.new { -1, -9.81, 0 })
--model.global:get_field("gravity"):set(rvec.new { -1, -9.81, -1 })

if model.global:has_field("iisph_relaxation") then
  model.global:get_field("iisph_relaxation"):set(0.5)
end

f.uniform:get_field("rest_density"):set(1000)

if f.uniform:has_field("surface_tension") then
  f.uniform:get_field("surface_tension"):set(1)
end

-- for sesph
if f.uniform:has_field("compressibility") then
  f.uniform:get_field("compressibility"):set(10e6)
end

-- for standard and wkbb18 viscosity
if f.uniform:has_field("dynamic_viscosity") then
  f.uniform:get_field("dynamic_viscosity"):set(1000)
  --f.uniform:get_field("dynamic_viscosity"):set(10)
  --f.uniform:get_field("dynamic_viscosity"):set(0)
end

-- for standard and wkbb18 viscosity
cfg.wkbb18 = { used = false }
if f.uniform:has_field("wkbb18_maximum_error") then
  cfg.wkbb18.used = true
  f.uniform:get_field("wkbb18_maximum_error"):set(0.05)
  f.uniform:get_field("wkbb18_maximum_iterations"):set(100)
end

cfg.pt16 = { used = false }
if f.uniform:has_field("pt16_maximum_error") then
  cfg.pt16.used = true
  cfg.pt16.vorticity_diffusion_maximum_error = .001
  cfg.pt16.vorticity_diffusion_maximum_iterations = 500
  cfg.pt16.velocity_reconstruction_maximum_error = .001
  cfg.pt16.velocity_reconstruction_maximum_iterations = 500
  -- [[ buckling -- ]] f.uniform:get_field("strain_rate_viscosity"):set(.99)
  -- [[ coiling  -- ]] f.uniform:get_field("strain_rate_viscosity"):set(.85)
  --[[ trying   - ]] f.uniform:get_field("strain_rate_viscosity"):set(0)
end


-- for aat13 surface tension
if b.uniform:has_field("adhesion") then
  b.uniform:get_field("adhesion"):set(0)
end

-- for standard and wkbb18 viscosity
if b.uniform:has_field("dynamic_viscosity") then
  --b.uniform:get_field("dynamic_viscosity"):set(100)
  b.uniform:get_field("dynamic_viscosity"):set(10)
end


--unitcube:sample_surface(b)
--floor:sample_surface(b)
--wall:translate(rvec.new { -0.05, 0, 0 })
--wall:sample_surface(b)
--wall:translate(rvec.new { 0.05 + 0.325, 0, 0 })
--wall:sample_surface(b)

local camera = prtcl.geometry.pinhole_camera.new()
camera.sensor_width, camera.sensor_height = 800, 600
camera.focal_length = 1
camera.origin = rvec.new { -1, 0.4, 0.5 }
camera.principal = rvec.new { 1, 0, 0 }
camera.up = rvec.new { 0, 1, 0 }


local nhood = prtcl.util.neighborhood.new()
nhood:load(model)
nhood:set_radius(2 * model.global:get_field("smoothing_scale"):get())
nhood:update()
--nhood:permute(model)


local function save_frame(frame)
  f:save_vtk('output/f.' .. frame .. '.vtk')
end


local schedule = prtcl.util.virtual_scheduler.new()


local current_frame = 0

-- schedule the frame handler
schedule:schedule_at(0, function(s, delay)
  current_frame = current_frame + 1 -- advance the frame counter first

  print('FRAME #' .. current_frame .. ' (DELAYED ' .. tostring(delay) .. ')')
  save_frame(current_frame)

  schemes.horas:run_procedure("update_visible_aabb", nhood)

  horasons:resize(0)
  camera:sample(horasons)
  schemes.horas:load(model)

  schemes.horas:run_procedure("reset", nhood)
  for step = 1, 100 do
    schemes.horas:run_procedure("step_fluid", nhood)
  end
  horasons:save_vtk('output/c.' .. (current_frame - 1) .. '.vtk')

  return s:reschedule_at(current_frame * seconds_per_frame)
end)


--[[ test source
local radius = 3 * model.global:get_field("smoothing_scale"):get()
local center = rvec.new { 0.1, 0.1, 0.1 }
local velocity = 3 * rvec.ones(3)
local source = prtcl.util.hcp_lattice_source.new(model, f, radius, center, velocity, 1000)
print("spawn interval: ", source.regular_spawn_interval)

schedule:schedule_after(seconds_per_frame, source)
--]]

--[[ coiling source
local radius = 1.2 * model.global:get_field("smoothing_scale"):get()
local center = rvec.new { 0.1, 1, 0.1 }
--local velocity = 1.0 * rvec.new { 0.8, 0.2, 0 }
local velocity = 1.0 * rvec.new { 0.2, -0.5, 0 }
local source = prtcl.util.hcp_lattice_source.new(model, f, radius, center, velocity, 10000)
print("spawn interval: ", source.regular_spawn_interval)

schedule:schedule_after(seconds_per_frame, source)
--]]

local function setup_fluid()
  -- [[
  fluidcube:sample_volume(f)
  --f:rotate(2 * math.pi / 6, rvec.new { 1, 0, 0 })
  --f:translate(rvec.new { .3, .4, .3 })
  --]]

  --[[ buckling
  fluidcolumn:sample_volume(f)
  f:scale(rvec.new { 1, 1, 1 })
  f:rotate(2 * math.pi * 0 / 360, rvec.new { 1, 0, 0 })
  --]]

  --[[ coiling
  fluidcolumn:sample_volume(f)
  f:scale(rvec.new { 0.9, 0.9, 0.9 })
  f:rotate(2 * math.pi * 2 / 360, rvec.new { 2, 0, 1 })
  --]]

  local h = model.global:get_field('smoothing_scale'):get()
  local rho0 = f.uniform:get_field('rest_density'):get()
  local m = f.varying:get_field('mass')
  for i = 0, f.item_count - 1 do
    m:set(i, h * h * h * rho0)
  end
end

setup_fluid()


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

schemes.boundary:run_procedure('compute_volume', nhood)

b:save_vtk('output/b.vtk')
save_frame(0)

--local g_base = model.global:get_field('gravity'):get()
--local g_angle, g_angle_vel = 0, 0.5 * 2 * math.pi


local axis = rvec.new { 1, 0, 0 }
local x = f.varying:get_field('position')
local v = f.varying:get_field('velocity')
for i = 0, f.item_count - 1 do
  local position = x:get(i)
  local velocity = axis:cross(position)
  v:set(i, velocity)
end


local current_step, steps_since_permute = 0, 0
while schedule.clock.seconds <= 10 do
  current_step = current_step + 1
  steps_since_permute = steps_since_permute + 1

  local t = model.global:get_field("current_time")
  t:set(schedule.clock.seconds)

  --g_angle = g_angle_vel * t:get()
  --model.global:get_field('gravity'):set(g_base + rvec.new { math.cos(g_angle), 0, math.sin(g_angle) })

  if model.dirty then
    nhood:load(model)
    nhood:update(model)
    load_schemes(model)
    model.dirty = false
  else
    nhood:update(model)
    if steps_since_permute >= 8 then
      nhood:permute(model)
      steps_since_permute = 0
    end
  end

  schemes.density:run_procedure('compute_density', nhood)

  schemes.gravity:run_procedure('initialize_acceleration', nhood)

  if schemes.surface_tension ~= nil then
    schemes.surface_tension:run_procedure('compute_particle_normal', nhood)
    schemes.surface_tension:run_procedure('accumulate_acceleration', nhood)
  end

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
