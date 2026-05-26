#include "executor.h"
#include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>

// TODO: shorten this
static int apply_redir(Redirection *redir) {
  RedirType type = redir->type;
  int fd;
  if (type == R_IN) {
    fd = open(redir->target.str, O_RDONLY);
    if (fd < 0) goto err;
    if (dup2(fd, STDIN_FILENO) < 0) {
      perror("dup2 stdin");
      close(fd);
      return -1;
    }
  } else if (type == R_OUT) {
    fd = open(redir->target.str, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) goto err;
    if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2 stdout");
      close(fd);
      return -1;
    }
  } else if (type == R_APPEND) {
    fd = open(redir->target.str, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) goto err;
    if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2 stdout append");
      close(fd);
      return -1;
    }
  } else if (type == R_OUT_BOTH) {
    fd = open(redir->target.str, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) goto err;
    if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2 stdout out both");
      close(fd);
      return -1;
    }
    if (dup2(fd, STDERR_FILENO) < 0) {
      perror("dup2 stderr out both");
      close(fd);
      return -1;
    }
  } else if (type == R_APPEND_BOTH) {
    fd = open(redir->target.str, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) goto err;
    if (dup2(fd, STDOUT_FILENO) < 0) {
      perror("dup2 stdout out both");
      close(fd);
      return -1;
    }
    if (dup2(fd, STDERR_FILENO) < 0) {
      perror("dup2 stderr out both");
      close(fd);
      return -1;
    }
  } else {
    return 0;
  }

  close(fd);
  return 0;

  err:;
  perror(redir->target.str);
  return -1;
}

// exit in child proc may cause wrong buffer flushing, use _exit instead of
static void exec_command(Command *cmd) {
  for (int i = 0; i < (int)cmd->nredirs; i++) {
    if (apply_redir(&cmd->redirs[i]) < 0) {
      _exit(1);
    }
  }

  if (cmd->argc == 0) _exit(0);

  for (size_t i = 0; i < n_builtin_commands; i++) {
    if (strcmp(builtin_commands[i]->name, cmd->argv[0]) == 0) {
      CommandStatus status = builtin_commands[i]->handler(cmd);
      if (status != COMMAND_SUCCEEDED) _exit(1);
      _exit(0);
    }
  }
  
  execvp(cmd->argv[0], cmd->argv);
  perror(cmd->argv[0]);
  _exit(127);
}

static int exec_builtin_parent(Command *cmd, builtin_command_t *builtin, int input_fd) {
  int saved_stdin = -1;
  int saved_stdout = -1;
  int saved_stderr = -1;

  int code = 1;

  fflush(NULL); // WARN: why?
  saved_stdin = dup(STDIN_FILENO);
  saved_stdout = dup(STDOUT_FILENO);
  saved_stderr = dup(STDERR_FILENO);

  if (saved_stdin < 0 || saved_stdout < 0 || saved_stderr < 0) {
    perror("dup");
    goto cleanup;
  }

  if (input_fd >= 0) {
    if (dup2(input_fd, STDIN_FILENO) < 0) {
      perror("dup2 stdin");
      goto restore;
    }
  }

  for (int i = 0; i < (int)cmd->nredirs; i++) {
    if (apply_redir(&cmd->redirs[i]) < 0) {
      goto restore;
    }
  }

  CommandStatus status = builtin->handler(cmd);
  code = status == COMMAND_SUCCEEDED ? 0 : 1;

restore:
  if (saved_stdin >= 0) {
    if (dup2(saved_stdin, STDIN_FILENO) < 0) {
      perror("dup2 restore stdin");
    }
  }

  if (saved_stdout >= 0) {
    if (dup2(saved_stdout, STDOUT_FILENO) < 0) {
      perror("dup2 restore stdout");
    }
  }

  if (saved_stderr >= 0) {
    if (dup2(saved_stderr, STDERR_FILENO) < 0) {
      perror("dup2 restore stderr");
    }
  }

cleanup:
  if (saved_stdin >= 0) close(saved_stdin);
  if (saved_stdout >= 0) close(saved_stdout);
  if (saved_stderr >= 0) close(saved_stderr);

  return code;
}

static builtin_command_t *find_builtin_command(Command *cmd) {
  if (cmd->argc == 0) return NULL;
  for (size_t i = 0; i < n_builtin_commands; i++) {
    if (strcmp(builtin_commands[i]->name, cmd->argv[0]) == 0) {
      return builtin_commands[i];
    }
  }
  return NULL;
}

