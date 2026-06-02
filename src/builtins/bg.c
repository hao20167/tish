#include "bg.h"
#include "builtins.h"
#include "jobs.h"
#include "parser.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <unistd.h>

static int is_number(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] > '9' || str[i] < '0')
      return 0;
  }
  return 1;
}

static int handler(Command *cmd) {
  // resume a background job
  if (cmd->argc > 2) {
    fprintf(stderr, "tish: bg: too many arguments\n");
    return COMMAND_FAILED;
  }
  int jid = -1;
  if (cmd->argc == 1 || (cmd->argc == 2 && strcmp(cmd->argv[1], "%") == 0)) {
    jid = get_current_job();
    if (getjid_remaining(jid) <= 0)
      goto NO_CURRENT_JOB;
  } else if (cmd->argc == 2) {
    char *num = cmd->argv[1];
    if (num[0] != '%' || strlen(num) > 4 + 1)
      goto NO_SUCH_JOB;
    num = num + 1;
    if (is_number(num)) {
      jid = atoi(num);
    } else {
      if (strcmp(num, "+") == 0) {
        jid = get_current_job();
      } else if (strcmp(num, "-") == 0) {
        jid = get_previous_job(get_current_job());
      } else
        goto NO_SUCH_JOB;
    }
    if (jid <= 0 || jid >= JMAX)
      goto NO_SUCH_JOB;
    if (getjid_remaining(jid) <= 0)
      goto NO_SUCH_JOB;
  }

  if (jt[jid].state == P_RUNNING) {
    fprintf(stderr, "tish: bg: job already in background\n");
    return COMMAND_FAILED;
  }

  pid_t pgid = jt[jid].pgid;

  if (kill(-pgid, SIGCONT) < 0) {
    perror("kill SIGCONT");
    return COMMAND_FAILED;
  }

  jt[jid].state = P_RUNNING;
  printf("[%d]    %9s %s\n", jid, ProcStatus[P_CONTINUED], jt[jid].pipe);

  return COMMAND_SUCCEEDED;

NO_SUCH_JOB:
  fprintf(stderr, "tish: bg: %s: no such job\n", cmd->argv[1]);
  return COMMAND_FAILED;

NO_CURRENT_JOB:
  fprintf(stderr, "tish: bg: no current job\n");
  return COMMAND_FAILED;
}

builtin_command_t builtin_bg = {
    .name = "bg",
    .description = "resume a background process (in background)",
    .handler = handler,
    .run_in_parent = 1};
