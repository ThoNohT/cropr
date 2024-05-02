#include <stdio.h>

#include "noh.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char **argv) {
    char *program_name = noh_shift_args(&argc, &argv);
    (void)program_name;

    if (argc < 1) {
        noh_log(NOH_ERROR, "Please provide an input filename.");
        return 1;
    }
    char *filename = noh_shift_args(&argc, &argv);

    Noh_Arena arena = noh_arena_init(10 KB);
    Noh_String file_contents = {0};
    if (!noh_string_read_file(&file_contents, filename)) return 1;

    Tokens tokens = {0};
    Errors errors = {0};
    lex_file(noh_sv_from_string(&file_contents), filename, &tokens, &errors);

    noh_log(NOH_INFO, "Lexer result.");
    Noh_String pos = {0};
    Noh_String type = {0};
    for (size_t i = 0; i < tokens.count; i++) {
        Token token = tokens.elems[i];

        switch (token.type) {
            case TokenIndent: noh_string_append_cstr(&type, "Indent"); break;
            case TokenWhitespace: noh_string_append_cstr(&type, "Whitespace"); break;
            case TokenKeyword: noh_string_append_cstr(&type, "Keyword"); break;
            case TokenSymbol: noh_string_append_cstr(&type, "Symbol"); break;
            case TokenStringLiteral: noh_string_append_cstr(&type, "StringLiteral"); break;
            case TokenNumberLiteral: noh_string_append_cstr(&type, "NumberLiteral"); break;
        }

        format_location(&arena, &pos, token.loc);
        printf(Nsv_Fmt ": " Nsv_Fmt " - '" Nsv_Fmt "'\n", Nsv_Arg(pos), Nsv_Arg(type), Nsv_Arg(token.value));
        noh_string_reset(&pos);
        noh_string_reset(&type);
    }
    noh_string_free(&pos);


    if (errors.count > 0) {
        noh_log(NOH_ERROR, "Lexer failed.");
        Noh_String pos = {0};
        for (size_t i = 0; i < errors.count; i++) {
            format_location(&arena, &pos, errors.elems[i].loc);
            printf(Nsv_Fmt ": ERROR: '" Nsv_Fmt "'\n", Nsv_Arg(pos), Nsv_Arg(errors.elems[i].message));
            noh_string_reset(&pos);
        }
        noh_string_free(&pos);
        return 1;
    }

    return 0;
}
