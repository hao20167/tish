#include "ls.h"
#include "builtins.h"
#include <stdio.h>

static int ls_run() {
  printf("lists:\ntemp1\ntemp2");
  return COMMAND_SUCESSED;
}

builtin_command_t builtin_ls = {.name = "ls",
                                .help = "list all the files/folders in path "
                                        "$(target)\n$(target) = cwd by default",
                                .run = ls_run};
