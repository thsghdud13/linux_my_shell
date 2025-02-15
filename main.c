#include "smallsh.h"
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

char prompt[200] = "Command> "; // Global prompt string initialized to "Command> "

// Function to initialize the prompt based on the current working directory
void initializePrompt() {
    char currentDir[200];                         // Buffer to store the current working directory
    struct passwd *userInfo = getpwuid(getuid()); // Retrieve user information

    if (getcwd(currentDir, sizeof(currentDir)) == NULL) {
        perror("getcwd error");
        return;
    }

    // Set up the prompt based on whether the current directory is within the home directory
    if (strstr(currentDir, userInfo->pw_dir) != NULL) { // Inside home directory or subdirectory
        if (strcmp(userInfo->pw_dir, currentDir) == 0) {
            snprintf(prompt, sizeof(prompt), "~$ "); // If exactly home directory, show "~$ "
        } else {
            snprintf(prompt, sizeof(prompt), "~%s$ ",
                     currentDir + strlen(userInfo->pw_dir)); // Show "~" + relative path
        }
    } else {
        snprintf(prompt, sizeof(prompt), "%s$ ", currentDir); // Outside home directory, show full path
    }
}

int main() {
    struct sigaction act;

    sigfillset(&act.sa_mask);
    act.sa_handler = sigchld_handler;
    act.sa_flags = SA_RESTART; // 안전하게 시스템 호출 재시작
    sigaction(SIGCHLD, &act, NULL);

    struct sigaction sigint_act;
    sigfillset(&sigint_act.sa_mask);
    sigint_act.sa_handler = sigint_handler;
    sigint_act.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sigint_act, NULL);

    // Initialize prompt
    initializePrompt();

    // ---------------------------------------------
    // Begin main loop to handle user commands
    // ---------------------------------------------

    // Main loop to continuously get user input and process commands
    while (userin(prompt) != EOF)
        procline(); // Process each line of user input

    return 0;
}