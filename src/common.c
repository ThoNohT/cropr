#include "common.h"

// Formats a location as filename:row:col: and writes it to a Noh_String.
void format_location(Noh_Arena *arena, Noh_String *string, Location loc) {
    noh_arena_save(arena);
    char *cstr = noh_arena_sprintf(arena, Nsv_Fmt ":%zu:%zu", Nsv_Arg(loc.filename), loc.row, loc.col);
    noh_string_append_cstr(string, cstr);
    noh_arena_reset(arena);
}

// Moves right on a location by the specified distance.
Location location_move_right(Location loc, int distance) {
    Location result = {
        .filename = loc.filename,
        .row = loc.row,
        .col = loc.col + distance
    };
    return result;
}

