#ifndef BUILTINS_H
#define BUILTINS_H

typedef enum { 
  COMMAND_SUCESSED = 0, 
  COMMAND_FAILED,
  COMMAND_NONE
} CommandStatus;

typedef struct builtin_command_t builtin_command_t;

struct builtin_command_t {
  char *name, *description;
  int (*handler)();
};

extern builtin_command_t *builtin_commands[];

#endif
