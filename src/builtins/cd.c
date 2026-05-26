#include "cd.h"
#include "builtins.h"
#include "parser.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

static int handler(Command *cmd) {
  size_t argc = cmd->argc;
  if (argc >= 3) return COMMAND_FAILED;

  char *target = NULL;

  char cwd[1024];
  getcwd(cwd, sizeof(cwd));

  if (argc == 1) {
    target = getenv("HOME");
  } else {
    if (strcmp(cmd->argv[1], "-") == 0) {
      target = getenv("OLDPWD");
      if (target == NULL) {
        fprintf(stderr, "tish: cd: OLDPWD not set\n");
        return COMMAND_FAILED;
      }
      printf("%s\n", target);
    } else {
      target = cmd->argv[1];
    }
  }

  if (chdir(target) != 0) {
    perror("tish: cd");
    return COMMAND_FAILED;
  }

  setenv("OLDPWD", cwd, 1);
  getcwd(cwd, sizeof(cwd));
  setenv("PWD", cwd, 1);

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_cd = {
  .name = "cd", 
  .description = "Go to the $(target) path", 
  .handler = handler,
  .run_in_parent = 1
};
