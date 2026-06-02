#include "tishtime.h"
#include "builtins.h"
#include "parser.h"
#include <stdio.h>
#include <time.h>

static int handler(Command *cmd) {
  if (cmd->argc > 1) {
    fprintf(stderr, "tish: time: too many arguments\n");
    return COMMAND_FAILED;
  }

  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  if (local == NULL) {
    perror("tish: time");
    return COMMAND_FAILED;
  }

  char buf[32];
  strftime(buf, sizeof(buf), "%H:%M:%S", local);
  printf("%s\n", buf);

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_time = {
  .name = "time",
  .description = "show current time",
  .handler = handler,
  .run_in_parent = 1
};
