#include "builtins.h"
#include "cd.h"
#include "ls.h"
#include <stdio.h>

builtin_command_t *builtin_cmds[] = {&builtin_cd, &builtin_ls, NULL};
