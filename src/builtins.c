#include "builtins.h"
#include "bg.h"
#include "cd.h"
#include "ls.h"
#include "exit.h"
#include "history.h"
#include "jobs.h"
#include "fg.h"
#include "kill.h"
#include "path.h"
#include "addpath.h"
#include "source.h"
#include "date.h"
#include "tishtime.h"
#include "help.h"

builtin_command_t *builtin_commands[] = {&builtin_help, &builtin_cd, &builtin_ls, &builtin_exit, &builtin_history, &builtin_jobs, &builtin_fg, &builtin_bg, &builtin_kill, &builtin_path, &builtin_addpath, &builtin_source, &builtin_date, &builtin_time};
size_t n_builtin_commands = sizeof(builtin_commands) / sizeof(builtin_command_t*);
