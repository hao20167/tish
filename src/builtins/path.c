#include "path.h"
#include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char *HOME;

char *tish_path_file = NULL;
char *pPATH = ".tishpath";
StringVec tish_paths = {0};

static int has_tish_path(char *path) {
  for (int i = 0; i < tish_paths.len; i++) {
    if (strcmp(tish_paths.arr[i].str, path) == 0)
      return 1;
  }
  return 0;
}

static void append_to_tish_paths(char *path) {
  String now = {
    .str = xstrdup(path),
    .len = strlen(path),
    .cap = strlen(path) + 1
  };
  append_string_to_stringvec(&tish_paths, now);
}

void init_tish_path() {
  tish_path_file = xmalloc(sizeof(char) * (strlen(HOME) + strlen(pPATH) + 2));
  strcpy(tish_path_file, HOME);
  strcat(tish_path_file, "/");
  strcat(tish_path_file, pPATH);

  FILE *shell = fopen(tish_path_file, "a+");
  if (shell == NULL) {
    perror("fopen tish_path_file to read");
    exit(EXIT_FAILURE);
  }

  char *tmp = NULL;
  size_t sz = 0;
  ssize_t len;
  rewind(shell);
  while ((len = getline(&tmp, &sz, shell)) > 0) {
    if (tmp[len - 1] == '\n') {
      tmp[--len] = '\0';
    }
    if (len == 0 || has_tish_path(tmp)) continue;
    append_to_tish_paths(tmp);
  }

  free(tmp);
  fclose(shell);
}

int add_tish_path(char *path) {
  char *target = path;
  char *expanded = NULL;
  if (strcmp(path, "~") == 0) {
    target = HOME;
  } else if (strncmp(path, "~/", 2) == 0) {
    expanded = xmalloc(sizeof(char) * (strlen(path) + strlen(HOME)));
    strcpy(expanded, HOME);
    strcat(expanded, path + 1);
    target = expanded;
  }

  char *resolved = realpath(target, NULL);
  free(expanded);
  if (resolved == NULL)
    return -1;

  struct stat st;
  if (stat(resolved, &st) < 0) {
    free(resolved);
    return -1;
  }
  if (!S_ISDIR(st.st_mode)) {
    free(resolved);
    errno = ENOTDIR;
    return -1;
  }

  if (has_tish_path(resolved)) {
    free(resolved);
    return 0;
  }

  FILE *shell = fopen(tish_path_file, "a");
  if (shell == NULL) {
    free(resolved);
    return -1;
  }
  fprintf(shell, "%s\n", resolved);
  fclose(shell);

  append_to_tish_paths(resolved);
  free(resolved);
  return 0;
}

void exec_tish_path(char *name, char **argv) {
  if (strchr(name, '/') != NULL) {
    errno = ENOENT;
    return;
  }

  int saved_errno = ENOENT;
  for (int i = 0; i < tish_paths.len; i++) {
    char *dir = tish_paths.arr[i].str;
    char *target = xmalloc(sizeof(char) * (strlen(dir) + strlen(name) + 2));
    strcpy(target, dir);
    strcat(target, "/");
    strcat(target, name);

    execv(target, argv);
    if (errno == EACCES) saved_errno = EACCES;
    free(target);
  }
  errno = saved_errno;
}

static int handler(Command *cmd) {
  if (cmd->argc > 1) {
    fprintf(stderr, "tish: path: too many arguments\n");
    return COMMAND_FAILED;
  }

  char *global_path = getenv("PATH");
  printf("global:\n");
  if (global_path != NULL)
    printf("%s\n", global_path);

  printf("\ntish:\n");
  for (int i = 0; i < tish_paths.len; i++) {
    printf("%s\n", tish_paths.arr[i].str);
  }

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_path = {
  .name = "path",
  .description = "show global path and tish path",
  .handler = handler,
  .run_in_parent = 1
};
