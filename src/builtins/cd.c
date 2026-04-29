#include "cd.h"
#include "builtins.h"
#include <stdio.h>

static int cd_run(char *target) {
  printf("cd to %s\n", target);
  return COMMAND_SUCESSED;
}

builtin_command_t builtin_cd = {
    .name = "cd", .help = "Go to the $(target) path", .run = cd_run};
