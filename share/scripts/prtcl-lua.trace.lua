local prtcl = require 'prtcl'
local rvec, rmat = prtcl.math.rvec, prtcl.math.rmat

local model_path = args[3]
local ppm_path = model_path .. '.ppm'

local model = prtcl.data.model.load_native_binary(model_path)

--[[ camera settings for the rotating cube scene
local camera = prtcl.geometry.pinhole_camera.new()
camera.sensor_width, camera.sensor_height = 800, 600
camera.focal_length = 1
camera.origin = rvec.new { -1.3, 0, -1.3 }
camera.principal = rvec.new { 1, 0, 1 }
camera.principal = camera.principal / camera.principal:norm()
camera.up = rvec.new { 0, 1, 0 }
--]]

-- [[ camera settings for the rotating cube scene
local camera = prtcl.geometry.pinhole_camera.new()
camera.sensor_width, camera.sensor_height = 400, 300 -- 800, 600
camera.focal_length = 1
camera.origin = rvec.new { -2, 0.7, 2 }
camera.principal = rvec.new { 1, 0, -1 }
camera.principal = camera.principal / camera.principal:norm()
camera.up = rvec.new { 0, 1, 0 }
--]]

local tracer = prtcl.util.sphere_tracer.new(camera)
tracer.threshold = 0.95
local image = tracer:trace(model)
print('IMAGE', image.width, image.height)

local ppm = io.open(ppm_path, 'wb')
ppm:write("P6\n")
ppm:write(tostring(image.width) .. "\n")
ppm:write(tostring(image.height) .. "\n")
ppm:write("255\n")
for iy = 0, image.height - 1, 1 do
  for ix = 0, image.width - 1, 1 do
    local gray = math.floor(image:get_pixel(ix, iy) * 255)
    ppm:write(string.char(gray, gray, gray))
    --ppm:write(string.char(ix % 256))
    --ppm:write(string.char(ix % 256))
    --ppm:write(string.char(ix % 256))
  end
end
io.close(ppm)

