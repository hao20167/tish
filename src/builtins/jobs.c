#include "jobs.h"
#include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

const char* ProcStatus[] = {
  "running",
  "suspended",
  "done",
  "exit",
  "terminated",
  "killed",
  "continued"
};

const size_t PS_LEN = sizeof(ProcStatus) / sizeof(char*);

int timer = 0; // WARN: overflow
Job jt[JMAX]; // TODO: 

static char* cat(Pipeline *pipe) {
  size_t sz = 0;
  for (int i = 0; i < pipe->ncmds; i++) {
    if (i > 0) ++sz;
    Command cmd = pipe->cmds[i];
    for (int j = 0; j < cmd.argc; j++) {
      sz += strlen(cmd.argv[j]) + 1; 
    }
    sz++; // '|'
  }
  sz--;

  char *res = xmalloc(sz);
  res[sz - 1] = '\0';

  for (int i = 0, p = 0; i < pipe->ncmds; i++) {
    if (i > 0) res[p++] = ' ';
    Command cmd = pipe->cmds[i];
    for (int j = 0; j < cmd.argc; j++) {
      char *str = cmd.argv[j];
      int len = strlen(str);
      for (int t = 0; t < len; t++) {
        res[p++] = str[t];
      }
      if (i == pipe->ncmds - 1 && j == cmd.argc - 1) continue;
      res[p++] = ' ';
    }
    if (i < pipe->ncmds - 1) res[p++] = '|';
    if (i == pipe->ncmds - 1) {
      assert(p == sz - 1);
    }
  }

  return res;
}

pid_t _pgid[PMAX];

// one more proc of gpid has done
// return jid if job will be ended after that
int getjid_if_done(pid_t pid) {
  pid_t pgid = _pgid[pid];
  if (pgid < 1) return -1; // in case not initalized
  for (int i = 1; i < JMAX; i++) {
    if (jt[i].pgid == pgid) {
      jt[i].remaining--;
      if (jt[i].remaining == 0) {
        return i;
      }
      break;
    }
  }
  return -1;
}

// nproc: remaining number of procs
int add_pipeline_to_jobtable(Pipeline *pipe, pid_t pgid, int nproc, ProcState state) {
  Job job = {
    .pgid = pgid,
    .pipe = cat(pipe),
    .state = state, // bg: P_RUNNING; suspended: P_SUSPENDED
    .remaining = nproc,
    .created_at = ++timer
  };
  for (int i = 1; i < JMAX; i++) {
    if (jt[i].remaining == 0) {
      jt[i] = job;
      return i;
    }
  }
  fprintf(stderr, "tish: jobtable is full!!!");
  return 0;
}

char* getjid_pipe(int jid) {
  return jt[jid].pipe;
}

int getjid_remaining(int jid) {
  if (jid < 0 || jid >= JMAX) return -1;
  return jt[jid].remaining;
}

// WARN: both cur&pre jobs arent quite the same as those in zsh
int get_current_job() {
  assert(jt[0].created_at == 0);
  int ans = 0;
  for (int i = 1; i < JMAX; i++) {
    if (jt[i].remaining == 0) continue;
    if (jt[i].created_at > jt[ans].created_at) {
      ans = i;
    }
  }
  return ans;
}

// WARN:
int get_previous_job(int current_job) {
  if (current_job == -1) { // not given
    current_job = get_current_job();
  }
  if (current_job <= 0) return 0;

  assert(jt[0].created_at == 0);
  int ans = 0;
  for (int i = 1; i < JMAX; i++) {
    if (jt[i].remaining == 0) continue;
    if (i == current_job) continue;
    if (jt[i].created_at > jt[ans].created_at) {
      ans = i;
    }
  }
  return ans;
}

static int handler(Command *cmd) {
  int current_job = get_current_job();
  int previous_job = get_previous_job(current_job);
  for (int i = 1; i < JMAX; i++) {
    if (jt[i].remaining > 0) {
      char mark = (i == current_job ? '+' : (i == previous_job ? '-' : ' '));
      printf("[%d]  %c %9s %s\n", i, mark, ProcStatus[jt[i].state], jt[i].pipe);
    }
  }
  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_jobs = {
  .name = "jobs",
  .description = "print all the jobs",
  .handler = handler,
  .run_in_parent = 1
};
