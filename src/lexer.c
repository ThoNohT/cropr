#include "noh.h"
#include "common.h"
#include "lexer.h"

typedef struct {
    Noh_String_View line;
    Location start;
    bool has_pound;
} Line;

typedef struct {
    Line *elems;
    size_t count;
    size_t capacity;
} Lines;

// The list of symbols that should be lexed as full symbols.
// When in this list, the full symbol will be attempted to be lexed, otherwise, any individual character is
// considered a unique symbol if it isn't alphanumeric.
static char *multichar_symbols[] = { "::", "()", "->", "//", "/*", "*/" };

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

static bool is_numeric(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_alphanum(char c) {
    return is_alpha(c) || is_numeric(c);
}

static Token lex_number(Line *line, size_t *start_col) {
    Noh_String_View start = line->line;
    Noh_String_View pre = noh_sv_chop_while(&line->line, *is_numeric);
    if (line->line.count > 0 && line->line.elems[0] == '.') {
        noh_sv_increase_position(&line->line, 1);
        Noh_String_View post = noh_sv_chop_while(&line->line, *is_numeric);

        Token token = {
            .type = TokenNumberLiteral,
            .loc = location_move_right(line->start, *start_col),
            .value = noh_sv_substring(start, 0, pre.count + post.count + 1)
        };
        *start_col += pre.count + post.count + 1;
        return token;
    } else {
        Token token = {
            .type = TokenNumberLiteral,
            .loc = location_move_right(line->start, *start_col),
            .value = pre
        };
        *start_col += pre.count;
        return token;
    }
}

static Token lex_symbol(Line *line, size_t *start_col) {
    // It can be a multichar symbol.
    for (size_t i = 0; i < noh_array_len(multichar_symbols); i++) {
        Noh_String_View symbol = noh_sv_from_cstr(multichar_symbols[i]);
        if (noh_sv_starts_with(line->line, symbol)) {
            Token token = {
                .type = TokenSymbol,
                .loc = location_move_right(line->start, *start_col),
                .value = symbol
            };
            noh_sv_increase_position(&line->line, symbol.count);
            *start_col += symbol.count;
            return token;
        }
    }

    // If no symbol matched, return single character as symbol.
    Token token = {
        .type = TokenSymbol,
        .loc = location_move_right(line->start, *start_col),
        .value = noh_sv_substring(line->line, 0, 1)
    };
    noh_sv_increase_position(&line->line, 1);
    *start_col += 1;
    return token;
}

// Lexes a string literal, continuing until the specified end symbol.
static void lex_literal(Tokens *tokens, Errors *errors, Line *line, size_t *start_col, char end_symbol) {
    noh_sv_increase_position(&line->line, 1);
    Noh_String_View start = line->line;
    size_t length = 0;
    bool escaped = false;
    while ((line->line.elems[0] != end_symbol || escaped) && line->line.count > 0) {
        noh_sv_increase_position(&line->line, 1);
        length++;
        escaped = (line->line.elems[0] == '\\');
    }

    // If we are at the end of the line, show an error that the string never ended.
    if (line->line.count == 0) {
        Error error = {
            .type = LexerError,
            .message = noh_sv_from_cstr("Line ended while lexing string literal."),
            .loc = location_move_right(line->start, *start_col)
        };
        noh_da_append(errors, error);
    } 

    // Add the string token regardless.
    Token token = {
        .type = TokenStringLiteral,
        .loc = location_move_right(line->start, *start_col),
        .value = noh_sv_substring(start, 0, length)
    };
    noh_da_append(tokens, token);
    noh_sv_increase_position(&line->line, 1);
    *start_col += length + 2;
}

// Gets an indent token from a line. This should be done at the start of every nonempty line.
static void lex_indent(Tokens *tokens, Errors *errors, Line *line, size_t *end_col) {
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

static void lex_elem(Tokens *tokens, Errors *errors, Line *line, size_t *start_col) {
    (void)errors;
    // Skip whitespace, and add it as a token if there is any.
    Noh_String_View ws = noh_sv_chop_while(&line->line, *is_whitespace);
    if (ws.count > 0) {
        Token token = {
            .type = TokenWhitespace,
            .loc = location_move_right(line->start, *start_col),
            .value = ws
        };
        noh_da_append(tokens, token);
    }
    *start_col += ws.count;

    // If we only had whitespace left, just return.
    if (line->line.count == 0) return;

    char c = line->line.elems[0];
    // Otherwise, check for the first character to see what we should try to lex.
    if (is_alpha(c)) {
        // Try to lex a keyword.
        Noh_String_View value = noh_sv_chop_while(&line->line, *is_alphanum);
        Token token = {
            .type = TokenKeyword,
            .loc = location_move_right(line->start, *start_col),
            .value = value
        };
        noh_da_append(tokens, token);
        *start_col += value.count;
    } else if (is_numeric(c)) {
        // Try to lex a number
        noh_da_append(tokens, lex_number(line, start_col));
    } else if (c == '#') {
        // Try to lex a pound keyword.
        Noh_String_View after_hash = noh_sv_substring(line->line, 1, 0);
        Noh_String_View value = noh_sv_chop_while(&after_hash, *is_alphanum);
        if (value.count > 0) {
            Token token = {
                .type = TokenKeyword,
                .loc = location_move_right(line->start, *start_col),
                .value = noh_sv_substring(line->line, 0, value.count + 1) // Value + preceding #
            };
            noh_da_append(tokens, token);
            noh_sv_increase_position(&line->line, value.count + 1);
            line->has_pound = true;
            *start_col += value.count + 1;
        } else {
            // Otherwise, return the pound symbol as a symbol.
            Token token = {
                .type = TokenSymbol,
                .loc = location_move_right(line->start, *start_col),
                .value = noh_sv_from_cstr("#")
            };
            noh_da_append(tokens, token);
            noh_sv_increase_position(&line->line, 1);
            *start_col += 1;
        }
    } else if (c == '"') {
        // Try to lex a regular string.
        lex_literal(tokens, errors, line, start_col, '"');
    } else if (c == '\'') {
        lex_literal(tokens, errors, line, start_col, '\'');
    } else if (c == '<' && line->has_pound) {
        lex_literal(tokens, errors, line, start_col, '>');
    } else {
        noh_da_append(tokens, lex_symbol(line, start_col));
    }
}

void lex_file(Noh_String_View sv, char *filename, Tokens *tokens, Errors *errors) {
    Lines lines = {0};
    // Note that this string should not be freed, as it will live in the locations of the tokens.
    Noh_String fn_str = noh_string_from_cstr(filename);

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
}

