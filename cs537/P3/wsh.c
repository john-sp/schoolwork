#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int wsh_cd(char **args);
int wsh_export(char **args);
int wsh_local(char **args);
int wsh_vars(char **args);
int wsh_history(char **args);
int wsh_exit(char **args);

extern char **environ;
int wsh_execute(char **args);
char **wsh_split_line(char *line);

#define MAX_VARIABLES 4096
int batchMode = 0;
FILE *in;

struct Variable
{
    char *name;
    char *value;
};

struct command
{
    char **argv;
};

struct Variable shell_variables[MAX_VARIABLES];
int num_shell_variables = 0;

int num_builtins = 6;
char *builtin_str[] = {
    "cd",
    "export",
    "local",
    "vars",
    "history",
    "exit"};
int (*builtin_func[])(char **) = {
    &wsh_cd,
    &wsh_export,
    &wsh_local,
    &wsh_vars,
    &wsh_history,
    &wsh_exit};

typedef struct HistoryNode
{
    char *command;
    struct HistoryNode *prev;
    struct HistoryNode *next;
} HistoryNode;

typedef struct HistoryList
{
    HistoryNode *head;
    HistoryNode *tail;
    int count;
    int maxCount;
} HistoryList;
HistoryList *history;
HistoryList *initHistoryList(int maxCommands)
{
    HistoryList *history = malloc(sizeof(HistoryList));
    history->head = NULL;
    history->tail = NULL;
    history->count = 0;
    history->maxCount = maxCommands;
    return history;
}

void freeAll()
{

    HistoryNode *curr = history->tail;
    while (curr != NULL)
    {
        HistoryNode *temp = curr;
        curr = curr->prev;
        free(temp->command);
        free(temp);
    }
    for (size_t i = 0; i < num_shell_variables; i++)
    {
        free(shell_variables[i].name);
        free(shell_variables[i].value);
    }

    free(history);
    if (batchMode == 1)
    {
        fclose(in);
    }
    
}

void addCommandToHistory(char *command)
{
    if (history->maxCount == 0)
    {
        return;
    }

    if (history->count == 0 || strcmp(history->tail->command, command) != 0)
    {

        HistoryNode *newNode = malloc(sizeof(HistoryNode));
        newNode->command = strdup(command);
        newNode->prev = NULL;
        newNode->next = NULL;

        if (history->head == NULL)
        {
            history->head = newNode;
        }
        else
        {
            history->tail->next = newNode;
            newNode->prev = history->tail;
        }
        history->tail = newNode;
        history->count++;

        if (history->count > history->maxCount)
        {
            HistoryNode *oldHead = history->head;
            history->head = oldHead->next;
            history->head->prev = NULL;
            free(oldHead->command);
            free(oldHead);
            history->count--;
        }
    }
}


// Taken and modified from DeepSeek-Coder-33B-instruct, run on local machine

void printHistoryVisualization()
{
    printf("History List Visualization:\n");
    printf("Count: %d\n", history->count);
    printf("Max Count: %d\n", history->maxCount);
    printf("Head: %p, Tail: %p\n\n", (void *)history->head, (void *)history->tail);

    HistoryNode *currentNode = history->head;

    while (currentNode != NULL)
    {
        printf("Command: %s\n", currentNode->command);
        printf("Prev: %p, Current: %p, Next: %p\n", (void *)currentNode->prev, (void *)currentNode, (void *)currentNode->next);
        printf("\n");

        currentNode = currentNode->next;
    }
}

void printHistory()
{
    HistoryNode *current = history->tail;
    int index = 1;

    while (current != NULL)
    {
        printf("%d) %s\n", index++, current->command);
        current = current->prev;
    }
}

void executeFromHistory(int index)
{
    if (index <= history->count)
    {
        HistoryNode *current = history->tail;
        for (int i = 1; i < index; i++)
        {
            current = current->prev;
        }
        char **args = wsh_split_line(current->command);
        wsh_execute(args);
        free(args);
    }
}

int wsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "wsh: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("wsh");
        }
    }
    return 1;
}

int wsh_export(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "wsh: expected argument for \"export\"\n");
    }
    else
    {
        char *variable = args[1];
        char *value = strchr(variable, '=');
        if (value != NULL)
        {
            *value++ = '\0';            // Split name and value
            setenv(variable, value, 1); // Overwrite existing variable
        }
        else
        {
            setenv(variable, "", 1); // Set empty value
        }
    }
    return 1;
}

