#ifndef PATH_H
#define PATH_H

#include "builtins.h"

extern builtin_command_t builtin_path;

void init_tish_path();
int add_tish_path(char *path);
void exec_tish_path(char *name, char **argv);

#endif
