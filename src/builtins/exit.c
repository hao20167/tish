#include "exit.h"
#include "builtins.h"
#include "parser.h"
#include <stdlib.h>

static int handler(Command *cmd) {
  printf("Exiting tish...");
  getchar();
  exit(0);
  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_exit = {
  .name = "exit",
  .description = "exit the shell",
  .handler = handler,
  .run_in_parent = 1
};
