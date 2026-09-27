# tish 

`tish` is a minimal, lightweight Unix shell implemented in C. It serves as an interactive command-line interpreter, designed to handle standard shell behaviors including command execution, piping, redirection, job control, and custom built-in commands.

![tish Demo](docs/demo.gif)

## Prerequisites

- Unix-like OS (macOS/Linux)
- C compiler (`gcc`/`clang`)
- `make` 

## Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/hao20167/tish.git
   cd tish
   ```

2. **Build the project:**
   ```bash
   make all
   ```
   This will compile the object files into the `obj/` directory and place the final executable `tish` inside the `bin/` directory.

## Usage

```bash
# Run the shell
./bin/tish

# Or clean, build, and run in one step
make run

# Exit the shell (Or just simply press Ctrl+D)
exit

# Clean build files
make clean
```

## Features & Commands

### Core Features

- **Command Execution:** Run standard Unix/Linux commands and programs.
- **Pipelines:** Chain multiple commands together using pipes (`|`).
- **I/O Redirection:** Supports input/output redirection (`>`, `>>`, `<`, `>&`, `>>&`).
- **Logical Operators:** Conditional command execution using `&&` (AND), `||` (OR), and `;` (sequential).
- **Job Control:** Manage background and foreground processes seamlessly. Run tasks in the background with `&`, and control them using `bg`, `fg`, `jobs`, and `kill`.
- **Custom Path Management:** Easily view and update the shell's executable search paths.
- **Robust Signal Handling:** Safely handles terminal signals (e.g., `SIGINT`, `SIGTSTP`) and reaps zombie processes gracefully.

### Built-in Commands

`tish` comes with several natively supported commands to ensure core shell functionalities:

- `cd` - Change the current working directory.
- `jobs` - List all currently running or suspended background jobs.
- `bg` / `fg` - Resume jobs in the background or bring them to the foreground.
- `kill` - Send a signal (such as termination) to a process or job.
- `history` - Display the history of previously executed commands.
- `path` / `addpath` - View or append directories to the shell's search path.
- `source` - Read and execute commands from a given file in the current shell environment.
- `date` / `time` - Display the current date and time.
- `ls` - Built-in simple directory listing.
- `help` - Show usage and help information for `tish`.
- `exit` - Terminate the shell session.
