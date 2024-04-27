#include <stdio.h>

#define NOH_IMPLEMENTATION
#include "noh.h"

typedef struct {
    Noh_String filename;
    size_t row;
    size_t col;
} Location;

typedef enum {
    LexerError,
    ParserError,
} ErrorType;

typedef struct {
    Noh_String_View message;
    ErrorType type;
    Location loc;
} Error;

typedef struct {
    Error *elems;
    size_t count;
    size_t capacity;
} Errors;

// Moves right on a location by the specified distance.
Location location_move_right(Location loc, int distance) {
    Location result = {
        .filename = loc.filename,
        .row = loc.row,
        .col = loc.col + distance
    };
    return result;
}

typedef struct {
    Noh_String_View line;
    Location start;
} Line;

typedef struct {
    Line *elems;
    size_t count;
    size_t capacity;
} Lines;

typedef enum {
    TokenKeyword,
    TokenComment,
    TokenSymbol,
    TokenStringLiteral,
    TokenIndent,
    TokenUnknown,
} TokenType;

typedef struct {
    Noh_String_View value;
    TokenType type;
    Location loc;
} Token;

typedef struct {
    Token *elems;
    size_t count;
    size_t capacity;
} Tokens;

bool is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

bool is_not_whitespace(char c) {
    return c != ' ' && c != '\t';
}

bool is_newline(char c) {
    return c == '\n' || c == '\r';
}

// Formats a location as filename:row:col: and writes it to a Noh_String.
void format_location(Noh_Arena *arena, Noh_String *string, Location loc) {
    noh_arena_save(arena);
    char *cstr = noh_arena_sprintf(arena, Nsv_Fmt ":%zu:%zu", Nsv_Arg(loc.filename), loc.row, loc.col);
    noh_string_append_cstr(string, cstr);
    noh_arena_reset(arena);
}

// Gets an indent token from a line. This should be done at the start of every nonempty line.
void lex_indent(Tokens *tokens, Errors *errors, Line *line, size_t *end_col) {
    Token token = {
        .type = TokenIndent,
        .loc = line->start,
    };
    if (is_whitespace(line->line.elems[0])) {
        Noh_String_View indent = noh_sv_chop_while(&line->line, *is_whitespace);
        token.value = indent;
        *end_col = indent.count;

        // If there is indentation, check that there are no invalid characters in the indentation.
        int tab_pos = noh_sv_index_of(indent, noh_sv_from_cstr("\t"));
        if (tab_pos >= 0) {
            Error error = {
                .type = LexerError,
                .message = noh_sv_from_cstr("Tabs are not allowed in indentation."),
                .loc = location_move_right(line->start, tab_pos)
            };
            noh_da_append(errors, error);
        }
    } else {
        // No whitespace at the start, insert a zero indentation token.
        Noh_String_View empty = { .elems = line->line.elems, .count = 0 };
        token.value = empty;
        *end_col = 0;

    }
    noh_da_append(tokens, token);
}

