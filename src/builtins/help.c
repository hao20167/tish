#include "help.h"
#include "builtins.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

static int handler(Command *cmd) {
  if (cmd->argc > 2) {
    fprintf(stderr, "tish: help: too many arguments\n");
    return COMMAND_FAILED;
  }

  if (cmd->argc == 1) {
    for (int i = 0; i < n_builtin_commands; i++) {
      printf("  %-10s %s\n", builtin_commands[i]->name, builtin_commands[i]->description);
    }
    printf("  %-10s %s\n", "clear", "clear the screen");
    return COMMAND_SUCCEEDED;
  }

  for (int i = 0; i < n_builtin_commands; i++) {
    if (strcmp(cmd->argv[1], builtin_commands[i]->name) == 0) {
      printf("%s: %s\n", builtin_commands[i]->name, builtin_commands[i]->description);
      return COMMAND_SUCCEEDED;
    }
  }

  fprintf(stderr, "tish: help: no help topics match `%s'\n", cmd->argv[1]);
  return COMMAND_FAILED;
}

builtin_command_t builtin_help = {
  .name = "help",
  .description = "show this help message",
  .handler = handler,
  .run_in_parent = 1
};
