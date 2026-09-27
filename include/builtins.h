#ifndef BUILTINS_H
#define BUILTINS_H

#include <stddef.h>

typedef struct Command Command;

typedef enum { 
  COMMAND_SUCCEEDED = 0, 
  COMMAND_FAILED,
  COMMAND_NONE
} CommandStatus;

typedef struct builtin_command_t builtin_command_t;

struct builtin_command_t {
  char *name, *description;
  int (*handler)(Command *);
  int run_in_parent;
};

extern builtin_command_t *builtin_commands[];
extern size_t n_builtin_commands;

#endif