int wsh_local(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "wsh: expected argument for \"local\"\n");
        return 1;
    }

    char *variable = args[1];
    char *value = strchr(variable, '=');

    if (value != NULL)
    {
        *value++ = '\0'; // Split name and value

        // Check if the variable already exists
        for (int i = 0; i < num_shell_variables; i++)
        {
            if (strcmp(shell_variables[i].name, variable) == 0)
            {
                // Free memory for the existing variable
                free(shell_variables[i].name);
                free(shell_variables[i].value);

                // Remove the variable by shifting the remaining variables
                for (int j = i; j < num_shell_variables - 1; j++)
                {
                    shell_variables[j] = shell_variables[j + 1];
                }

                num_shell_variables--;
                break;
            }
        }

        // If the value is not empty, add the variable
        if (*value != '\0')
        {
            if (num_shell_variables < MAX_VARIABLES)
            {
                shell_variables[num_shell_variables].name = strdup(variable);
                shell_variables[num_shell_variables].value = strdup(value);
                num_shell_variables++;
            }
            else
            {
                fprintf(stderr, "wsh: maximum number of shell variables exceeded\n");
            }
        }
    }
    else
    {
        fprintf(stderr, "wsh: invalid syntax for \"local\"\n");
    }

    return 1;
}

int wsh_vars(char **args)
{
    int i;
    for (i = 0; i < num_shell_variables; i++)
    {
        printf("%s=%s\n", shell_variables[i].name, shell_variables[i].value);
    }
    return 1;
}
int wsh_history(char **args)
{
    if (args[1] == NULL)
    {
        printHistory(history);
    }

    else if (strcmp(args[1], "set") == 0)
    {

        int new_size = strtol(args[2], NULL, 10);
        if (new_size < history->maxCount)
        {
            if (history->tail == NULL)
            {
                history->maxCount = new_size;
                return 1;
            }

            int toRemove = history->count - new_size;
            for (size_t i = 0; i < toRemove && history->head != NULL; i++)
            {
                HistoryNode *oldHead = history->head;
                history->head = oldHead->next;

                if (history->head != NULL)
                {
                    history->head->prev = NULL;
                }

                free(oldHead->command);
                free(oldHead);
                history->count--;
            }
            history->maxCount = new_size;
            if (history->count == 0)
            {
                history->tail = NULL;
            }
        }
        else
        {
            history->maxCount = new_size;
        }
    }
    else
    {

        int index = strtol(args[1], NULL, 10);
        executeFromHistory(index);
    }

    return -1;
}
int wsh_exit(char **args)
{
    freeAll();
    free(args);
    exit(0);
}

char *wsh_read_line(FILE *in) // MEM LEAK GET DELIM
{
    char *line = NULL;
    size_t bufsize = 0; // have getline allocate a buffer for us

    if (getline(&line, &bufsize, in) == -1)
    {
        if (feof(in))
        {
            free(line);
            freeAll();
            exit(0); // We recieved an EOF, exit 0
        }
        else
        {
            free(line);
            perror("readline");
            exit(1);
        }
    }
    line[strcspn(line, "\n")] = '\0';

    return line;
}

char **wsh_split_line(char *line)
{

    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "wsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " ");
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "wsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " ");
    }
    tokens[position] = NULL;
    // free(line2);
    return tokens;
}

int wsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
        {
            perror("execvp");
            return 99999999;
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Error forking
        perror("wsh");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    if (status == 2560)
    {
        return 10; // Return 10 if execvp didn't find a command
    }
    return 1;
}
int spawn_proc(int in, int out, struct command *cmd, pid_t pgid)
{
    pid_t pid;

    if ((pid = fork()) == 0)
    {
        // Set the process group id (pgid) of the child process
        setpgid(0, pgid);

        if (in != 0)
        {
            dup2(in, 0);
            close(in);
        }

        if (out != 1)
        {
            dup2(out, 1);
            close(out);
        }

        if (execvp(cmd->argv[0], cmd->argv) == -1)
        {
            perror("execvp");
            return 99999999;
        }
    } 
    else if (pid < 0)
    {
        // Error forking
        perror("fork");
    }

    return pid;
}

