#ifndef _PARSER_H
#define _PARSER_H

#include "common.h"
#include "lexer.h"

// Parses the tokens lexed from a file into a syntax tree.
void parse_file(Tokens *tokens, Errors *errors);

#endif // _PARSER_H