// return code = last cmd's
static int exec_pipeline(Pipeline *pipeline) {
  size_t ncmds = pipeline->ncmds;
  Command *cmds = pipeline->cmds;
  int pre[2], cur[2];
  // pipes[ncmds - 1][2]
  // write to pipes[i][1], then
  // read from pipes[i][0]
  pid_t pids[ncmds];

  int ran_last_command_parent = 0;
  int code = 1;

  for (int i = 0; i < ncmds; i++) {
    if (i == ncmds - 1) {
      builtin_command_t *builtin = find_builtin_command(&cmds[i]);
      if (pipeline->sep != S_BG && builtin != NULL && builtin->run_in_parent) {
        pids[i] = -1;
        code = exec_builtin_parent(&cmds[i], builtin, (i >= 1 ? pre[0] : -1));
        ran_last_command_parent = 1;
        if (i >= 1) close(pre[0]);
        continue;
      }
    }

    // before fork, for both childrent & parent
    if (i < ncmds - 1 && pipe(cur) < 0) {
      perror("pipe");
      goto fail;
    }
    
    fflush(NULL); // WARN: why ?
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      goto fail;
    } 

    if (pid == 0) {
      // only shell ignores signal, child processes dont
      signal(SIGINT, SIG_DFL); // set to default
      signal(SIGQUIT, SIG_DFL);
      signal(SIGTSTP, SIG_DFL);
      signal(SIGCHLD, SIG_DFL);

      if (i < ncmds - 1) close(cur[0]); // child proc doesnt need read-end pipe

      if (i > 0) {
        if (dup2(pre[0], STDIN_FILENO) < 0) {
          perror("dup2 stdin");
          _exit(1);
        }
        close(pre[0]); // close redundant
      }
      if (i < ncmds - 1) {
        if (dup2(cur[1], STDOUT_FILENO) < 0) {
          perror("dup2 stdout");
          _exit(1);
        }
        close(cur[1]);
      }

      exec_command(&cmds[i]);
      perror("exec_command");
      _exit(1);
    }

    if (i < ncmds - 1) close(cur[1]);
    if (i >= 1) close(pre[0]);

    pids[i] = pid;
    SWAP(pre[0], cur[0]);
    SWAP(pre[1], cur[1]);
  }

  if (pipeline->sep == S_BG) {
    printf("[] "); // TODO: job manager (array + first empty position)
    for (int i = 0; i < ncmds; i++) {
      printf("%d ", pids[i]);
    }
    printf("\n");
    return 0; // wont need this anw, bg proc wouldnt appear between anytime soon
  }

  for (int i = 0; i < ncmds; i++) {
    if (pids[i] <= 0) continue; // avoiding waitpid -1 at ran_last_command_parent
    int status;
    while (waitpid(pids[i], &status, 0) < 0) {
      if (errno == EINTR) { // SIGINT, could be from some bg proc (exit)
        continue;
      }
      perror("waitpid");
      break;
    }
    if (i == ncmds - 1 && !ran_last_command_parent) {
      if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        code = 128 + WTERMSIG(status);
      } 
    }
  }

  return code;

fail:
  if (pre[0] >= 0) close(pre[0]);
  if (pre[1] >= 0) close(pre[1]);
  if (cur[0] >= 0) close(cur[0]);
  if (cur[1] >= 0) close(cur[1]);

  for (size_t k = 0; k < ncmds; k++) {
    if (pids[k] > 0) {
      kill(pids[k], SIGTERM);
    }
  }

  for (size_t k = 0; k < ncmds; k++) {
    if (pids[k] > 0) {
      int status;
      while (waitpid(pids[k], &status, 0) < 0) {
        if (errno == EINTR) continue;
        break;
      }
    }
  }

  return 1;
}

void exec_commandlist(CommandList *cl) {
  int npipes = cl->npipes, exec_next = 1, last_code = 0;
  Pipeline *pipes = cl->pipes;

  for (int i = 0; i < npipes; i++) {
    Separator sep = pipes[i].sep;

    if (exec_next) {
      last_code = exec_pipeline(&pipes[i]);
    }

    if (sep == S_SEMI) {
      exec_next = 1;
    } else if (sep == S_OR) {
      exec_next = (last_code != 0); // GO_NEXT_IF_FAILED
    } else if (sep == S_AND) {
      exec_next = (last_code == 0); // GO_NEXT_IF_SUCCEEDED
    } else exec_next = 0;
    // else: BG/NONE
  }
}
