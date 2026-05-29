#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "parser.h"
#include "executor.h"
#include "history.h"

char *HOME = NULL;

char* get_current_path() {
  // TODO: resolve memory leak
  char* cwd = getcwd(NULL, 0), *home = getenv("HOME");
  if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
    return cwd + strlen(home);
  } else return cwd;
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
    if (nread == -1) exit(EXIT_FAILURE);
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
    if (WIFEXITED(status)) {
      printf("[]\tdone (%d)\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("[]\tkilled (%d)\n", WTERMSIG(status));
    }
  }

  got_sigchld = 0;
}

int main() {
  // ignore all the bullsht
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  install_sigchld_handler();

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
    parse_line(&cl, line);
    append_to_process_history(&cl);
    exec_commandlist(&cl);

    free_commandlist(&cl);
  }

  
  return 0;
}
