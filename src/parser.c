// this takes forever
// #include "builtins.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StringVec StringVec;

struct String {
  char *str;
  size_t len, cap;
};

struct StringVec {
  String *arr;
  size_t len, cap;
};

const char* RedirStr[] = {
  "<",
  ">",
  ">>",
  ">&",
  ">>&"
};

struct Redirection {
  RedirType type;
  String target;
};

struct Command {
  String *argv;
  size_t argc, argv_cap;

  Redirection *redirs;
  size_t nredirs, redirs_cap;
};

// example: ls -l | grep ".c" > list_code.txt
// ls -l
// grep ...
struct Pipeline {
  Command *cmds; // separated by '|'
  size_t ncmds, cmds_cap;
  int background;
};

// TODO: struct Job --- multiple Pipelines
// ls -l | grep ".c" && pwd ; echo "Done"
//
// Pipelines:
// ls -l | grep ".c"      GO_NEXT_IF_SUCESSED
// pwd                    GO_NEXT
// echo "Done"            END

void read_command(char **inp) {
  size_t sz = 0;
  ssize_t nread;
  do {
    nread = getline(inp, &sz, stdin);
    if (nread == -1)
      exit(EXIT_FAILURE);
    if ((*inp)[nread - 1] == '\n')
      (*inp)[nread - 1] = '\0';
    break;
  } while (1);
}

