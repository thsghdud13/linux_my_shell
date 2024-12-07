#include "smallsh.h"
#include <pwd.h>
static char inpbuf[MAXBUF];     // Buffer to store the user's input line
static char tokbuf[2 * MAXBUF]; // Buffer to store tokens created from the input
static char *ptr = inpbuf;      // Pointer to traverse inpbuf
static char *tok = tokbuf;      // Pointer to store tokens in tokbuf

static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

int userin(char *p) {
    int c, count;
    ptr = inpbuf; // Reset ptr to the start of inpbuf
    tok = tokbuf; // Reset tok to the start of tokbuf
    printf("%s", p);
    count = 0;

    while (1) {
        if ((c = getchar()) == EOF)
            return EOF;
        if (count < MAXBUF)
            inpbuf[count++] = c;
        if (c == '\n' && count < MAXBUF) {
            inpbuf[count] = '\0';
            return count;
        }
        if (c == '\n' || count >= MAXBUF) {
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s", p);
        }
    }
}

int gettok(char **outptr) {
    int type;
    *outptr = tok; // Set 'outptr' to point to the current 'tok' position in 'tokbuf'

    while (*ptr == ' ' || *ptr == '\t') // Skip leading spaces and tabs in 'inpbuf'
        ptr++;

    *tok++ = *ptr;    // Copy the current character from 'ptr' to 'tok' and increment 'tok'
    switch (*ptr++) { // Determine the type of the token based on the current character
    case '\n':
        type = EOL;
        break;
    case '&':
        type = AMPERSAND;
        break;
    case ';':
        type = SEMICOLON;
        break;
    default:
        type = ARG;
        while (inarg(*ptr)) // While the character is not in 'special' characters, continue adding to the token
            *tok++ = *ptr++;
    }
    *tok++ = '\0';
    return type;
}

int inarg(char c) {
    char *wrk;

    for (wrk = special; *wrk; wrk++) { // Check if character 'c' is in the 'special' characters array
        if (c == *wrk)
            return 0;
    }
    return 1;
}

void procline() {
    char *arg[MAXARG + 1];
    int toktype, type;
    int narg = 0;
    for (;;) {                                  // Infinite loop to process tokens until end of line (EOL)
        switch (toktype = gettok(&arg[narg])) { // Get the next token and assign it to 'toktype'
        case ARG:
            if (narg < MAXARG)
                narg++;
            break;
        case EOL:
        case SEMICOLON:
        case AMPERSAND:
            if (toktype == AMPERSAND)
                type = BACKGROUND;
            else
                type = FOREGROUND;
            if (narg != 0) {
                arg[narg] = NULL;
                runcommand(arg, type);
            }
            if (toktype == EOL)
                return;
            narg = 0; // Reset argument count for the next command
            break;
        }
    }
}

int runcommand(char **cline, int where) {
    pid_t pid;
    int status;

    // ---------------------------------------------
    // Exit command: terminates the shell if 'exit' is entered
    // ---------------------------------------------

    if (strcmp(*cline, "exit") == 0)
        exit(1);

    // ---------------------------------------------
    // Handle 'cd' command with argument parsing
    // ---------------------------------------------
    if (strcmp(*cline, "cd") == 0)
        return handle_cd_command(cline);


    // ---------------------------------------------
    // Command execution: fork a new process and execute command
    // ---------------------------------------------

    switch (pid = fork()) {
    case -1:
        perror("smallsh");
        return -1;
    case 0: // Code executed by the child process
        execvp(*cline, cline);
        perror(*cline);
        exit(1);
    }

    /* following is the code of parent */
    if (where == BACKGROUND) {
        printf("[Process id] %d\n", pid);
        return 0;
    }
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    else
        return status;
}

// Function to handle 'cd' command
int handle_cd_command(char **cline) {
    int argCount = 0;

    // Count the number of arguments
    while (cline[argCount] != NULL) {
        argCount++;
    }

    if (argCount > 2) {
        printf("too many arguments.\n");
        return 1;
    }

    // Change to home directory if no argument is given
    if (argCount == 1) {
        struct passwd *userInfo = getpwuid(getuid());
        chdir(userInfo->pw_dir);
    } else if (argCount == 2) {
        char currentDir[200];
        char targetDir[200] = {0};

        getcwd(currentDir, sizeof(currentDir)); // Get current directory path
        struct passwd *userInfo = getpwuid(getuid());

        // Determine the target directory
        if (cline[1][0] == '~') { // Handle '~' as home directory shortcut
            strcpy(targetDir, userInfo->pw_dir);
            if (strcmp(cline[1], "~") != 0) {    // If "~" followed by path
                strcat(targetDir, cline[1] + 1); // Append path after '~'
            }
        } else if (cline[1][0] == '/') { // Absolute path
            strcpy(targetDir, cline[1]);
        } else { // Relative path
            snprintf(targetDir, sizeof(targetDir), "%s/%s", currentDir, cline[1]);
        }
        // Attempt to change directory and handle error if it fails
        if (chdir(targetDir) != 0) {
            perror("cd error"); // Print error message if directory change fails
            return 1;
        }
    }

    // Construct prompt string
    char prompt[200];
    char promptDir[200] = {0};
    getcwd(prompt, sizeof(prompt)); // Get the new current directory

    struct passwd *userInfo = getpwuid(getuid());
    if (strstr(prompt, userInfo->pw_dir) == NULL) {
        snprintf(promptDir, sizeof(promptDir), "%s$ ", prompt);
    } else {
        if (strcmp(userInfo->pw_dir, prompt) == 0) {
            strcpy(promptDir, "~$ ");
        } else {
            snprintf(promptDir, sizeof(promptDir), "~%s$ ", prompt + strlen(userInfo->pw_dir));
        }
    }

    // Start new input loop with updated prompt
    while (userin(promptDir) != EOF)
        procline();

    return 0;
}