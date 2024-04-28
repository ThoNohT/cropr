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

// Lexes a file.
// Appends the generated tokens to tokens and the generated errors to errors.
void lex_file(Noh_Arena *arena, Noh_String_View sv, char *filename, Tokens *tokens, Errors *errors);
