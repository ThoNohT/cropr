#include <stdio.h>

#define NOH_IMPLEMENTATION
#include "noh.h"

typedef enum {
    TokenKeyword,
    TokenSymbol,
    TokenStringLiteral,
    TokenIndent,
} TokenType;

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
    for (size_t i = 0; i < tokens.count; i++) {
        format_location(&arena, &pos, tokens.elems[i].loc);
        printf(Nsv_Fmt ": '" Nsv_Fmt "'\n", Nsv_Arg(pos), Nsv_Arg(tokens.elems[i].value));
        noh_string_reset(&pos);
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
