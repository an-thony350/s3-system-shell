#ifndef _S3_H_
#define _S3_H_

///See reference for what these libraries provide
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glob.h> //Used for globbing
#include <signal.h> //Used for jobs

///Constants for array sizes, defined for clarity and code readability
#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PROMPT_LEN 256
#define MAX_CMDS 16 // Used for task 4 - ensures safe pipelining
#define MAX_HISTORY 100 // Used for command history
#define MAX_JOBS 10 // Used for jobs

///Enum for readable argument indices (use where required)
enum ArgIndex
{
    ARG_PROGNAME,
    ARG_1,
    ARG_2,
    ARG_3,
};

///With inline functions, the compiler replaces the function call 
///with the actual function code;
///inline improves speed and readability; meant for short functions (a few lines).
///the static here avoids linker errors from multiple definitions (needed with inline).
static inline void reap()
{
    wait(NULL);
}

///Shell I/O and related functions (add more as appropriate)
void read_command_line(char line[], char lwd[]);
void construct_shell_prompt(char shell_prompt[]);
void parse_command(char line[], char *args[], int *argsc);

///Child functions (add more as appropriate)
void child(char *args[], int argsc);

///Program launching functions (add more as appropriate)
void launch_program(char *args[], int argsc);

//Task 2 Commands
int command_with_redirection(char line[]);
void launch_program_with_redirection(char *args[], int argsc);
//smaller functions for specific purposes
void child_with_output_redirected(char *args[], int argsc, char *filename, int append);
void child_with_input_redirected(char *args[], int argsc, char *filename);
char* filename(char *args[], int argsc);
void clean_args(char *args[], int *argsc);

//Task 3 Commands
void init_lwd(char lwd[]);
int is_cd(char line[]);
void run_cd(char *args[], int argsc, char lwd[]);

//Task 4 Commands
int is_pipe(char line[]);
void parse_pipe_command(char line[], char *cmds[MAX_CMDS][MAX_ARGS], int cmdsc[], int* num_cmds);
void launch_pipeline(char *cmds[MAX_CMDS][MAX_ARGS], int cmdsc[], int num_cmds);

//Task 5 Commands
int get_redirection_type_from_args(char *args[], int argsc);
int is_batched(char line[]);
void parse_batched_commands(char line[], char *args[], int *argsc);
void launch_batch(char *args[], int argsc, char lwd[]);

//Extension - Nested Subshell

int has_subshell(char line[]);
void extract_subshell(char line[], char* subshell, char* remaining_cmd);
void launch_subshell(char* subshell, char lwd[]);
void process_command(char *cmd, char lwd[]);

//Globbing

int has_globs(char* args[], int argsc);
void ext_globs(char* args[], int* argsc);

//History

void add_to_history(char* line, char* history[], int* history_count, int* current_history);
void show_history(char* history[], int history_count);

//Job control - struct to simplify implementation
typedef struct{
    pid_t pid;
    char command[100];
    int job_id;
} Job;

void add_job(pid_t pid, char* cmd);
void remove_job(pid_t pid);
void handle_jobs();
void handle_fg(char* job_id_str);


#endif
