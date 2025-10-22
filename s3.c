#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
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

    for(int i = 0; i < *argsc; i++){
        if(strcmp(args[i], ">") == 0  || strcmp(args[i], ">>") == 0  || strcmp(args[i], "<") == 0){
            i++;
        }
        else{
            args[index] = args[i];
            index++;
        }
    }
    args[index] == NULL;
    *argsc = index;
}

void launch_program_with_redirection(char *args[], int argsc){
    char tmp_line[MAX_LINE];
    tmp_line[0] = '\0';

    for(int i = 0; i < argsc; i++){
        strcat(tmp_line, args[i]);
        if(i < argsc - 1) strcat(tmp_line, " ");
    }

    int redir_type = command_with_redirection(tmp_line);

    int rc = fork();
    if(rc<0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
      
    else if(rc == 0){

        char *file = filename(args, argsc);
        clean_args(args, &argsc);

        if(redir_type == 1){
            child_with_output_redirected(args, argsc); // > operator
        }
        else if(redir_type == 2){
            child_with_input_redirected(args, argsc); // < operator
        }
        else if(redir_type == 3){
            child_with_output_redirected(args, argsc); // >> operator
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