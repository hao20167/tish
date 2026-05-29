#ifndef HISTORY_H
#define HISTORY_H

#include "builtins.h"
#include "parser.h"

extern builtin_command_t builtin_history;

void init_shell_history();
void append_to_process_history(CommandList *cl);
void append_to_shell_history();

#endif
