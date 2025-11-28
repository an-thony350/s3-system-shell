# s3-system-shell
Build of a functional shell in C which implements many of the fundamental features found in shells like Bash

## Overview

This shell implements basic commands, and also allows for extended features and complpex command lines used in a bash shell. Including tasks such as batched commands, pipelining, and redirection, further enhacncements have been imoplemented including:
- Subshells and nested subshells
- Globbing
- Command history
- Job handling

This shell has been formed using the following files:
- A c++ file `s3main.c` acting as a main for the s3.c file
- A c++ file `s3.c` implementing all the functions definined in the header file
- A header file `s3.h` defining all functions, libraries and constants
- A shell file `start.sh` has been used to allow for instant execution of the shell.

## Basic Implementation

Basic implementation includes the following additions to the shell:
- Commands with redirection
- Support for cd
- Commands with pipes
- Batched commands

These implementations generally followed a formula to successfully be implemented:
1. Using a function to determine whether a specific symbol signifying a type of command is present, often named is_[name of addition] (e.g. `int is_pipe(charline[])` in the `s3.h` file.
2. A parsing function implementing tokenization where required
3. A launching function that actually runs the command

An additional function for batched commands included `int get_redirection_type_from_args(char *args[], int argsc)`. This was required due to issues occuring when a pipe and redirection command occur within the same command line.
These basic commands produced the following outputs when tested:

| Command | Output | Expected Output? |
| ------ | ----- | ---------------- |
| ls     | LICENSE  README.md  files.txt  s3  s3.c  s3.h  s3main.c  start.sh | ✅ |
| pwd | /home/dke24/s3-system-shell | ✅ |
| echo "Hello World!" | "Hello World!" | ✅ |
| echo "This is a test" > testfile.txt ; cat < testfile.txt| "This is a test" | ✅ |
| echo "This is a second line" >> testfile.txt ; wc -l < testfile.txt | 2 | ✅ |
| cd tmp (not in cwd) | cd failed: No such file or directory | ✅ |
| [s3-system-shell]$ cd /tmp | [tmp]$ cd | ✅ |
| echo "hello world" (pipe) wc -w | 2 | ✅ |
| echo "this is a string with a semicolon ; and a line (pipe) | "this is a string with a semicolon | ✖️|

As seen by the table above, the only issues in testing came when a specific character fir a command was used elswhere (i.e. a semicolon within an echo command).

## Proposed Extensions and Further Enhancements

The following explaiins each extension and their implementation in the shell.

### Subshells and Nested Subshells

This command is implemented using the following steps:
1. Using a function to determine if a subshell exists through the `has_subshell(char line[])` function
2. Extracting the subshell to form a command line with said subshell in `extract_subshell(char line[], char* subshell, char* remaining_cmd)`
3. Launching the subshell via forkng a child which then runs the command from the subshell in `launch_subshell(char* subshell, char lwd[])`
4. Using the `process_command` function within the main allowing for the subshell to act as the main shell

In order to allow for nested subshells, the folowing lines allowed for the initial subshells to be detected and then removed to intialise the required subshell values.
```
subshell[0] = '\0';
remaining_cmd[0] = '\0';
```

In testing, the following oututs were recieved:

| Command | Output | Expected Output ? |
| ------- | ------ | ----------------- |
| (echo "simple" ) | "simple" | ✅ |
| (echo "first" ); echo "second" | "first" "second" | ✅ |
| ( ( echo "nested" ) ) | "nested" execvp failed: No such file or directory | ❓ |
| (echo "nested"; (echo "not nested") ) | "nested" "not nested" execvp failed: No such file or directory | ❓ |

For subshells, there is clear success in testing, even when combining nesting with batched commands.
However, with nested subshells, the commands do give the correct output, but after each nested subshell an error `execvp failed: No such file or directory` occurs. Although this is an error, it doesn't affect overall design.

### Globbing

To implement globbing, the header `<glob.h> ` is used allowing for a `glob_t` variable to be used.

The command is then implemented as such:
1. A function `has_globs(char* args[], int argsc)` is used to determine whether characters required in globbing are present (and returning 1 if so)
2. A function `ext_globs(char* args[], int* argsc)` which, if has_globs is true, will do the following:
    - Iterate through each argument determining whether a glob character is present in the argument through `if(strpbrk(args[i], "*?[{"))` (will glob if = 1)
    - Forms a glob_t structure and glob flags `GLOB_TILDE` (expands to home directories) and `GLOB_NOCHECK` (returns file type if not present rather than failing)
    - Executes a `glob()` function which copies the filename of matches found
    - Preserves non-glob arguments and replaces original args[] array

With this implementation the following is also noted:
- `glob_result.gl_pathc` is the no. matches present given the command
- `glob_result.gl_pathv[j]` is the array (specifically the string in position j) holding the strings of the matches
- `globfree(&glob_result)` frees memory which is allocated by glob()

The following outputs were recieved in testing:

| Command | Output | Expected ? |
| ------- | ------ | ---------- |
| echo *.c | s3.c s3main.c | ✅ |
| ls file?.txt | files.txt | ✅ |
| echo *.py | *.py | ✅ - This output occurs due to the `GLOB_NOCHECK` flag |

### Command History

This addition was implemented via two functions:

1. `add_to_history(char* line, char* history[], int* history_count, int* current_history)` - adds a command to an array which stores the previously inputted commands. It will also remove commands when over 100 commands are used (the first entry into the array is removed and all other commands are shifted by -1 spots)
2. `show_history(char* history[], int history_count)` - prints a vertical list of all the commands in the array

When the command `history` is inputted, an ouptut similar to this example is produced:
```
1 ls
2 pwd
3 echo "Hello World!"
4 cat files.txt
5 /bin/echo "Using absolute path"
6 dne
7 echo "This is a test" > testfile.txt
8 cat < testfile.txt
9 echo "This is a second line" >> testfile.txt
10 wc -l < testfile.txt
11 sort < testfile.txt > sorted_testfile.txt
```

### Job Handling

To implement job handling, the header `<signal.h>` is used to implement job control in a shell. However as only basic job handling takes place (running background jobs, listing them and foregrounding them), this header isn't required. In a further extension, this could be implemented.

To implpement job handling, the structure below is used to implement jobs:

```
typedef struct{
    pid_t pid;
    char command[100];
    int job_id;
} Job;
```

Including this, the Job structure is added as global variables in both c files. This is because when implementing the following functions, it was much simpler to do this than add the Job structure and the job count to each parsing function.
```
//Used for job commands

Job jobs[MAX_JOBS];
int job_count = 0;
```
This basic handling is occured via the following funcctions:

1. `add_job(pid_t pid, char* cmd)` - adds a job to the structure of jobs (as well as its pid and job id)
2. `remove_job(pid_t pid)` - removes a job from the structure of jobs
3. `handle_jobs()` - shows all the jobs that are currently running
4. `handle_fg(char *job_id_str)` - This function "foregrounds" (i.e. bringing a background/stopped job to the foreground controlling the terminal). Via a I/O instruction (`waitpid()`), it will be removed from the job table and kernel will wake up the shell

The following outputs were recieved in testing:

| Command | Output | Expected |
| ------- | ------ | -------- |
| sleep 5 & | [1] 183430 | ✅ |
| jobs | [1] 183430 Running sleep | ✅ |
| ls & | [2] 306253 LICENSE  README.md  s3  s3.c  s3.h  s3main.c  start.sh | ✅ |
| jobs |  [1] 183430 Running sleep [2] 306253 Running ls | ✅ |
| fg | Bringing job [2] to foreground: ls | ✅ |
| jobs | [1] 183430 Running sleep | ✅ |
| fg %1 | Job [0] not found | ✖️ - Not implemented |

## Conclusion and Improvements

In conclusion, a functional shell has been implemented in C acting similar to a shell such as Bash. The basic features
of the shell have been implemented and further extensions have been added to improve userbility and also allow for
more features and functions. In terms of the basic tasks, they have been implemented to allow for complex command lines
which work as expected. In terms of the extensions, they have basic implementation. Globbing is working as expected
for all cases, and subshells work as expected (including nested subshells) where the slight errors are negligible to
the overall function of the shell and nesting. Job handling works as expected for basic implementation, but for more
complex commands, further implentation has not been included.

To further improve this shell, adding functions to handle more complex job control could be added via signal handling
with Ctrl+C/Z/D ore xpanding the `handle_fg()`, `add_jobs()` and parsing functions. Given more time, environment variables such as `$HOME` `$PATH` and `$USER` could also be implemented.
