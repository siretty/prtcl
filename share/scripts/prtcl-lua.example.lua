local prtcl = require 'prtcl'
local ttype = prtcl.data.ttype
local rvec, rmat = prtcl.math.rvec, prtcl.math.rmat

print(prtcl.data.ctype.new(), prtcl.data.ctype.new("b"))
print(prtcl.data.shape.new {}, prtcl.data.shape.new { 1, 2 })

local the_ttype = prtcl.data.ttype.new("f32", { 2, 3 })
print(the_ttype)

local model = prtcl.data.model.new()
print("group count: " .. model.group_count)

model:add_global_field("smoothing_scale", ttype.new("f32", {}))


local a = model:add_group("a", "type")
a:add_tag("first_tag")
a:add_tag("second_tag")
for _, tag in ipairs(a.tags) do print(tag) end
a:remove_tag("first_tag")
for _, tag in ipairs(a.tags) do print(tag) end
if a:has_tag("second_tag") then print("yup, second_tag is here") end

local rho0 = a:add_uniform_field("rest_density", ttype.new("f32", {}))
local x = a:add_varying_field("position", ttype.new("f32", { 3 }))
local v = a:add_varying_field("velocity", ttype.new("f32", { 3 }))
local m = a:add_varying_field("mass", ttype.new("f32", {}))
a:add_varying_field("time_of_birth", ttype.new("f32", {}))

a:resize(30)

x:set(0, rvec.unit(3, 0))
x:set(1, rvec.unit(3, 1))
x:set(2, rvec.unit(3, 2))

local x2 = a.varying:get_field("position")
print(x2:get(0), x2:get(1), x2:get(2))

rho0:set(1000)
print(rho0:get())

local u = a:add_uniform_field("u", ttype.new("f32", { 3 }))
u:set(rvec.new { 1, 2, 3 })
print("u", u:get())

a:resize(23)
print("resized")

local b = model:add_group("b", "type")
print("group count: " .. model.group_count)


local v_a = 6 * rvec.new { 1, 2, 3 } * 4 + rvec.new { 4, 5, 6 } / 2
print(v_a)

print(rvec.unit(3), rvec.unit(3, 0), rvec.unit(3, 1), rvec.unit(3, 2))

local v_m = rmat.new({ { 1, 2, 3 }, { 4, 5, 6 } }) * rmat.identity(3, 3)
print(v_m)


local nhood = prtcl.util.neighborhood.new()
nhood:load(model)
nhood:update()
nhood:permute(model)


local center = rvec.zeros(3)
local velocity = rvec.ones(3)
local source = prtcl.util.hcp_lattice_source.new(model, a, 1, center, velocity, 1000)
print("spawn interval: ", source.regular_spawn_interval)


local schedule = prtcl.util.virtual_scheduler.new()

schedule:schedule_after(1, function(s, delay)
    print("scheduled", s:get_clock():now(), delay)
    return s:reschedule_after(1)
end)

schedule:schedule_after(1, source)


a:save_vtk("output/test_a.0.vtk")

for frame = 1, 24 * 20 do
    schedule:get_clock():advance(1 / 24)
    schedule:tick()

    a:save_vtk("output/test_a." .. frame .. ".vtk")
end
