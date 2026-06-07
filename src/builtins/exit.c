#include "exit.h"
#include "builtins.h"
#include "parser.h"
#include "history.h"
#include <stdlib.h>

static int handler(Command *cmd) {
  printf("Exiting tish...");
  append_to_shell_history();
  exit(0);
  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_exit = {
  .name = "exit",
  .description = "exit the shell",
  .handler = handler,
  .run_in_parent = 1
};
