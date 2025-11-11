#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN-6]; 

    init_lwd(lwd);///Implement this function: initializes lwd with the cwd (using getcwd)

    //Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    //The following are used for multiple pipes to store commands and arguments
    char *cmds[MAX_CMDS][MAX_ARGS];
    int argsc_arr[MAX_CMDS];
    int num_cmds;

    while (1) {

        read_command_line(line, lwd); ///Notice the additional parameter (required for prompt construction)

        if(is_batched(line)){
            char *batch_commands[MAX_CMDS];
            int num_batch_commands;
            parse_batched_commands(line, batch_commands, &num_batch_commands);
            launch_batch(batch_commands, num_batch_commands, lwd);
            reap();
        }
        else if(is_cd(line)){///Implement this function
            parse_command(line, args, &argsc);
            run_cd(args, argsc, lwd); ///Implement this function
        }
        else if(is_pipe(line)){
            parse_pipe_command(line, cmds, argsc_arr, &num_cmds);
            launch_pipeline(cmds, argsc_arr, num_cmds);
            reap();
        }
        else if(command_with_redirection(line)){
            ///Command with redirection
           parse_command(line, args, &argsc);
           launch_program_with_redirection(args, argsc);
           reap();
       }
       else ///Basic command
       {
           parse_command(line, args, &argsc);
           launch_program(args, argsc);
           reap();
       }
    }

    return 0;
}

