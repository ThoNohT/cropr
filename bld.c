#define NOH_IMPLEMENTATION
#include "src/noh.h"
#define NOH_BLD_IMPLEMENTATION
#include "noh_bld.h"

#define DEBUG_TOOL "gf2"
#define COMPILER_TOOL "clang"

typedef struct{
    char **elems;
    size_t count;
    size_t capacity;
} Linker_Params;

static bool build(
        Noh_Arena *arena,
        Noh_Cmd *cmd,
        Noh_File_Paths *update_check_paths,
        char *input_path,
        char *output_path,
        Linker_Params *linker_params) {
    bool result = true;

    char *abs_out_path = noh_arena_sprintf(arena, "./build/%s", output_path);
    int needs_rebuild = noh_output_is_older(abs_out_path, update_check_paths->elems, update_check_paths->count);
    if (needs_rebuild < 0) noh_return_defer(false);
    if (needs_rebuild == 0) {
        noh_log(NOH_INFO, noh_arena_sprintf(arena, "%s is up to date.", output_path));
        noh_return_defer(true);
    }

    noh_cmd_append(cmd, COMPILER_TOOL);

    // c-flags
    noh_cmd_append(cmd, "-Wall", "-Wextra", "-ggdb");

    // Output
    if (noh_sv_ends_with(noh_sv_from_cstr(output_path), noh_sv_from_cstr(".o"))) {
        // Add -c flag if we are compiling a library, this does not link and simply outputs the compiled code.
        noh_cmd_append(cmd, "-c");
    }
    noh_cmd_append(cmd, "-o", abs_out_path);

    // Source
    char *abs_in_path = noh_arena_sprintf(arena, "./src/%s", input_path);
    noh_cmd_append(cmd, abs_in_path);

    // Linker
    if (linker_params) {
        for (size_t i = 0; i < linker_params->count; i++) {
            noh_cmd_append(cmd, linker_params->elems[i]);
        }
    }

    if (!noh_cmd_run_sync(*cmd)) noh_return_defer(false);

defer:
    noh_arena_reset(arena);
    noh_cmd_reset(cmd);
    noh_da_reset(update_check_paths);
    if (linker_params) noh_da_reset(linker_params);
    return result;
}

bool build_noh(Noh_Arena *arena, Noh_Cmd *cmd, Noh_File_Paths *ucp) {
    noh_da_append(ucp, "./src/noh.h");
    noh_da_append(ucp, "./src/noh.c");

    return build(arena, cmd, ucp, "noh.c", "libnoh.o", NULL);
}

bool build_common(Noh_Arena *arena, Noh_Cmd *cmd, Noh_File_Paths *ucp) {
    noh_da_append(ucp, "./src/noh.h");
    noh_da_append(ucp, "./src/common.h");
    noh_da_append(ucp, "./src/common.c");

    return build(arena, cmd, ucp, "common.c", "libcommon.o", NULL);
}

bool build_lexer(Noh_Arena *arena, Noh_Cmd *cmd, Noh_File_Paths *ucp) {
    noh_da_append(ucp, "./src/noh.h");
    noh_da_append(ucp, "./src/common.h");
    noh_da_append(ucp, "./src/lexer.h");
    noh_da_append(ucp, "./src/lexer.c");

    // Depends on libnoh.o

    return build(arena, cmd, ucp, "lexer.c", "liblexer.o", NULL);
}

bool build_parser(Noh_Arena *arena, Noh_Cmd *cmd, Noh_File_Paths *ucp) {
    noh_da_append(ucp, "./src/noh.h");
    noh_da_append(ucp, "./src/common.h");
    noh_da_append(ucp, "./src/lexer.h");
    noh_da_append(ucp, "./src/parser.h");
    noh_da_append(ucp, "./src/parser.c");

    // Depends on libnoh.o

    return build(arena, cmd, ucp, "parser.c", "libparser.o", NULL);
}

bool build_cropr(Noh_Arena *arena, Noh_Cmd *cmd, Noh_File_Paths *ucp, Linker_Params *lp) {
    // First build dependencies.
    if (!build_noh(arena, cmd, ucp)) return false;
    if (!build_common(arena, cmd, ucp)) return false;
    if (!build_lexer(arena, cmd, ucp)) return false;
    if (!build_parser(arena, cmd, ucp)) return false;

    noh_da_append(ucp, "./src/main.c");
    noh_da_append(ucp, "./build/libnoh.o");
    noh_da_append(ucp, "./build/libcommon.o");
    noh_da_append(ucp, "./build/liblexer.o");
    noh_da_append(ucp, "./build/libparser.o");

    noh_da_append(lp, "-lm");
    noh_da_append(lp, "-L./build");
    noh_da_append(lp, "-l:libnoh.o");
    noh_da_append(lp, "-l:libcommon.o");
    noh_da_append(lp, "-l:liblexer.o");
    noh_da_append(lp, "-l:libparser.o");

    return build(arena, cmd, ucp, "main.c", "cropr", lp);
}

void print_usage(char *program) {
    noh_log(NOH_INFO, "Usage: %s <command>", program);
    noh_log(NOH_INFO, "Available commands:");
    noh_log(NOH_INFO, "- build: build cropr (default).");
    noh_log(NOH_INFO, "- run: build and run cropr.");
    noh_log(NOH_INFO, "- test: build and run tests.");
    noh_log(NOH_INFO, "- debug: build and debug cropr using the defined debug tool.");
    noh_log(NOH_INFO, "- clean: clean all build artifacts.");
}

int main(int argc, char **argv) {
    noh_rebuild_if_needed(argc, argv);
    char *program = noh_shift_args(&argc, &argv);

    Noh_Arena arena = noh_arena_init(10 KB);
    Noh_Cmd cmd = {0};
    Noh_File_Paths ucp = {0};
    Linker_Params lp = {0};

    // Determine command.
    char *command = NULL;
    if (argc == 0) {
        command = "build";
    } else {
        command = noh_shift_args(&argc, &argv);
    }

    // Ensure build directory exists.
    if (!noh_mkdir_if_needed("./build")) return 1;

    if (strcmp(command, "build") == 0) {
        // Only build.
        if (!build_cropr(&arena, &cmd, &ucp, &lp)) return 1;

    } else if (strcmp(command, "run") == 0) {
        // Build and run.
        if (!build_cropr(&arena, &cmd, &ucp, &lp)) return 1;

        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, "./build/cropr");

        if (argc == 0) {
            // For easy debugging, use hello world example for now when running.
            noh_cmd_append(&cmd, "examples/hello_world.cr");
        }

        while (argc > 0) noh_cmd_append(&cmd, noh_shift_args(&argc, &argv)); 
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else if (strcmp(command, "debug") == 0) {
        // Build and debug.
        if (!build_cropr(&arena, &cmd, &ucp, &lp)) return 1;

        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, DEBUG_TOOL, "./build/cropr");
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else if (strcmp(command, "test") == 0) {
        // run tests
        printf("Tests not yet implemented.\n");
        return 1;

    } else if (strcmp(command, "clean") == 0) {
        Noh_Cmd cmd = {0};
        noh_cmd_append(&cmd, "rm", "-rf", "./build/");
        if (!noh_cmd_run_sync(cmd)) return 1;
        noh_cmd_free(&cmd);

    } else {
        print_usage(program);
        noh_log(NOH_ERROR, "Invalid command: '%s'", command);
        return 1;

    }
} 
