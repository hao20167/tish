#ifndef BUILTINS_H
#define BUILTINS_H

enum { COMMAND_SUCESSED = 0, COMMAND_FAILED };

typedef struct builtin_command_t builtin_command_t;

struct builtin_command_t {
  char *name, *help;
  int (*run)();
};

extern builtin_command_t *builtin_cmds[];

#endif
