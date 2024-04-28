#ifndef _COMMON_H
#define _COMMON_H

#include "noh.h"

typedef struct {
    Noh_String filename;
    size_t row;
    size_t col;
} Location;

// Formats a location as filename:row:col: and writes it to a Noh_String.
void format_location(Noh_Arena *arena, Noh_String *string, Location loc) ;

// Creats a new location that is the specified distance to the right.
Location location_move_right(Location loc, int distance);

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

#endif //_COMMON_H
