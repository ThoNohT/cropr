typedef enum {
    TokenIndent,
    TokenWhitespace,
    TokenKeyword,
    TokenSymbol,
    TokenStringLiteral,
    TokenNumberLiteral,
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

// Lexes a file.
// Appends the generated tokens to tokens and the generated errors to errors.
void lex_file(Noh_Arena *arena, Noh_String_View sv, char *filename, Tokens *tokens, Errors *errors);
