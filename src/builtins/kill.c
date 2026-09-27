#include "kill.h"
#include "builtins.h"
#include "jobs.h"
#include "parser.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int is_number(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] > '9' || str[i] < '0')
      return 0;
  }
  return 1;
}

static int parse_signal(char *str) {
  if (str[0] != '-')
    return -1;
  str++;

  if (is_number(str))
    return atoi(str);
  if (strcmp(str, "TERM") == 0)
    return SIGTERM;
  if (strcmp(str, "KILL") == 0)
    return SIGKILL;
  if (strcmp(str, "STOP") == 0)
    return SIGSTOP;
  if (strcmp(str, "CONT") == 0)
    return SIGCONT;

  return -1;
}

static int parse_job(char *str) {
  char *num = str;
  int jid = -1;

  if (num[0] != '%' || strlen(num) > 4 + 1)
    return -1;
  num = num + 1;

  if (is_number(num)) {
    jid = atoi(num);
  } else {
    if (strcmp(num, "+") == 0) {
      jid = get_current_job();
    } else if (strcmp(num, "-") == 0) {
      jid = get_previous_job(get_current_job());
    } else
      return -1;
  }

  if (jid <= 0 || jid >= JMAX)
    return -1;
  if (getjid_remaining(jid) <= 0)
    return -1;

  return jid;
}

static char get_job_mark(int jid) {
  int current_job = get_current_job();
  int previous_job = get_previous_job(current_job);
  return jid == current_job ? '+' : (jid == previous_job ? '-' : ' ');
}

static void print_signal_status(int jid, int sig) {
  char mark = get_job_mark(jid);

  if (sig == SIGSTOP) {
    printf("[%d]  %c suspended (signal)  %s\n", jid, mark, jt[jid].pipe);
  } else if (sig == SIGCONT) {
    printf("[%d]  %c %-10s %s\n", jid, mark, "running", jt[jid].pipe);
  } else if (sig == SIGKILL) {
    printf("[%d]  %c %-10s %s\n", jid, mark, "killed", jt[jid].pipe);
  } else if (sig == SIGTERM) {
    printf("[%d]  %c %-10s %s\n", jid, mark, "terminated", jt[jid].pipe);
  } else {
    printf("[%d]  %c %-10s %s\n", jid, mark, "killed", jt[jid].pipe);
  }
}

static int handler(Command *cmd) {
  if (cmd->argc < 2) {
    fprintf(stderr, "tish: kill: not enough arguments\n");
    return COMMAND_FAILED;
  }
  if (cmd->argc > 3) {
    fprintf(stderr, "tish: kill: too many arguments\n");
    return COMMAND_FAILED;
  }

  int sig = SIGTERM;
  char *job_arg = cmd->argv[1];

  if (cmd->argc == 3) {
    sig = parse_signal(cmd->argv[1]);
    if (sig <= 0) {
      fprintf(stderr, "tish: kill: %s: invalid signal\n", cmd->argv[1]);
      return COMMAND_FAILED;
    }
    job_arg = cmd->argv[2];
  }

  int jid = parse_job(job_arg);
  if (jid < 0) {
    fprintf(stderr, "tish: kill: %s: no such job\n", job_arg);
    return COMMAND_FAILED;
  }

  pid_t pgid = jt[jid].pgid;
  if (kill(-pgid, sig) < 0) {
    perror("kill");
    return COMMAND_FAILED;
  }

  if (sig == SIGSTOP) {
    jt[jid].state = P_SUSPENDED;
  } else if (sig == SIGCONT) {
    jt[jid].state = P_RUNNING;
  } else if (sig == SIGKILL) {
    jt[jid].state = P_KILLED;
  } else if (sig == SIGTERM) {
    jt[jid].state = P_TERMINATED;
  }

  print_signal_status(jid, sig);

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_kill = {
  .name = "kill",
  .description = "send a signal to a job",
  .handler = handler,
  .run_in_parent = 1
};
