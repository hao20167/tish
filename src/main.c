#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "parser.h"
#include "executor.h"
#include "history.h"
#include "jobs.h"

char *HOME = NULL;

char* get_current_path() {
  // TODO: resolve memory leak
  char* cwd = getcwd(NULL, 0), *home = getenv("HOME");
  if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
    return cwd + strlen(home);
  } else return cwd;
}

pid_t shell_pgid = 0;
// to detach the shell from 'make run' process group since
// it may cause infinite loop in read_command (ctrlZ)
// codex my goat
void init_shell_pg() {
  shell_pgid = getpid();
  if (setpgid(0, shell_pgid) < 0) {
    perror("setpgrp shell");
  }
  if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
    perror("tcsetpgrp shell");
  }
}

void print_prompt() {
  printf("~%s\n", get_current_path());
  printf("❯ ");
  fflush(stdout);
}

void read_command(char **inp) {
  size_t sz = 0;
  ssize_t nread;
  do {
    print_prompt();
    nread = getline(inp, &sz, stdin);
    if (nread == -1) {
      if (feof(stdin)) {
        printf("\nExiting tish...\n");
        exit(EXIT_SUCCESS);
      }
      if (errno == EINTR) {
        clearerr(stdin);
        continue;
      }
      if (errno == EIO) {
        clearerr(stdin);
        // if read_command is not fg proc group, reset, then retry
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
          perror("tcsetpgrp shell");
          exit(EXIT_FAILURE);
        }
        printf("\n");
        continue;
      }
      perror("getline");
      exit(EXIT_FAILURE);
    }
    if (nread == 0) continue;
    (*inp)[nread - 1] = '\0';
    break;
  } while (1);
}

void print_error(char* err) {
  printf("tish: %s\n", err);
}

volatile sig_atomic_t got_sigchld = 0;
void sigchld_handler(int signo) {
  (void)signo;
  got_sigchld = 1;
}

void install_sigchld_handler() {
  struct sigaction sa = {0};
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; 
  if (sigaction(SIGCHLD, &sa, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }
}

// clean background
void reap() {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    int jid = getjid_if_done(pid);
    if (jid < 0) continue;
    // TODO: too lazy to fully implement
    if (WIFEXITED(status)) {
      printf("[%d]\tdone (%d) %s\n", jid, WEXITSTATUS(status), getjid_pipe(jid));
    } else if (WIFSIGNALED(status)) {
      printf("[%d]\tkilled (%d) %s\n", jid, WTERMSIG(status), getjid_pipe(jid));
    } else continue;
    free(getjid_pipe(jid)); // free only after all the proc in job was done
  }

  got_sigchld = 0;
}

int main() {
  // ignore all the bullsht
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  install_sigchld_handler();
  init_shell_pg();

  HOME = getenv("HOME");
  if (HOME == NULL) {
    fprintf(stderr, "tish: HOME not set\n");
    return EXIT_FAILURE;
  }

  init_shell_history();

  while (1) {
    if (got_sigchld) reap();

    char *line = NULL;
    read_command(&line);

    if (got_sigchld) reap();

    CommandList cl = {0};
    ParserStatus status = parse_line(&cl, line);

    if (status == PARSER_FAILED) {
      printf("tish: parser error\n");
      continue;
    }

    append_to_process_history(&cl);
    exec_commandlist(&cl);
    free_commandlist(&cl);
    printf("\n");
  }
  
  return 0;
}
