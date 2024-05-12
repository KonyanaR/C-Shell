#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

// Define ANSI escape codes for colors
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RED "\033[1;31m"
#define COLOR_NORMAL "\033[0m"

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGUMENTS 10

// Function prototypes
void executeCommand(char* command);
char** tabCompletion(const char* text, int start, int end);
void changeDirectory(char* directory);
void clearScreen();
void listDirectory(char* directory);
void listEnvironment();
void echo(char* comment);
void displayHelp();
void pauseExecution();
void quitShell();
void sigintHandler(int signum);
void sigtstpHandler(int signum);
void diskUsage();
void generatePassword(int length);
int isRootUser();

// Function to retrieve the current working directory and hostname for the prompt
char* getPromptPrefix() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // Get hostname
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            perror("gethostname");
            strcpy(hostname, "hostname"); // Default hostname if not found
        }

        // Get username
        char* username = getenv("USER");
        if (username == NULL) {
            username = "user"; // Default username if not found
        }

        // Check if the user is root
        int isRoot = isRootUser();

        // Constructing the prompt with colors
        char* prompt = malloc(1024);
        if (isRoot) {
            sprintf(prompt, "%s%s@%s%s:%s%s%s%s ", COLOR_RED, username, hostname, COLOR_NORMAL, COLOR_BLUE, cwd, "\\myshell -#", COLOR_NORMAL);
        } else {
            sprintf(prompt, "%s%s@%s%s:%s%s%s%s ", COLOR_YELLOW, username, hostname, COLOR_NORMAL, COLOR_BLUE, cwd,"\\myshell -$", COLOR_NORMAL);
        }

        return prompt; // Return current working directory
    } else {
        perror("getcwd");
        return NULL; // Return NULL in case of error
    }
}

// Forward declaration of predefinedCommandsGenerator
char* predefinedCommandsGenerator(const char* text, int state);

// Global variables
char* predefinedCommands[] = {
    "mycd", "myclr", "mydir", "myenviron", "myecho", 
    "myhelp", "mypause", "myquit", "myone",
    "mytwo",
    NULL // Null terminator
};

// Function to check if the user is root
int isRootUser() {
    return geteuid() == 0;
}

// Function to handle tab completion
char** tabCompletion(const char* text, int start, int end) {
    // Prevent compiler warnings
    (void)end;

    rl_attempted_completion_over = 1; // Signal that we're done

    // If this is the first word being completed...
    if (start == 0) {
        return rl_completion_matches(text, &predefinedCommandsGenerator);
    }

    return NULL;
}

//Function to change directory
void changeDirectory(char* directory) {
    if (directory == NULL) {
        // Display current directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current directory: %s\n", cwd);
        } else {
            perror("getcwd");
        }
    } else {
        // Change directory
        char* oldpwd = getenv("PWD");
        if (oldpwd != NULL) {
            setenv("OLDPWD", oldpwd, 1); // Update OLDPWD
        }

        if (chdir(directory) == 0) {
            // Update the 'PWD' environment variable
            setenv("PWD", directory, 1);
            printf("Directory changed to: %s\n", directory);
        } else {
            perror("chdir");
        }
    }
}

// Main function
int main() {
    char* command;
    char* prompt = getPromptPrefix();

    // Check if prompt is NULL
    if (prompt == NULL) {
        return 1; // Error handling
    }

    // Set tab completion function
    rl_attempted_completion_function = (rl_completion_func_t*)tabCompletion;

    // Set signal handlers for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z)
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, sigtstpHandler);

    while (1) {
        command = readline(prompt);
        add_history(command);

        if (command && *command) {
            executeCommand(command);
        }

        free(command);
        free(prompt);
        prompt = getPromptPrefix();
    }

    return 0;
}

// Function to execute a command
void executeCommand(char* command) {
    char* arguments[MAX_ARGUMENTS];
    
    
    for (int i = 0; command[i] && command[i] != ' '; i++) {
        command[i] = tolower(command[i]);
    }

    // Tokenize the command string
    int numArgs = 0;
    char* token = strtok(command, " \n");
    while (token != NULL && numArgs < MAX_ARGUMENTS) {
        arguments[numArgs++] = token;
        token = strtok(NULL, "\n"); // Changed delimiter to "\n" for handling long strings
    }
    arguments[numArgs] = NULL;
    

    if (numArgs > 0) {

        int i = 0;
        while (predefinedCommands[i] != NULL) {
            if (strcmp(arguments[0], predefinedCommands[i]) == 0) {
                // Execute the corresponding command
                switch (i) {
                    case 0: // mycd
                        changeDirectory(arguments[1]);
                        break;
                    case 1: // myclr
                        clearScreen();
                        break;
                    case 2: // mydir
                        listDirectory(arguments[1]);
                        break;
                    case 3: // myenviron
                        listEnvironment();
                        break;
                    case 4: // myecho
                        echo(arguments[1]);
                        break;
                    case 5: // myhelp
                        displayHelp();
                        break;
                    case 6: // mypause
                        pauseExecution();
                        break;
                    case 7: // myquit
                        quitShell();
                        break;
                    case 8: // myone
                        generatePassword(atoi(arguments[1]));
                        break;
                    case 9: // mytwo
                        diskUsage();
                        break;
                    default:
                        printf("Unhandled command: %s\n", arguments[0]);
                        break;
                }
                return;
            }
            i++;
        }

        // If the command is not one of the predefined commands
        printf("Unknown command: %s\n", arguments[0]);
    }
}

