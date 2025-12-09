#include "s3.h"

//Used for job commands
// Note their inclusion in this file is merley for simpler
// implementaion of the following job functions

Job jobs[MAX_JOBS];
int job_count = 0;

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    char cwd[MAX_PROMPT_LEN];

    if(getcwd(cwd, sizeof(cwd)) != NULL){
        char *name_cwd = strrchr(cwd, '/');
        if(name_cwd != NULL && name_cwd[1] != '\0'){
            snprintf(shell_prompt, MAX_PROMPT_LEN, "[%s]$ ", name_cwd+1);
            //snprintf used to changed the memory contents in shell_prompt
        }
        else{
            snprintf(shell_prompt, MAX_PROMPT_LEN, "[%s]$ ", cwd);

        }

    }
    else{
        strcpy(shell_prompt, "[s3]$ ");
    }
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[], char lwd[])
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
    perror("execvp failed");
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

    //Used to check for a background job
    int background = 0;
    if(argsc > 0 && strcmp(args[argsc-1], "&") == 0){
        background = 1;
        args[--argsc] = NULL;
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
        if(background){
            printf("[%d] %d\n", job_count + 1, rc);
            add_job(rc, args[0]);
        }
        else reap();
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

    char *file = filename(args, argsc);

    int rc = fork();
    if(rc<0){
        fprintf(stderr, "fork failed\n");
        exit(1);
    }
    else if(rc == 0){
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

void init_lwd(char lwd[]){
    if(getcwd(lwd, MAX_PROMPT_LEN) == NULL){ 
        perror("getcwd failed");
        strcpy(lwd, ".");
        //Ensures that lwd stays in the current directory
        // if it contains "." and not the cwd
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

void run_cd(char *args[], int arg_count, char lwd[]) {
    char current_dir[MAX_LINE];
    getcwd(current_dir, sizeof(current_dir));
    
    int success = 0;
    
    if (arg_count == 1){
        char *home = getenv("HOME");
        if (home != NULL) success = (chdir(home) == 0);
    } 
    else if (arg_count == 2){
        if (strcmp(args[1], "-") == 0){
            success = (chdir(lwd) == 0);
        }
        else{
            success = (chdir(args[1]) == 0);
        }
    }
    
    if (!success) perror("cd failed");
    else strcpy(lwd, current_dir);
}

int is_pipe(char line[]){
    if (line == NULL) return 0;

    for(int i = 0; line[i] != '\0'; i++){
        if(line[i] == '|') return 1;
    }

    return 0;
}

int get_redirection_type_from_args(char *args[], int argsc){
    //Used specifically for task 5 when issues occur
    //with a pipe and redirection within the same argument
    for(int i = 0; i < argsc; i++){
        if(strcmp(args[i], ">") == 0) return 1;
        if(strcmp(args[i], "<") == 0) return 2;
        if(strcmp(args[i], ">>") == 0) return 3;
    }
    return 0;
}

void parse_pipe_command(char line[], char *cmds[MAX_CMDS][MAX_ARGS], int argsc_arr[], int* num_cmds){
    char *pipe_token = strtok(line, "|");
    *num_cmds = 0;
    char *tmp_cmd[MAX_CMDS];
    while(pipe_token != NULL && *num_cmds < MAX_CMDS){
        tmp_cmd[(*num_cmds)++] = pipe_token;
        pipe_token = strtok(NULL, "|");
    }

    for(int i = 0; i < *num_cmds; i++){

        parse_command(tmp_cmd[i], cmds[i], &argsc_arr[i]);
    }
}

void launch_pipeline(char *cmds[MAX_CMDS][MAX_ARGS], int argsc_arr[], int num_cmds){
    int prev_pipe = -1;
    int fd[2];

    for(int i = 0; i < num_cmds; i++){

        if(i < num_cmds - 1) pipe(fd);

        int rc = fork();

        if(rc == 0){
            if(i > 0){
                dup2(prev_pipe, STDIN_FILENO);
                close(prev_pipe);
            }
            if(i == num_cmds - 1){
                int redir_type = get_redirection_type_from_args(cmds[i], argsc_arr[i]);
                
                if (redir_type == 1 || redir_type == 3) {
                    char *file = filename(cmds[i], argsc_arr[i]);
                    
                    clean_args(cmds[i], &argsc_arr[i]); 

                    if(redir_type == 1){
                        child_with_output_redirected(cmds[i], argsc_arr[i], file, 0);
                    } else if(redir_type == 3){
                        child_with_output_redirected(cmds[i], argsc_arr[i], file, 1);
                    }
                }
            }

            if(i < num_cmds - 1){
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                close(fd[0]);
            }

            execvp(cmds[i][0], cmds[i]);
            perror("execvp failed");
            exit(1);
        }

        else{
            if(i > 0) close(prev_pipe);

            if(i< num_cmds - 1){
                close(fd[1]);
                prev_pipe = fd[0];
            }
        }
    }

    for(int i = 0; i < num_cmds; i++) wait(NULL);
}

int is_batched(char line[]){
    if(line == NULL) return 0;

    for(int i  = 0; line[i] != '\0'; i++){
        if(line[i] == ';') return 1;
    }

    return 0;
}

void parse_batched_commands(char line[], char *args[], int *argsc){
    char *semi_token = strtok(line, ";");
    *argsc = 0;
    while(semi_token != NULL && *argsc < MAX_ARGS - 1){
        args[(*argsc)++] = semi_token;
        semi_token = strtok(NULL, ";");
    }
}

void launch_batch(char *args[], int argsc, char lwd[]){
    for(int i = 0; i < argsc; i++){
        if(is_cd(args[i])){
            char *new_args[MAX_ARGS];
            int new_argsc;
            parse_command(args[i], new_args, &new_argsc);
            run_cd(new_args, new_argsc,lwd);
        }
        else if(is_pipe(args[i])){
            char* cmds[MAX_CMDS][MAX_ARGS];
            int argsc_arr[MAX_CMDS];
            int num_cmds;
            parse_pipe_command(args[i], cmds, argsc_arr, &num_cmds);
            launch_pipeline(cmds, argsc_arr, num_cmds);
        }
        else if(command_with_redirection(args[i])){
            char *new_args[MAX_ARGS];
            int new_argsc;
            parse_command(args[i], new_args, &new_argsc);
            launch_program_with_redirection(new_args, new_argsc);
        }
        else{
            char *new_args[MAX_ARGS];
            int new_argsc;
            parse_command(args[i], new_args, &new_argsc);
            launch_program(new_args, new_argsc);
        }
    }
}

int has_subshell(char line[]){
    if(line == NULL) return 0;
    for(int i = 0; line[i] != '\0'; i++){
        if(line[i] == '(') return 1;
    }
    return 0;
}

void extract_subshell(char line[], char* subshell, char* remaining_cmd){
    // Function "parses" the subshell, makes a new command within the subsell
    // taking it out of the main command
    //Adapted to handle nested subshells
    subshell[0] = '\0';
    remaining_cmd[0] = '\0';

    char* open_bracket = strchr(line, '(');
    if (!open_bracket) return;
    
    int paren_count = 1;
    char* current = open_bracket + 1;
    char* closed_bracket = NULL;
    
    while (*current != '\0') {
        if (*current == '(') paren_count++;
        else if (*current == ')') paren_count--;
        
        if (paren_count == 0) {
            closed_bracket = current;
            break;
        }
        current++;
    }

    int len_subshell = closed_bracket - open_bracket - 1;
    if (len_subshell > 0 && len_subshell < MAX_LINE) {
        strncpy(subshell, open_bracket + 1, len_subshell);
        subshell[len_subshell] = '\0';
    }
    if (*(closed_bracket + 1) != '\0') {
        char* remaining_start = closed_bracket + 1;
        while (*remaining_start == ' ' || *remaining_start == '\t') remaining_start++;
        if (*remaining_start != '\0') {
            strncpy(remaining_cmd, remaining_start, MAX_LINE - 1);
            remaining_cmd[MAX_LINE - 1] = '\0';
        }
    }
}

void launch_subshell(char* subshell, char lwd[]){
    int rc = fork();
    if(rc == 0){
        process_command(subshell, lwd);
        exit(0);
    }
    else wait(NULL);
}

void process_command(char *cmd, char lwd[]) {
    // Note that this is a repeat of the main due to subshells
    // But history has been removed from this
    char line_copy[MAX_LINE];
    strcpy(line_copy, cmd);
    if(strcmp(line_copy, "jobs") == 0){
        handle_jobs();
    }
    else if(strncmp(line_copy, "fg", 2) == 0){
        char* job_num = (strlen(line_copy) > 2) ? line_copy + 3 : NULL;
        handle_fg(job_num);
    }
    if(has_subshell(line_copy)){
        char subshell[MAX_LINE];
        char remaining_cmd[MAX_LINE];
        extract_subshell(line_copy, subshell, remaining_cmd);
        launch_subshell(subshell, lwd);
        if(strlen(remaining_cmd) > 0) process_command(remaining_cmd, lwd);
    }
    if(is_batched(line_copy)){
        char *batch_commands[MAX_CMDS];
        int num_batch_commands;
        parse_batched_commands(line_copy, batch_commands, &num_batch_commands);
        launch_batch(batch_commands, num_batch_commands, lwd);
        reap();
    }
    else if(is_cd(line_copy)){
        char *args[MAX_ARGS];
        int argsc;
        parse_command(line_copy, args, &argsc);
        run_cd(args, argsc, lwd);
    }
    else if(is_pipe(line_copy)){
        char *cmds[MAX_CMDS][MAX_ARGS];
        int argsc_arr[MAX_CMDS];
        int num_cmds;
        parse_pipe_command(line_copy, cmds, argsc_arr, &num_cmds);
        launch_pipeline(cmds, argsc_arr, num_cmds);
        reap();
    }
    else if(command_with_redirection(line_copy)){
        char *args[MAX_ARGS];
        int argsc;
        parse_command(line_copy, args, &argsc);
        launch_program_with_redirection(args, argsc);
        reap();
    }
    else {
        char *args[MAX_ARGS];
        int argsc;
        parse_command(line_copy, args, &argsc);
        launch_program(args, argsc);
        reap();
    }
}

int has_globs(char* args[], int argsc){
    for(int i = 0; i < argsc; i++){
        if(strpbrk(args[i], "*?[{")) return 1;
    }
    return 0;
}

void ext_globs(char* args[], int* argsc){
    if(!has_globs(args, *argsc)) return;

    char* new_args[MAX_ARGS];
    int new_argsc = 0;

    // Forming of new glob_t structure and flags
    for(int i = 0; i < *argsc && new_argsc < MAX_ARGS -1; i++){
        
        if(strpbrk(args[i], "*?[{")){
            glob_t glob_result;
            int glob_flags = GLOB_NOCHECK | GLOB_TILDE;
            if(glob(args[i], glob_flags, NULL, &glob_result) == 0){
                for(int j = 0; j < glob_result.gl_pathc && new_argsc < MAX_ARGS - 1; j++){
                    new_args[new_argsc++] = strdup(glob_result.gl_pathv[j]);
                }
            }
            globfree(&glob_result);  
        }
        else{
            new_args[new_argsc++] = args[i];
        }
    }
    new_args[new_argsc] = NULL;
    for(int i = 0; i < new_argsc; i++) {
        args[i] = new_args[i];
    }
    *argsc = new_argsc;
}

void add_to_history(char* line, char* history[], int* history_count, int* current_history){
    // array is used to form history list
    if(strlen(line) == 0) return;

    if(*history_count > 0 && strcmp(history[*history_count-1], line) == 0) return;

    if(*history_count < MAX_HISTORY){
        history[*history_count] = malloc(strlen(line) + 1);
        if(history[*history_count]){
            strcpy(history[*history_count], line);
            (*history_count)++;
        }
    }
    else{
        free(history[0]);
        for(int i = 1; i < MAX_HISTORY; i++){
            history[i-1] = history[i];
        }
        history[MAX_HISTORY-1] = malloc(strlen(line) + 1);
        if(history[MAX_HISTORY-1]){
            strcpy(history[MAX_HISTORY-1], line);
        }
    }
    *current_history = *history_count;
}

void show_history(char* history[], int history_count){
    printf("History of commands:\n");
    for(int i = 0; i < history_count; i++){
        printf("%d %s\n", i+1, history[i]);
    }
}

// Note that this implementation is for simplpe jobs
// i.e. ones using '&' to specify them

void add_job(pid_t pid, char* cmd){
    if(job_count < MAX_JOBS){
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, cmd, 99);
        jobs[job_count].job_id = job_count + 1;
        job_count++;
    }
}

void remove_job(pid_t pid){
    for(int i = 0; i < job_count; i++){
        if(jobs[i].pid == pid){
            for(int j = i; j < job_count - 1; j++) jobs[j] = jobs[j+1];
        
        job_count--;
        break;
        }
    }
}

void handle_jobs(){
    if(job_count == 0){
        printf("No background jobs\n");
        return;
    }
    
    for(int i = 0; i < job_count; i++) {
        printf("[%d] %d Running %s\n", jobs[i].job_id, jobs[i].pid, jobs[i].command);
    }
}

void handle_fg(char *job_id_str) {
    if(job_count == 0) {
        printf("No background jobs\n");
        return;
    }
    
    // atoi() converts the string value into an integer
    int job_id = job_id_str ? atoi(job_id_str) : job_count;
    
    for(int i = 0; i < job_count; i++) {
        if(jobs[i].job_id == job_id) {
            printf("Bringing job [%d] to foreground: %s\n", job_id, jobs[i].command);
            waitpid(jobs[i].pid, NULL, 0);
            remove_job(jobs[i].pid);
            return;
        }
    }
    printf("Job [%d] not found\n", job_id);
}
