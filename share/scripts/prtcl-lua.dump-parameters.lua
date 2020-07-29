local prtcl = require 'prtcl'
local rvec, rmat = prtcl.math.rvec, prtcl.math.rmat

local model_path = args[3]
local ppm_path = model_path .. '.ppm'

local model = prtcl.data.model.load_native_binary(model_path)

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

