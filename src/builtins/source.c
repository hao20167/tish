#include "source.h"
#include "builtins.h"
#include "executor.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *HOME;

static int should_skip(char *line) {
  while (*line == ' ') line++;
  return line[0] == '\0' || line[0] == '#';
}

static int handler(Command *cmd) {
  if (cmd->argc < 2) {
    fprintf(stderr, "tish: source: not enough arguments\n");
    return COMMAND_FAILED;
  }
  if (cmd->argc > 3) {
    fprintf(stderr, "tish: source: too many arguments\n");
    return COMMAND_FAILED;
  }

  int verbose = 0;
  char *target = cmd->argv[1];
  if (target[0] == '-') {
    if (strcmp(target, "-v") != 0) {
      fprintf(stderr, "tish: source: %s: invalid option\n", target);
      return COMMAND_FAILED;
    }
    if (cmd->argc < 3) {
      fprintf(stderr, "tish: source: not enough arguments\n");
      return COMMAND_FAILED;
    }
    verbose = 1;
    target = cmd->argv[2];
  } else if (cmd->argc > 2) {
    fprintf(stderr, "tish: source: too many arguments\n");
    return COMMAND_FAILED;
  }

  char *expanded = NULL;
  if (strcmp(target, "~") == 0) {
    target = HOME;
  } else if (strncmp(target, "~/", 2) == 0) {
    expanded = xmalloc(sizeof(char) * (strlen(target) + strlen(HOME)));
    strcpy(expanded, HOME);
    strcat(expanded, target + 1);
    target = expanded;
  }

  FILE *file = fopen(target, "r");
  if (file == NULL) {
    free(expanded);
    perror("tish: source");
    return COMMAND_FAILED;
  }

  char *line = NULL;
  size_t sz = 0;
  ssize_t len;
  int code = COMMAND_SUCCEEDED;
  while ((len = getline(&line, &sz, file)) > 0) {
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[--len] = '\0';
    }
    if (should_skip(line)) continue;
    if (verbose) {
      char *cwd = getcwd(NULL, 0);
      if (cwd == NULL) {
        perror("tish: source");
        code = COMMAND_FAILED;
      } else {
        printf("[%s] + %s\n", cwd, line);
        free(cwd);
      }
      fflush(stdout);
    }
    if (exec_line(xstrdup(line)) != COMMAND_SUCCEEDED)
      code = COMMAND_FAILED;
  }

  free(line);
  free(expanded);
  fclose(file);
  return code;
}

builtin_command_t builtin_source = {
  .name = "source",
  .description = "execute commands from a file",
  .handler = handler,
  .run_in_parent = 1
};
