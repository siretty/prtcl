local prtcl = require 'prtcl'
local log = prtcl.log

local model = prtcl.data.model.new()
print("group count: " .. model.group_count)

local a = model:add_group("a", "type")
local b = model:add_group("b", "type")
print("group count: " .. model.group_count)

print(a.group_name, a.group_type, a.group_index, a.item_count)
print(a.varying.field_count);
local x = a.varying:get_field("position")
print(x)

print(b.group_name, b.group_type, b.group_index, b.item_count)
