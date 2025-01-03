" Vim syntax file
" Language: hex
" Maintainer: Fabio Cevasco
" Last Change: 2024-12-10
" Version: 0.3.0

if exists("b:current_syntax")
  finish
endif

syntax keyword          hexNativeSymbol         if while error symbols throw try dup pop swap stack and or not xor int str hex dec type 
syntax keyword          hexNativeSymbol         cat chr len get ord index join split replace debug map puts warn print gets 
syntax keyword          hexNativeSymbol         read write append args exit exec run
syntax match            hexNativeSymbol         /\v\!/
syntax match            hexNativeSymbol         /\v\!\=/ 
syntax match            hexNativeSymbol         /\v\%/
syntax match            hexNativeSymbol         /\v\&\&/
syntax match            hexNativeSymbol         /\v\./
syntax match            hexNativeSymbol         /\v\'/
syntax match            hexNativeSymbol         /\v\+/
syntax match            hexNativeSymbol         /\v\-/
syntax match            hexNativeSymbol         /\v\*/
syntax match            hexNativeSymbol         /\v\//
syntax match            hexNativeSymbol         /\v\::/
syntax match            hexNativeSymbol         /\v\:/
syntax match            hexNativeSymbol         /\v\</
syntax match            hexNativeSymbol         /\v\>/
syntax match            hexNativeSymbol         /\v\<\=/
syntax match            hexNativeSymbol         /\v\=\=/
syntax match            hexNativeSymbol         /\v\>\=/
syntax match            hexNativeSymbol         /\v\>\>/
syntax match            hexNativeSymbol         /\v\<\</
syntax match            hexNativeSymbol         /\v\^/
syntax match            hexNativeSymbol         /\v\~/
syntax match            hexNativeSymbol         /\v\|\|/
syntax match            hexNativeSymbol         /\v\|/
syntax match            hexNativeSymbol         /\v\#/
syntax match            hexNativeSymbol         /\v\&/

syntax match            hexUserSymbol         ;[_a-zA-Z_][a-zA-Z0-9_-]*; 

syntax keyword          hexCommentTodo        TODO FIXME XXX TBD contained
syntax match            hexComment            /;.*$/ contains=hexCommentTodo
syntax region           hexComment            start=;#|; end=;|#; contains=hexCommentTodo

syntax match            hexNumber             ;0x[0-9a-f]\{1,8\};
syntax region           hexString             start=+"+ skip=+\\\\\|\\$"+  end=+"+  


syntax match            hexParen              ;(\|); 

syntax match            hexSharpBang          /\%^#!.*/


" Highlighting
hi default link         hexComment            Comment
hi default link         hexCommentTodo        Todo
hi default link         hexString             String
hi default link         hexNumber             Number
hi default link         hexNativeSymbol       Statement
hi default link         hexUserSymbol         Identifier
hi default link         hexParen              Special
hi default link         hexSharpBang          Preproc

let b:current_syntax = "hex"
