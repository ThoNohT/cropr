#ifndef _PARSER_H
#define _PARSER_H

#include "common.h"
#include "lexer.h"

typedef struct {} PoundPreProc;
typedef struct {} FunctionDefinition;
typedef struct {} FunctionImplementation;
typedef struct {} CExpression;

typedef enum {
    ST_PoundPreProc,
    ST_FunctionDefinition,
    ST_FunctionImplementation,
    ST_CExpression,
} StatementType;

typedef struct {
    StatementType type;
    union {
        PoundPreProc *preproc;
        FunctionDefinition *fundef;
        FunctionImplementation *funimpl;
        CExpression *cexpr;
    };
} Statement;


typedef struct {
    Statement *elems;
    size_t count;
    size_t capacity;
} Program;


// Parses the tokens lexed from a file into a syntax tree.
void parse_file(Tokens *tokens, Errors *errors);

#endif // _PARSER_H
