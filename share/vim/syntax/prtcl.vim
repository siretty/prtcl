
if exists("b:current_syntax")
  finish
endif

syntax keyword prtcl_keyword procedure foreach particle neighbor
syntax keyword prtcl_keyword compute reduce 

syntax keyword prtcl_keyword scheme groups global
syntax keyword prtcl_keyword select type tag
syntax keyword prtcl_keyword uniform varying real integer boolean

syntax keyword prtcl_init_keyword field

syntax match prtcl_cxx_comment "\v//.*$"
syntax region prtcl_c_comment start='/\*' end='\*/' contains=@Spell

highlight link prtcl_keyword Keyword
highlight link prtcl_init_keyword Type
highlight link prtcl_cxx_comment Comment
highlight link prtcl_c_comment Comment

let b:current_syntax = "prtcl"

