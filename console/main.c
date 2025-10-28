#include "common.h"
#include "commands.h"
#include "security.h"

int main(int argc, char *argv[]) {

	printf("▗▄▄▖  ▖ ▗▖▗▄▄▄▖▗▄▄▖\n▐▌   ▐▌ ▐▌▐▌   ▐▌ ▐▌\n▐▌   ▐▌ ▐▌▐▛▀▀▘▐▛▀▚\n▝▚▄▄▖▐▙█▟▌▐▙▄▄▖▐▙▄▞▘\n \n");
    // Basic argument validation
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    const char *command = argv[1];
    
    // Validate command string
    if (strlen(command) > MAX_COMMAND_LEN) {
        fprintf(stderr, "Error: Command name too long\n");
        return 1;
    }

    
    // Exec command
    int result = execute_command(command, argc - 1, argv + 1);
    if (result == -1) {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return result;
}