static void *xmalloc(size_t size) {
  void *tmp = malloc(size);
  if (tmp == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

static void *xrealloc(void *buf, size_t size) {
  void *tmp = realloc(buf, size);
  if (tmp == NULL) {
    perror("realloc");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

// static char *xstrdup(const char *str) {
//   char *tmp = strdup(str);
//   if (tmp == NULL) {
//     perror("strdup");
//     exit(EXIT_FAILURE);
//   }
//   return tmp;
// }

// buf: pointer to the position of array
static void reserve_buffer(void **buf, size_t *len, size_t *cap, size_t init, size_t sz) {
  if (*cap == 0) {
    *buf = xmalloc(init * sz);
    *cap = init;
  }
  if (*len + 2 > *cap) {
    *buf = xrealloc(*buf, (*cap) * 2 * sz);
    (*cap) *= 2;
  }
}

static void append_char_to_string(String *str, char c) {
  reserve_buffer((void**)(&str->str), &str->len, &str->cap, 32, sizeof(char));
  (str->str)[str->len++] = c;
  (str->str)[str->len] = '\0';
}

static void append_string_to_stringvec(StringVec *vec, String str) {
  reserve_buffer((void**)(&vec->arr), &vec->len, &vec->cap, 16, sizeof(String));
  (vec->arr)[vec->len++] = str;
  // TODO: end element? NULL cannot be used
}

static void append_arg_to_command(Command *cmd, String arg) {
  reserve_buffer((void**)(&cmd->argv), &cmd->argc, &cmd->argv_cap, 8, sizeof(String));
  (cmd->argv)[cmd->argc++] = arg;
  // TODO: end element? NULL cannot be used
}

static void append_redir_to_command(Command *cmd, Redirection redir) {
  reserve_buffer((void**)(&cmd->redirs), &cmd->nredirs, &cmd->redirs_cap, 4, sizeof(Redirection));
  // o(1) -- redirs[id] = redir
  // redirs[id].type = redir.type
  // redirs[id].target = redir.target
  (cmd->redirs)[(cmd->nredirs)++] = redir; 
  // TODO: end element? NULL cannot be used
}

static void append_command_to_pipeline(Pipeline *pipe, Command cmd) {
  reserve_buffer((void**)(&pipe->cmds), &pipe->ncmds, &pipe->cmds_cap, 8, sizeof(Command));
  // o(1) -- cmds[id] = cmd
  (pipe->cmds)[(pipe->ncmds)++] = cmd;
  // TODO: end element? NULL cannot be used
}

// TODO: APPEND pipeline to job
// static void append_


static int is_redir_char(char c) {
  return c == '<' || c == '>' || c == '&'; 
}

// > list_code.txt
static int parse_redirection(String redir, String target, Redirection *out) {
  RedirType r = R_END;
  for (int i = 0; i < REDIR_LEN; i++) {
    if (strcmp(RedirStr[i], redir.str) == 0) {
      r = i;
    }
  }
  if (r == R_END) return PARSER_FAILED;
  out->type = r;

  for (int i = 0; i < target.len; i++)
    if (is_redir_char(target.str[i]))
      return PARSER_FAILED;
  out->target = target;
  
  return PARSER_SUCESSED;
}

static int command_parser_failed(Command *out) {
  free(out->argv);
  free(out->redirs);
  return PARSER_FAILED;
}

// ls -l
// grep ".c" > list_code.txt
static int parse_command(StringVec *vec, Command *out) {
  size_t len = vec->len;
  int p = -1; // arg's last position
  for (int i = 0; i < len; i++) {
    if (is_redir_char(vec->arr[i].str[0])) break;
    p = i;
    append_arg_to_command(out, vec->arr[i]);
  }
  for (int i = p + 1; i < len; i += 2) {
    if (i + 1 >= len) return command_parser_failed(out);
    if (!is_redir_char(vec->arr[i].str[0]) || is_redir_char(vec->arr[i + 1].str[0])) 
      return command_parser_failed(out);
    Redirection redir;
    if (parse_redirection(vec->arr[i], vec->arr[i + 1], &redir) == PARSER_FAILED) {
      return command_parser_failed(out);
    }
    append_redir_to_command(out, redir);
  }
  return PARSER_SUCESSED;
}

static int pipeline_parser_failed(Pipeline *out) {
  free(out->cmds);
  return PARSER_FAILED;
}

// ls -l | grep ".c" > list_code.txt
int parse_pipeline(StringVec *vec, Pipeline *out) { // TODO: change change `out` to Job*
  size_t len = vec->len;
  int prev = -1;
  for (int i = 0; i < len; i++) {
    if (vec->arr[i].str[0] == '|') {
      if (i == (int)len - 1) {
        return pipeline_parser_failed(out); // TODO: need to be handled 
      }
      if (prev == i - 1) return pipeline_parser_failed(out);

      StringVec cur = {0};
      for (int j = prev + 1; j < i; j++) {
        append_string_to_stringvec(&cur, vec->arr[j]);
      }
      prev = i;
      Command now = {0};
      if (parse_command(&cur, &now) == PARSER_FAILED)
          return pipeline_parser_failed(out);

      append_command_to_pipeline(out, now);
    }
  }
  
  StringVec cur = {0};
  for (int i = prev + 1; i < len; i++) {
    append_string_to_stringvec(&cur, vec->arr[i]);
  }
  Command now = {0};
  if (parse_command(&cur, &now) == PARSER_FAILED)
      return pipeline_parser_failed(out);

  append_command_to_pipeline(out, now);
  
  return PARSER_SUCESSED;
}

static void print_redirection(Redirection *redir) {
  printf("redirection -> [type = %s, target = %s]", RedirStr[redir->type], redir->target.str);
}

static void print_command(Command *cmd) {
  printf("command -> []\n");
  printf("argv: [");
  for (int i = 0; i < cmd->argc; i++)
    printf("%s, ", cmd->argv[i].str);
  printf("]\nredirs[\n");
  for (int i = 0; i < cmd->nredirs; i++) {
    print_redirection(&cmd->redirs[i]);
    printf("\n");
  }
  printf("]\n");
}

static void print_pipeline(Pipeline *pipe) {
  printf("pipeline -> [[]]\n");
  for (int i = 0; i < pipe->ncmds; i++) {
    print_command(&pipe->cmds[i]);
    printf("\n");
  }
  printf("\n");
}

// WARN: for testing only
// int main() {
//   char *str = NULL;
//   read_command(&str);
//   printf("> %s", str);

//   int len = strlen(str);

//   StringVec vec = {0};
//   for (int i = 0; i < len; i++) {
//     if (str[i] == ' ') continue;
//     String now = {0};
//     
//     int p = i - 1;
//     while (p + 1 < len && str[p + 1] != ' ') {
//       ++p;
//       append_char_to_string(&now, str[p]);
//     }

//     append_string_to_stringvec(&vec, now);

//     i = p;
//   }

//   int n = vec.len;
//   for (int i = 0; i < n; i++) {
//     printf("%s\n", vec.arr[i].str);
//   }

//   Pipeline pipe = {0};
//   parse_pipeline(&vec, &pipe);
//   print_pipeline(&pipe);

//   return 0;
// }
