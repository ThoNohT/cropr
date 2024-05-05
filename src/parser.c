#include "noh.h"
#include "parser.h"

static PreProc *parse_preproc(Noh_Arena *arena, Tokens *tokens, Errors *errors) {
    (void)arena;
    (void)tokens;
    (void)errors;

    return null;
}

Program *parse_file(Noh_Arena *arena, Tokens tokens, Errors *errors) {
    Program *program = {0};

    while (tokens.count > 0) {
        Token token = tokens.elems[0];
        if (token.type == TokenKeyWord && noh_sv_starts_with(token.value, noh_sv_from_cstr("#"))) {
            PreProc *preproc = parse_preproc(arena, tokens, errors);
            if (preproc) {
                noh_da_append(program, *preproc);
            }
        }

    }

    Error error = {
        .message = noh_sv_from_cstr("Parser is not finished."),
        .type = ParserError,
        .loc = tokens->elems[0].loc
    };
    noh_da_append(errors, error);

    return &program;
}
