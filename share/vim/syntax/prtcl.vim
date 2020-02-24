
if exists("b:current_syntax")
  finish
endif

syntax keyword prtcl_keyword let procedure compute reduce foreach particle neighbor

syntax keyword prtcl_init_keyword field
syntax keyword prtcl_keyword global uniform varying real integer boolean

syntax keyword prtcl_init_keyword particle_selector
syntax keyword prtcl_keyword types tags

syntax match prtcl_c_comment "\v//.*$"

highlight link prtcl_keyword Keyword
highlight link prtcl_init_keyword Type
highlight link prtcl_c_comment Comment

let b:current_syntax = "prtcl"

