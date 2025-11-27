# s3-system-shell
Build of a functional shell in C which implements many of the fundamental features found in shells like Bash

## Overview

This shell implements basic commands, and also allows for extended features and complpex command lines used in a bash shell. Including tasks such as batched commands, pipelining, and redirection, further enhacncements have been imoplemented including:
- Subshells and nested subshells
- Globbing
- Command history
- Job handling

A shell file `start.sh` has been used to allow for instant execution of the shell.

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

| Input | Output | Expected output? |
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
