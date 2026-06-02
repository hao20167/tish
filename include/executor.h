#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

int exec_line(char *line);
int exec_commandlist(CommandList *cl);

#endif
