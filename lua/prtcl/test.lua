
local src_to_pst = require "prtcl.src_to_pst"
local pst_to_ast = require "prtcl.pst_to_ast"

files = {'aat13.prtcl', 'aiast12.prtcl', 'correction.prtcl', 'density.prtcl', 'gravity.prtcl', 'he14.prtcl', 'iisph.prtcl', 'pt16.prtcl', 'sesph.prtcl', 'symplectic_euler.prtcl', 'viscosity.prtcl', 'wkbb18.prtcl'}

for _, file in ipairs(files) do
  file = '../share/schemes/' .. file
  print('loading "' .. file .. '"')

  local f = io.open(file, 'r')

  src = f:read('*a')

  pst = src_to_pst(src)
  if pst == nil then print('error in src_to_pst', file) end

  ast = pst_to_ast(pst)
  if ast == nil then print('error in pst_to_ast', file) end

  local cxx_omp = require "prtcl.generate.cxx_omp"

  local printer = require("prtcl.printer.file"):new()
  cxx_omp.generate_hpp(ast, printer, {})
end

