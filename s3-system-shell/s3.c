#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[], char lwd)
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.
    execvp(args[0], args);
    perror("execvp failed \n");
    exit(1);
}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.

    if(args[0] == NULL){
        return;
    }

    if(strcmp(args[0], "exit") == 0){
        exit(0);
    }

    int rc = fork();
    if(rc<0) {
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if (rc == 0){
        child(args, argsc);
        exit(1);
    }
    else{
        reap();
    }
}

int command_with_redirection(char line[]){
    // Checks if any redirection operators are present
    // Assumes no redirection operators otherwise present

    if(line == NULL) return 0;

    for(int i = 0; line[i] != '\0'; i++){
        if(line[i] == '>'){
            if(line[i+1] == '>') return 3;
            else return 1;
        }
        if(line[i] == '<') return 2;
    }
    return 0;
}

char* filename(char *args[], int argsc){
    //Extracts the filename from the args
    //Will allow for the input/output/append instructions
    //Note that strcmp retrns 0 if strings are equal

    for(int i = 0; i<argsc; i++){
        if(strcmp(args[i], ">") == 0  || strcmp(args[i], ">>") == 0  || strcmp(args[i], "<") == 0){
            if(i + 1 < argsc) {
                return args[i + 1];
            }
        }
    }
    return NULL;
}

void clean_args(char *args[], int *argsc){
    //Removes redirection operator and filename from args list
    //Required for correct execvp execution
    //argsc is a pointer as this value is changed

    int index = 0;
    int skip = 0;

    for(int i = 0; i < *argsc; i++){

        if(skip){
            skip = 0;
            continue;
        }
        if(strcmp(args[i], ">") == 0  || strcmp(args[i], ">>") == 0  || strcmp(args[i], "<") == 0){
            skip = 1;
        }
        else{
            args[index++] = args[i];
        }
    }
    args[index] = NULL;
    *argsc = index;
}

void child_with_input_redirected(char *args[], int argsc, char* filename){
    
    int fd = open(filename, O_RDONLY);
    if(fd<0){
        perror("open");
        exit(1);
    }

    dup2(fd, STDIN_FILENO);
    close(fd);
    execvp(args[0], args);
}

void child_with_output_redirected(char *args[], int argsc, char *filename, int append){


    int flags;

    if(append){
       flags = O_APPEND | O_WRONLY | O_CREAT;
    }
    else{
        flags = O_TRUNC | O_WRONLY | O_CREAT;

        // O_TRUNC Overwrites the file if it exists
        // File has write access, create access and overwrite access
    }

    int fd = open(filename, flags, 0644);
     // 0644 is the file permission (rw-r--r--)


    if(fd<0){
        perror("open");
        exit(1);
    }

    dup2(fd, STDOUT_FILENO);
    close(fd);
    execvp(args[0], args);
    perror("execvp failed");
    exit(1);
    
}

void launch_program_with_redirection(char *args[], int argsc){

    char tmp_line[MAX_LINE];
    tmp_line[0] = '\0';

    for(int i = 0; i < argsc; i++){
        strcat(tmp_line, args[i]);
        if(i < argsc - 1) strcat(tmp_line, " ");
    }

    int redir_type = command_with_redirection(tmp_line);

    // Extract filename in parent (this doesn't modify args)
    char *file = filename(args, argsc);

    int rc = fork();
    if(rc<0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if(rc == 0){
        // Clean args in the child process
        clean_args(args, &argsc);


        if(redir_type == 1){
            child_with_output_redirected(args, argsc, file, 0);
        }
        else if(redir_type == 2){
            child_with_input_redirected(args, argsc, file);
        }
        else if(redir_type == 3){
            child_with_output_redirected(args, argsc, file, 1);
        }
        else{
            fprintf(stderr, "Redirection operator error");
            exit(1);
        }
    }
    else{
        wait(NULL);
    }
}


int is_cd(char line[]) {
    if (line == NULL) return 0;

    int i = 0;
    while (line[i] == ' ' || line[i] == '\t') i++;

    if (line[i] == 'c' && line[i+1] == 'd') {
        char next_char = line[i+2];
        return (next_char == ' ') || (next_char == '\t') || 
               (next_char == '\n') || (next_char == '\0');
    }
    
    return 0;
}