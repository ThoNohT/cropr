" Vim syntax file
" Language: Cropr

" Usage Instructions
" Put this file in .vim/syntax/cropr.vim
" and add in your .vimrc file the next line:
" autocmd BufRead,BufNewFile *.cr set filetype=cropr

if exists("b:current_syntax")
  finish
endif

set iskeyword=a-z,A-Z,-,*,_,!,@,:,>,(,),#
syntax keyword croprTodos TODO

" Language keywords
syntax keyword croprKeywords break case const continue do else enum false for goto if inline register restrict return signed sizeof static struct switch true typedef unsigned volatile while struct union

syntax match croprPound display "#\w\+"
	
" Comments
syntax region croprCommentLine start="//" end="$"   contains=croprTodos
syntax region croprCommentBlock start="/\*" end="\*/"   contains=croprTodos

" String literals
syntax region croprString start=/\v"/ skip=/\v\\./ end=/\v"/ contains=croprEscapes
syntax region croprString2 start=/\v\</ skip=/\v\\./ end=/\v\>/ contains=croprEscapes

" Char literals
syntax region croprChar start=/\v'/ skip=/\v\\./ end=/\v'/ contains=croprEscapes

" Escape literals \n, \r, ....
syntax match croprEscapes display contained "\\[nr\"']"

" Number literals
syntax region croprNumber start=/\s\d/ skip=/\d/ end=/\s/

syntax keyword cropSpecialChars () :: ->

" Type names the compiler recognizes
syntax keyword croprTypeNames auto bool char float int long short signed void struct 

" Set highlights
highlight default link croprTodos Todo
highlight default link croprKeywords Keyword
highlight default link croprPound Keyword
highlight default link croprCommentLine Comment
highlight default link croprCommentBlock Comment
highlight default link croprString String
highlight default link croprString2 String
highlight default link croprNumber Number
highlight default link croprTypeNames Type
highlight default link croprChar Character
highlight default link croprEscapes SpecialChar
highlight default link cropSpecialChars SpecialChar

let b:current_syntax = "cropr"
