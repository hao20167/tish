#include "builtins.h"
#include "cd.h"
#include "ls.h"
#include "exit.h"

builtin_command_t *builtin_commands[] = {&builtin_cd, &builtin_ls, &builtin_exit};
size_t n_builtin_commands = sizeof(builtin_commands) / sizeof(builtin_command_t*);
