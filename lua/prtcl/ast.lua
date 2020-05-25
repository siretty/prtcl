local ast = {}

ast.scheme = require "prtcl.ast.scheme"

ast.groups = require "prtcl.ast.groups"
ast.global = require "prtcl.ast.global"

ast.group_selector = require "prtcl.ast.group_selector"
ast.field_def = require "prtcl.ast.field_def"

ast.procedure = require "prtcl.ast.procedure"
ast.foreach_particle = require "prtcl.ast.foreach_particle"
ast.foreach_neighbor = require "prtcl.ast.foreach_neighbor"

ast.solve_pcg = require "prtcl.ast.solve_pcg"
ast.solve_setup = require "prtcl.ast.solve_setup"
ast.solve_product = require "prtcl.ast.solve_product"
ast.solve_apply = require "prtcl.ast.solve_apply"

ast.uop = require "prtcl.ast.uop"
ast.bop = require "prtcl.ast.bop"

ast.unprocessed = require "prtcl.ast.unprocessed"

return ast
