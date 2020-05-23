local object = require "prtcl.object"
local block = require "prtcl.ast.block"

local groups = object:make_class(block)

function groups:_init(kwargs)
  if kwargs == nil then kwargs = {} end
  object:init(groups, self, kwargs)
  self.name = kwargs.name
  self.select_expr = kwargs.select_expr
end

function groups:replace(child, with)
  local base = object:class_base(groups)
  base.replace(self, child, with)
  if self.select_expr == child then
    self.select_expr = with
  end
end

return groups