void lex_elem(Tokens *tokens, Errors *errors, Line *line, size_t *start_col) {
    (void)errors;
    Noh_String_View skipped_ws = noh_sv_chop_while(&line->line, *is_whitespace);
    *start_col += skipped_ws.count;

    // If we only had whitespace left, just return.
    if (line->line.count == 0) return;

    // Otherwise, check for the first character to see what we should try to parse.
    switch (line->line.elems[0]) {
        case '/':
            // Try to parse a single- or multiline comment.
            if (line->line.count < 2) {
                Token token = {
                    .type = TokenUnknown,
                    .loc = location_move_right(line->start, *start_col),
                    .value = noh_sv_substring(&line->line, 0, 1)
                };
                noh_da_append(tokens, token);

                Error error = {
                    .message = noh_sv_from_cstr("End of line reached while trying to parse a comment."),
                    .type = LexerError,
                    .loc = location_move_right(line->start, *start_col + 1)
                };
                noh_da_append(errors, error);

                *start_col += 1;
                noh_sv_increase_position(&line->line, 1);
                return;
            }

            if (line->line.elems[1] == '/') {
                // Single-line comment.
                Token token = {
                    .type = TokenComment,
                    .loc = location_move_right(line->start, *start_col),
                    .value = line->line
                };
                noh_da_append(tokens, token);

                *start_col += line->line.count;
                noh_sv_increase_position(&line->line, line->line.count);
                return;
            } else if (line->line.elems[1] == '*') {
                // Multiline comment.
            } else {
                Token token = {
                    .type = TokenUnknown,
                    .loc = location_move_right(line->start, *start_col),
                    .value = noh_sv_substring(&line->line, 0, 1)
                };
                noh_da_append(tokens, token);

                Error error = {
                    .message = noh_sv_from_cstr("Unknown token '/'"),
                    .type = LexerError,
                    .loc = location_move_right(line->start, *start_col)
                };
                noh_da_append(errors, error);

                *start_col += 1;
                noh_sv_increase_position(&line->line, 1);
                return;
            }
            break;
        case '#':
            // Try to parse a pound-include keyword.
            break;
        case '<':
            // Try to parse a <> string.
            break;
        case '"':
            // Try to parse a "" string.
            break;
        case '\'':
            // Try to parse a character.
            break;
        case '(':
            // Try to parse '()' or parenthesis symbol.
            break;
        case '-':
            // Try to parse '->' symbol.
            break;
        case '=':
            // return '=' symbol.
            break;
        default:
            // Try to get number or alphanumeric keyword.
            break;
    }


    // Otherwise, get the next token.
    Noh_String_View token_str = noh_sv_chop_while(&line->line, *is_not_whitespace);
    Token token = {
        .type = TokenSymbol,
        .loc = location_move_right(line->start, *start_col),
        .value = token_str
    };
    noh_da_append(tokens, token);
    *start_col += token_str.count;
}

// Lexes a file.
// Appends the generated tokens to tokens and the generated errors to errors.
void lex_file(Noh_Arena *arena, Noh_String_View sv, char *filename, Tokens *tokens, Errors *errors) {
    (void)arena; // TODO: Do we need this arena?
    Lines lines = {0};
    // Note that this string should not be freed, as it will live in the locations of the tokens.
    Noh_String fn_str = noh_string_from_cstr(filename);

    Location start_loc = (Location) { fn_str, 1, 1 };

    // Split up in lines.
    size_t line_counter = 0;
    while (sv.count > 0) {
        Line line = {0};
        line.line = noh_sv_chop_line(&sv);
        line.start = (Location) { fn_str, ++line_counter, 1 };
        noh_da_append(&lines, line);
    }

    // Gather tokens from lines.
    size_t end_col;
    for (size_t i = 0; i < lines.count; i++) {
        Line line = lines.elems[i];
        lex_indent(tokens, errors, &line, &end_col); // Check for indentation.
        while (line.line.count > 0) lex_elem(tokens, errors, &line, &end_col);
    }

    Error error = {
        .type = LexerError,
        .message = noh_sv_from_cstr("Lexer implementation is not yet finished."),
        .loc = start_loc
    };
    noh_da_append(errors, error);
}

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
    lex_file(&arena, noh_sv_from_string(&file_contents), filename, &tokens, &errors);

    noh_log(NOH_INFO, "Lexer result.");
    Noh_String pos = {0};
    Noh_String type = {0};
    for (size_t i = 0; i < tokens.count; i++) {
        Token token = tokens.elems[i];

        switch (token.type) {
            case TokenKeyword: noh_string_append_cstr(&type, "Keyword"); break;
            case TokenComment: noh_string_append_cstr(&type, "Comment"); break;
            case TokenSymbol: noh_string_append_cstr(&type, "Symbol"); break;
            case TokenStringLiteral: noh_string_append_cstr(&type, "StringLiteral"); break;
            case TokenIndent: noh_string_append_cstr(&type, "Indent"); break;
            case TokenUnknown: noh_string_append_cstr(&type, "Unknown"); break;
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
