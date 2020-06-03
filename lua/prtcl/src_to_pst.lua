local grammar = require "prtcl.parser.grammar"

return function(source)
  return grammar.parse(source)
end
