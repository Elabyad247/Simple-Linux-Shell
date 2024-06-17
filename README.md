# Simple Linux Shell

## Overview

This project is a custom implementation of a simple Linux shell, designed to execute user commands similarly to the bash shell. The shell supports basic command execution, background process handling, signal management, and both serial and parallel command execution.

## Features

- **Basic Command Execution**: Supports execution of standard Unix commands.
- **Background Processes**: Allows running commands in the background using `&`.
- **Serial Command Execution**: Supports sequential execution of multiple commands using `&&`.
- **Parallel Command Execution**: Supports parallel execution of multiple commands using `&&&`.
- **Signal Handling**: Handles `SIGINT` signal to interrupt and terminate processes.
- **Graceful Exit:** Implemented `exit` command to terminate the shell and all background processes.

## Getting Started

### Prerequisites

- GCC compiler
- Unix-like operating system (Linux)

### Running the Shell

To start the shell, compile and run the provided source code:

```shell
gcc -o my_shell my_shell.c
./my_shell
```

### Basic Commands

- **Change Directory:**
  
  ```shell
  $ cd /path/to/directory
  $ cd ..
  ```

- **Execute a Command:**
  
  ```shell
  $ ls
  $ echo "Hello, World!"
  ```

- **Background Execution:**
  
  ```shell
  $ sleep 10 &
  ```

- **Foreground Serial Execution:**
  
  ```shell
  $ command1 && command2 && command3
  ```

- **Foreground Parallel Execution:**
  
  ```shell
  $ command1 &&& command2 &&& command3
  ```

- **Exit the Shell:**
  
  ```shell
  $ exit
  ```

### Signal Handling

- **Terminate Foreground Process:** Press `Ctrl+C` to send `SIGINT` to the foreground process. The shell will remain running, and only the foreground process will be terminated. 

## Implementation Details

- The shell reads user input, tokenizes it, and uses `fork`, `exec`, and `wait` system calls to execute commands.
- Commands are executed in the foreground by default.

- Commands followed by `&` are executed in the background, allowing the shell to accept new commands immediately.
- Background processes are periodically reaped to prevent zombie processes.

- The `exit` command terminates the shell and all running background processes.
- The shell cleans up dynamically allocated memory before exiting.

- Custom handling of `SIGINT` to ensure the shell does not terminate but only the foreground process does.
- Background processes are placed in separate process groups to isolate them from `SIGINT`.

- Commands separated by `&&` are executed serially.
- Commands separated by `&&&` are executed in parallel.
- The shell ensures proper termination and cleanup of all foreground processes when `SIGINT` is received.
