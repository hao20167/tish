#include "addpath.h"
#include "builtins.h"
#include "parser.h"
#include "path.h"
#include <stdio.h>

static int handler(Command *cmd) {
  if (cmd->argc < 2) {
    fprintf(stderr, "tish: addpath: not enough arguments\n");
    return COMMAND_FAILED;
  }
  if (cmd->argc > 2) {
    fprintf(stderr, "tish: addpath: too many arguments\n");
    return COMMAND_FAILED;
  }

  if (add_tish_path(cmd->argv[1]) < 0) {
    perror("tish: addpath");
    return COMMAND_FAILED;
  }

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_addpath = {
  .name = "addpath",
  .description = "add a path for tish",
  .handler = handler,
  .run_in_parent = 1
};