// Function to handle the 'myclr' command
void clearScreen() {
    system("clear");
}

// Function to handle the 'mydir' command
void listDirectory(char* directory) {
    if (directory == NULL) {
        // List contents of current directory
        system("ls");
    } else {
        // List contents of specified directory
        char command[MAX_COMMAND_LENGTH];
        snprintf(command, sizeof(command), "ls %s", directory);
        system(command);
    }
}

// Function to handle the 'myenviron' command
void listEnvironment() {
    extern char** environ;
    for (int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
}

// Function to handle the 'myecho' command
void echo(char* comment) {
    if (comment != NULL) {
        // Loop to reduce multiple spaces/tabs to a single space
        int i = 0, j = 0;
        while (comment[i]) {
            if (!(comment[i] == ' ' || comment[i] == '\t')) {
                comment[j++] = comment[i];
            } else if (i > 0 && !(comment[i - 1] == ' ' || comment[i - 1] == '\t')) {
                comment[j++] = ' ';
            }
            i++;
        }
        comment[j] = '\0'; // Null-terminate the string after removing extra spaces/tabs
        printf("%s\n", comment);
    } else {
        printf("\n");
    }
}
// Function to handle the 'myhelp' command
void displayHelp() {
    // Open a pipe to the more command
    FILE *pipe = popen("more", "w");
    if (pipe == NULL) {
        perror("popen");
        return;
    }

    // Write the help message to the pipe
    fprintf(pipe, "MyShell Help\n");
    fprintf(pipe, "-------------\n");
    fprintf(pipe, "mycd - Change directory to root if the user is root, otherwise show current directory.\n");
    fprintf(pipe, "myclr - Clear the screen.\n");
    fprintf(pipe, "mydir - List the contents of the current directory.\n");
    fprintf(pipe, "myenviron - List all environment strings.\n");
    fprintf(pipe, "myecho - Display a test message.\n");
    fprintf(pipe, "myhelp - Display the user manual.\n");
    fprintf(pipe, "mypause - Pause the shell until 'Enter' is pressed.\n");
    fprintf(pipe, "myquit - Quit the shell.\n");
    fprintf(pipe, "myone - Generates a random password of given length.\n");
    fprintf(pipe, "mytwo - Display disk usage.\n");

    // Close the pipe
    pclose(pipe);
}

// Function to handle the 'mypause' command
void pauseExecution() {
    printf("Press Enter to continue...");
    fflush(stdout); // Flush the output buffer to ensure the message is displayed

    // Loop until Enter key is pressed
    while (getchar() != '\n') {
        // Do nothing, just wait for Enter key
    }
}

// Function to handle the 'myquit' command
void quitShell() {
    printf("Exiting MyShell...\n");
    exit(0);
}

// Function to handle the 'myone' command
void generatePassword(int length) {
    // Define the character set for the password
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+";
    if (length > 0){
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Generate and print the password
    printf("Generated password: ");
    for (int i = 0; i < length; ++i) {
        printf("%c", charset[rand() % (sizeof(charset) - 1)]);
    }
    printf("\n");
   }
   else {
        printf("Invalid password length\n");  
	}
}
// Signal handler for SIGINT (Ctrl+C)
void sigintHandler(int signum) {
    printf("\nSIGINT received. Use 'myquit' to exit the shell.\n");
    // Re-enable the signal handler
    signal(SIGINT, sigintHandler);
}

// Signal handler for SIGTSTP (Ctrl+Z)
void sigtstpHandler(int signum) {
    printf("\nSIGTSTP received. Use 'myquit' to exit the shell.\n");
    // Re-enable the signal handler
    signal(SIGTSTP, sigtstpHandler);
}

// Function to display disk usage
void diskUsage() {
    system("df -h");
}

// Generator function for tab completion
char* predefinedCommandsGenerator(const char* text, int state) {
    static int listIndex, len;
    const char* name;

    // If this is the first command being completed...
    if (!state) {
        listIndex = 0;
        len = strlen(text);
    }

    // Iterate through the predefined commands
    while ((name = predefinedCommands[listIndex++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    // No more matches
    return NULL;
}

