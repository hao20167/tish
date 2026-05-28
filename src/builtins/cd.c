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

  char *target = NULL, *HOME = getenv("HOME");
  if (HOME == NULL) {
    fprintf(stderr, "tish: cd: HOME not set\n");
    return COMMAND_FAILED;
  }

  char cwd[1024];
  getcwd(cwd, sizeof(cwd));

  int flag = 0; // free

  if (argc == 1 || (argc == 2 && strcmp(cmd->argv[1], "~") == 0)) {
    target = HOME;
  } else {
    if (strcmp(cmd->argv[1], "-") == 0) {
      target = getenv("OLDPWD");
      if (target == NULL) {
        fprintf(stderr, "tish: cd: OLDPWD not set\n");
        return COMMAND_FAILED;
      }
    } else {
      target = cmd->argv[1];
      if (strncmp(target, "~/", 2) == 0) {
        char *new_target = xmalloc(sizeof(char) * (strlen(target) + strlen(HOME)));
        strcpy(new_target, HOME);
        target = target + 1;
        strcat(new_target, target);
        target = new_target;
        flag = 1;
      }
    }
  }

  if (chdir(target) != 0) {
    if (flag) free(target);
    perror("tish: cd");
    return COMMAND_FAILED;
  }
  if (flag) free(target);

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
