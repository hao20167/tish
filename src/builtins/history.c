#include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>


extern char *HOME;
char *history_path = NULL;
char *hPATH = ".tish_history";

StringVec shell_history = {0};
StringVec process_history = {0};

void init_shell_history() {
  history_path = xmalloc(sizeof(char) * (strlen(HOME) + strlen(hPATH) + 2));
  strcpy(history_path, HOME);
  strcat(history_path, "/");
  strcat(history_path, hPATH);

  FILE *shell = fopen(history_path, "a+");
  if (shell == NULL) {
    perror("fopen history_path to read");
    exit(EXIT_FAILURE);
  }
  
  char *tmp = NULL;
  size_t sz = 0;
  ssize_t len;
  rewind(shell);
  while ((len = getline(&tmp, &sz, shell)) > 0) { 
    if (len == 1) continue;
    // last chr is "\n"
    len -= 1;
    tmp[len] = '\0';

    String now = {0};
    now.cap = len;
    now.len = len;
    now.str = xstrdup(tmp);

    append_string_to_stringvec(&shell_history, now);
  }

  free(tmp);
  fclose(shell);
}

// function that connect all the String in StringVec into 1 String, separated by ' '
static String cat(StringVec vec, size_t n) {
  if (n == 0) {
    return (String){ .str = xstrdup(""), .len = 0, .cap = 1 };
  }
  String ans = {0};
  size_t sz = 0;
  for (int i = 0; i < n; i++) {
    sz += vec.arr[i].len + 1;
  }

  ans.str = xmalloc(sizeof(char) * sz);
  ans.len = sz - 1;
  ans.cap = sz;
  ans.str[sz - 1] = '\0';

  for (int i = 0, p = 0; i < n; i++) {
    char* now = vec.arr[i].str;
    int len = strlen(now);
    for (int j = 0; j < len; j++) {
      ans.str[p++] = now[j];
    }
    if (i < n - 1) ans.str[p++] = ' ';
    if (i == n - 1) assert(p == sz - 1);
  }

  return ans;
}

void append_to_process_history(CommandList *cl) {
  if (cl->raw.len == 0) return;
  String now = cat(cl->raw, cl->raw.len);
  append_string_to_stringvec(&process_history, now);
}

void append_to_shell_history() {
  FILE *shell = fopen(history_path, "a");
  if (shell == NULL) {
    perror("fopen history_path to write");
    exit(1);
  }
  for (int i = 0; i < process_history.len; i++) {
    fprintf(shell, "%s\n", process_history.arr[i].str);
  }
  fclose(shell);
}

static int max(int x, int y) {
  return x > y ? x : y;
}

static int min(int x, int y) {
  return x < y ? x : y;
}

static int strtoi(char *str) {
  int len = strlen(str);
  if (len > 8) return -1;
  int res = 0;
  for (int i = 0; i < len; i++) {
    if (str[i] == '-' || !isdigit(str[i])) return -1;
    res = res * 10 + ((int)str[i] - '0');
  }
  return res;
}

// TODO: support `history 500 400`
static int handler(Command *cmd) {
  if (cmd->argc > 3) {
    fprintf(stderr, "tish: history: too many arguments\n");
    return COMMAND_FAILED;
  }
  int nproc = process_history.len, nshell = shell_history.len, nall = nshell + nproc;
  int l = max(nall - 16, 1), r = nall - 1;

  if (cmd->argc == 2) {
    l = strtoi(cmd->argv[1]);
    if (l <= 0 || l > nall) {
      fprintf(stderr, "tish: history: no such event: %d\n", l);
      return COMMAND_FAILED;
    }
  } else if (cmd->argc == 3) {
    l = strtoi(cmd->argv[1]);
    int r_in = strtoi(cmd->argv[2]);
    if (l <= 0 || r_in <= 0) {
      fprintf(stderr, "tish: history: numeric arguments must be positive\n");
      return COMMAND_FAILED;
    }
    if (l > nall) {
      fprintf(stderr, "tish: history: no events in that range\n");
      return COMMAND_FAILED;
    }
    r = min(r_in, nall);
    if (l > r) {
      fprintf(stderr, "tish: history: no events in that range\n");
      return COMMAND_FAILED;
    }
  }
  
  if (l <= nshell) {
    for (int i = l; i <= min(r, nshell); i++) {
      printf("%5d  %s\n", i, shell_history.arr[i - 1].str);
    }
  }
  if (r > nshell) {
    for (int i = max(nshell + 1, l); i <= min(r, nall); i++) {
      printf("%5d  %s\n", i, process_history.arr[i - nshell - 1].str);
    }
  }

  return COMMAND_SUCCEEDED;
}

builtin_command_t builtin_history = {
  .name = "history",
  .description = "show history",
  .handler = handler,
  .run_in_parent = 1
};
