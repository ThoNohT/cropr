#ifndef _PARSER_H
#define _PARSER_H

#include "common.h"
#include "lexer.h"

// Preprocessor expression.
typedef struct {
    Location loc;
    Token *tokens;
    size_t token_count;
} PreProc;

typedef struct {} FunctionDefinition;
typedef struct {} FunctionImplementation;
typedef struct {} Expression;

typedef enum {
    ST_PreProc,
    ST_FunctionDefinition,
    ST_FunctionImplementation,
    ST_Expression,
} StatementType;

typedef struct {
    StatementType type;
    union {
        PreProc *preproc;
        FunctionDefinition *fundef;
        FunctionImplementation *funimpl;
        Expression *expr;
    };
} Statement;


typedef struct {
    Statement *elems;
    size_t count;
    size_t capacity;
} Program;


// Parses the tokens lexed from a file into a syntax tree.
Program *parse_file(Noh_Arena *arena, Tokens tokens, Errors *errors);

#endif // _PARSER_H
