#define NOH_IMPLEMENTATION
#include "src/noh.h"
#define NOH_BLD_IMPLEMENTATION
#include "noh_bld.h"

#define DEBUG_TOOL "gf2"
#define COMPILER_TOOL "clang"

bool build_cropr() {
    bool result = true;
    Noh_Arena arena = noh_arena_init(10 KB);

    Noh_Cmd cmd = {0};
    Noh_File_Paths input_paths = {0};
    noh_da_append(&input_paths, "./src/noh.h");
    noh_da_append(&input_paths, "./src/main.c");

    int needs_rebuild = noh_output_is_older("./build/cropr", input_paths.elems, input_paths.count);
    if (needs_rebuild < 0) noh_return_defer(false);
    if (needs_rebuild == 0) {
        noh_log(NOH_INFO, "cropr is up to date.");
        noh_return_defer(true);
    }

    noh_cmd_append(&cmd, COMPILER_TOOL);

    // c-flags
    noh_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb");

    // Output
    noh_cmd_append(&cmd, "-o", "./build/cropr");

    // Source
    noh_cmd_append(&cmd, "./src/main.c");

    // Linker
    noh_cmd_append(&cmd, "-lm");

    if (!noh_cmd_run_sync(cmd)) noh_return_defer(false);

defer:
    noh_cmd_free(&cmd);
    noh_da_free(&input_paths);
    noh_arena_free(&arena);
    return result;
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
        if (!build_cropr()) return 1;

    } else if (strcmp(command, "run") == 0) {
        // Build and run.
        if (!build_cropr()) return 1;

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
        if (!build_cropr()) return 1;

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
