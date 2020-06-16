local prtcl = require 'prtcl'
local ttype = prtcl.data.ttype

print(prtcl.data.ctype.new(), prtcl.data.ctype.new("b"))
print(prtcl.data.shape.new {}, prtcl.data.shape.new { 1, 2 })

local the_ttype = prtcl.data.ttype.new("f32", { 2, 3 })
print(the_ttype)

local model = prtcl.data.model.new()
print("group count: " .. model.group_count)

local a = model:add_group("a", "type")
a:add_tag("first_tag")
a:add_tag("second_tag")
for _, tag in ipairs(a.tags) do print(tag) end
a:remove_tag("first_tag")
for _, tag in ipairs(a.tags) do print(tag) end
if a:has_tag("second_tag") then print("yup, second_tag is here") end

a:add_varying_field("position", ttype.new("f32", { 3 }))
a:add_uniform_field("u", the_ttype)
a:resize(23)

local b = model:add_group("b", "type")
print("group count: " .. model.group_count)

print(a.group_name, a.group_type, a.group_index, a.uniform.field_count, a.varying.field_count, a.item_count)
local x = a.varying:get_field("position"):get_access()
x:set_component(1, { 1 }, 123)
x:set_component(1, { 2 }, 321)
print("x", x.type, x.size, x:get_component(1, { 1 }))

local item = x:get_item(1)
item[2] = 456;
x:set_item(1, item)
print("x", x.type, x.size, x:get_component(1, { 2 }))
print(type(item), #item)

print(b.group_name, b.group_type, b.group_index, b.uniform.field_count, b.varying.field_count, b.item_count)


print("saving")
a:save_vtk("test_a.vtk")


local nhood = prtcl.util.neighborhood.new()
nhood:load(model)
nhood:update()
nhood:permute(model)


local schedule = prtcl.util.virtual_scheduler.new()
schedule:schedule_after(1, function(s, delay)
    print("scheduled", s:get_clock():now(), delay)
    return s:reschedule_after(1)
end)

for _ = 1, 20 do
    schedule:get_clock():advance(0.26)
    schedule:tick()
end
