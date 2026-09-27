#include "date.h"
#include "builtins.h"
#include "parser.h"
#include <stdio.h>
#include <time.h>

static int handler(Command *cmd) {
  if (cmd->argc > 1) {
    fprintf(stderr, "tish: date: too many arguments\n");
    return COMMAND_FAILED;
  }

  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  if (local == NULL) {
    perror("tish: date");
    return COMMAND_FAILED;
  }

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d", local);
  printf("%s\n", buf);

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_date = {
  .name = "date",
  .description = "show current date",
  .handler = handler,
  .run_in_parent = 1
};
