#include "exit.h"
#include "builtins.h"
#include "parser.h"

static int handler(Command *cmd) {
  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_exit = {
  .name = "exit",
  .description = "exit the shell",
  .handler = handler,
  .run_in_parent = 1
};
