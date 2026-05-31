#ifndef JOBS_H
#define JOBS_H

#include "builtins.h"
#include "parser.h"
#include <sys/types.h>

#define PMAX 100001

typedef enum {
  P_RUNNING = 0,
  P_SUSPENDED,
  P_DONE,
  P_EXIT,
  P_TERMINATED,
  P_KILLED,
  P_CONTINUED,
  P_NONE
} ProcState;

extern pid_t _pgid[PMAX];
extern builtin_command_t builtin_jobs;

int getjid_if_done(pid_t pid);
char* getjid_pipe(int jid);
int get_current_job();
int get_previous_job(int current_job);
int add_pipeline_to_jobtable(Pipeline *pipe, pid_t pgid, int nproc, ProcState state);

#endif
