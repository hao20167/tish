#include "fg.h"
#include "builtins.h"
#include "jobs.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/errno.h>

int is_number(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] > '9' || str[i] < '0') return 0;
  }
  return 1;
}

static int handler(Command *cmd) {
  // move a background job to foreground
  if (cmd->argc > 2) return COMMAND_FAILED;
  int jid = -1;
  if (cmd->argc == 1) {
    jid = get_current_job();
    if (getjid_remaining(jid) <= 0) goto NO_CURRENT_JOB;
  }
  if (cmd->argc == 2) {
    char *num = cmd->argv[1];
    if (num[0] != '%' || strlen(num) > 4 + 1) return COMMAND_FAILED;
    num = num + 1;
    if (is_number(num)) {
      jid = atoi(num);
    } else {
      if (strcmp(num, "+") == 0) {
        jid = get_current_job();
      } else if (strcmp(num, "-") == 0) {
        jid = get_previous_job(get_current_job());
      } else goto NO_SUCH_JOB;
    }
    if (jid <= 0 || jid >= JMAX) goto NO_SUCH_JOB;
    if (getjid_remaining(jid) <= 0) goto NO_SUCH_JOB;
  }

  pid_t shell_pgid = getpgrp();
  pid_t pgid = jt[jid].pgid;
  if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
    perror("tcsetpgrp fg");
    return COMMAND_FAILED;
  }

  if (kill(-pgid, SIGCONT) < 0) {
    perror("kill SIGCONT");
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
      perror("tcsetpgrp restore");
    }
    return COMMAND_FAILED;
  }

  // didnt need to be removed from jt yet, reason:
  // 1. there wont be any `jobs` command (fg proc is running)
  // 2. could be added back to jt (ctrlZ 1 more time)
  // jt[jid].remaining = 0; 
  // TODO: add mark? +/-
  printf("[%d]    %9s %s\n", jid, ProcStatus[P_CONTINUED], jt[jid].pipe);
  
  // does this return code affect original pipeline/commandlist?
  // i think it should not, its just used for current pipeline
  // e.g. fg | ... 
  int status, code = 1;
  while (1) {
    int ret = waitpid(-pgid, &status, WUNTRACED);
    if (ret < 0) {
      if (errno == ECHILD) break; // procs are all done
      if (errno == EINTR) continue;
      perror("waitpid");
      break; // WARN: is this true
    }
    
    if (WIFEXITED(status)) {
      code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      code = 128 + WTERMSIG(status);
    } else if (WIFSTOPPED(status)) {
      code = 1;
      jt[jid].state = P_SUSPENDED;
      printf("\ntish: suspended  %s", getjid_pipe(jid)); 
      break;
    }
    
    if (--jt[jid].remaining == 0) break;
  }
  
  if (jt[jid].remaining == 0) {
    free(jt[jid].pipe);
  }

  if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) {
    perror("tcsetpgrp restore");
  }

  return code == 0 ? COMMAND_SUCCEEDED : COMMAND_FAILED; // go next or nah

NO_SUCH_JOB:
  fprintf(stderr, "tish: fg: %s: no such job\n", cmd->argv[1]);
  return COMMAND_FAILED;

NO_CURRENT_JOB:
  fprintf(stderr, "tish: fg: no current job\n");
  return COMMAND_FAILED;
}

builtin_command_t builtin_fg = {
  .name = "fg",
  .description = "move a job running in background to foreground",
  .handler = handler,
  .run_in_parent = 1
};
