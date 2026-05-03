#include "cd.h"
#include "builtins.h"
#include "parser.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

static int handler(FILE *out, Command *cmd) {
  if (cmd->argc >= 2) return COMMAND_FAILED;
  if (out == NULL) out = stdout;

  char *target = NULL;

  char cwd[1024];
  getcwd(cwd, sizeof(cwd));

  if (cmd->argc == 0) {
    target = getenv("HOME");
  } else {
    if (strcmp(cmd->argv[0].str, "-") == 0) {
      target = getenv("OLDPWD");
      if (target == NULL) {
        fprintf(stderr, "tish: cd: OLDPWD not set\n");
        return COMMAND_FAILED;
      }
      fprintf(out, "%s\n", target);
    } else {
      target = cmd->argv[0].str;
    }
  }

  if (chdir(target) != 0) {
    perror("tish: cd");
    return COMMAND_FAILED;
  }

  setenv("OLDPWD", cwd, 1);
  getcwd(cwd, sizeof(cwd));
  setenv("PWD", cwd, 1);

  return COMMAND_SUCESSED;
}

builtin_command_t builtin_cd = {
    .name = "cd", .description = "Go to the $(target) path", .handler = handler
};