int fork_pipes(int n, struct command *cmd)
{
    int i;
    pid_t pid;
    int in, fd[2];
    int status;
    int original_stdin;
    pid_t pgid = 0;

    original_stdin = dup(0);
    if (original_stdin == -1)
    {
        perror("dup");
        exit(EXIT_FAILURE);
    }

    /* The first process should get its input from the original file descriptor 0.  */
    in = 0;

    /* Note the loop bound, we spawn here all, but the last stage of the pipeline.  */
    for (i = 0; i < n; ++i)
    {
        if (pipe(fd) == -1)
        {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Spawn the child process
        int status2 = spawn_proc(in, fd[1], cmd + i, pgid);
        if (status2 == 99999999)
        {
            return status2;
        }
        
        if (pgid == 0)
        {
            // Set the process group id (pgid) to the pid of the first child
            pgid = status2;
        }

        /* No need for the write end of the pipe, the child will write here.  */
        close(fd[1]);

        /* Keep the read end of the pipe, the next child will read from there.  */
        in = fd[0];
    }

    /* Last stage of the pipeline - set stdin to be the read end of the previous pipe
       and output to the original file descriptor 1. */
    if (in != 0)
        dup2(in, 0);

    // Fork for the last command
    if ((pid = fork()) == 0)
    {
        // Set the process group id (pgid) for the last child
        setpgid(0, pgid);

        execvp(cmd[i].argv[0], cmd[i].argv);
        perror("execvp");
        return 99999999; // Exit child process if execvp fails
    }
    else if (pid < 0)
    {
        // Error forking
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Wait for the last child process to finish
    waitpid(pid, &status, 0);
    for (i = 0; i < n; ++i)
    {
        waitpid(-1, &status, 0);
    }
    // Restore stdin to its original state
    if (dup2(original_stdin, 0) == -1)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }

    // Close the original stdin copy
    close(original_stdin);

    // TODO: Add function to wait for the rest of the children to run
    return 1;
}

int wsh_execute(char **args)
{
    int i;
    int num_pipes = 0;

    if (args[0] == NULL)
    {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; args[i] != NULL; i++)
    {
        if (args[i][0] == '$') // Check if argument starts with $
        {
            char *var_name = args[i] + 1; // Skip the $
            char *var_value = NULL;

            // Search for variable in env variables
            var_value = getenv(var_name);

            // If not found, search in shell variables
            if (var_value == NULL)
            {
                for (int j = 0; j < num_shell_variables; j++)
                {
                    if (strcmp(var_name, shell_variables[j].name) == 0)
                    {
                        var_value = shell_variables[j].value;
                        break;
                    }
                }
            }

            // Replace argument with variable value
            if (var_value != NULL)
            {
                args[i] = (var_value); // Assign the variable value
                if (args[i] == NULL)
                {
                    fprintf(stderr, "wsh: allocation error\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                for (int j = i; args[j] != NULL; j++)
                {
                    args[j] = args[j + 1];
                }
                i--;
            }
        }
    }

    if (args[0] == NULL)
    {
        // An empty command was entered.
        return 1;
    }

    // Count the number of pipes
    for (i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            num_pipes++;
        }
    }

    struct command commands[num_pipes + 1];

    // Split the commands based on pipe symbol
    int command_index = 0;
    commands[command_index].argv = args;

    for (i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            args[i] = NULL;
            command_index++;
            commands[command_index].argv = args + i + 1;
        }
    }
    if (num_pipes > 0)
    {
        return fork_pipes(num_pipes, commands);
    }
    else
    {
        for (i = 0; i < num_builtins; i++)
        {
            if (strcmp(args[0], builtin_str[i]) == 0)
            {
                return (*builtin_func[i])(args);
            }
        }

        return wsh_launch(args);
    }
}

void wsh_loop(FILE *in)
{
    char *line;
    char **args;
    int status;

    do
    {
        if (in == stdin)
        {
            printf("wsh> ");
        }

        line = wsh_read_line(in);
        char *line2 = strdup(line);
        args = wsh_split_line(line);
        if (args[0] == NULL)
        {

            free(line2);
            free(line);
            free(args);
            status = 1;
            continue;
        }

        else if (strcmp(args[0], "exit") == 0)
        {

            free(line);
            free(line2);
            wsh_exit(args);
        }

        status = wsh_execute(args);
        if (args[0] == NULL)
        {

            free(line2);
            free(line);
            free(args);
            status = 1;
            continue;
        }
        if (status == 99999999)
        {
            freeAll();
            free(line2);
            free(line);
            free(args);
            exit(1);
        }

        if (status != 10)
        {

            int isBuiltin = 0;
            for (int i = 0; i < num_builtins; i++)
            {

                if (strcmp(args[0], builtin_str[i]) == 0)
                {
                    isBuiltin = 1;
                }
            }
            if (isBuiltin == 0)
            {
                addCommandToHistory(line2);
            }
        }

        free(line2);
        free(line);
        free(args);
    } while (status);
}

int main(int argc, char const *argv[])
{
    history = initHistoryList(5);
    if (argc == 1)
    {
        wsh_loop(stdin);
    }
    else if (argc == 2)
    {
        in = fopen(argv[1], "r");
        if (in == NULL)
        {
            printf("ERROR: Can't open input file\n");
            return (1);
        }
        batchMode = 1;
        wsh_loop(in);
        fclose(in);
    }

    return 0;
}
