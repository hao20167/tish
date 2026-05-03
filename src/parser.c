// this takes forever
// #include "builtins.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_line(char **inp) {
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


static void append_arg_to_command(Command *cmd, String arg) {
  reserve_buffer((void**)(&cmd->argv), &cmd->argc, &cmd->argv_cap, 8, sizeof(String));
  (cmd->argv)[cmd->argc++] = arg;
}

static void append_redir_to_command(Command *cmd, Redirection redir) {
  reserve_buffer((void**)(&cmd->redirs), &cmd->nredirs, &cmd->redirs_cap, 4, sizeof(Redirection));
  (cmd->redirs)[(cmd->nredirs)++] = redir; 
}

static void append_command_to_pipeline(Pipeline *pipe, Command cmd) {
  reserve_buffer((void**)(&pipe->cmds), &pipe->ncmds, &pipe->cmds_cap, 8, sizeof(Command));
  (pipe->cmds)[(pipe->ncmds)++] = cmd;
}

static void append_pipeline_to_commandlist(CommandList *cmdlst, Pipeline pipe) {
  reserve_buffer((void**)(&cmdlst->pipes), &cmdlst->npipes, &cmdlst->pipes_cap, 8, sizeof(Pipeline));
  (cmdlst->pipes)[(cmdlst->npipes)++] = pipe;
}

static int is_redir_char(char c) {
  return c == '<' || c == '>' || c == '&'; 
}

// > list_code.txt
static ParserStatus parse_redirection(String redir, String target, Redirection *out) {
  RedirType r = R_NONE;
  for (int i = 0; i < REDIR_LEN; i++) {
    if (strcmp(RedirStr[i], redir.str) == 0) {
      r = i;
      break;
    }
  }
  if (r == R_NONE) return PARSER_FAILED;
  out->type = r;

  for (int i = 0; i < target.len; i++)
    if (is_redir_char(target.str[i]))
      return PARSER_FAILED;
  out->target = target;
  
  return PARSER_SUCESSED;
}

static void free_command(Command *cmd) {
  free(cmd->argv);
  free(cmd->redirs);
}

static int command_parser_failed(Command *out) {
  free_command(out);
  return PARSER_FAILED;
}

// ls -l
// grep ".c" > list_code.txt
static ParserStatus parse_command(StringVec *vec, Command *out) {
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

static void free_pipeline(Pipeline *pipe) {
  for (int i = 0; i < pipe->ncmds; i++)
    free_command(&pipe->cmds[i]);
  free(pipe->cmds);
}

static int pipeline_parser_failed(Pipeline *out) {
  free_pipeline(out);
  return PARSER_FAILED;
}

// ls -l | grep ".c" > list_code.txt
static ParserStatus parse_pipeline(StringVec *vec, Pipeline *out) { // TODO: change change `out` to Job*
  size_t len = vec->len;
  // TODO: review this, background or separator
  // if (strcmp(vec->arr[len - 1].str, "&") == 0) {
  //   out->background = 1;
  //   vec->len -= 1;
  // }

  int prev = -1;
  for (int i = 0; i < len; i++) {
    if (strcmp(vec->arr[i].str, "|") == 0) {
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
      ParserStatus status = parse_command(&cur, &now);
      free(cur.arr);

      if (status == PARSER_FAILED)
          return pipeline_parser_failed(out);

      append_command_to_pipeline(out, now);
    }
  }
  
  StringVec cur = {0};
  for (int i = prev + 1; i < len; i++) {
    append_string_to_stringvec(&cur, vec->arr[i]);
  }
  Command now = {0};
  ParserStatus status = parse_command(&cur, &now);
  free(cur.arr);

  if (status == PARSER_FAILED)
      return pipeline_parser_failed(out);

  append_command_to_pipeline(out, now);
  
  return PARSER_SUCESSED;
}

static void free_commandlist(CommandList *cmdlst) {
  for (int i = 0; i < cmdlst->npipes; i++)
    free_pipeline(&cmdlst->pipes[i]);
  free(cmdlst->pipes);
  free_stringvec(cmdlst->raw);
}

static int line_parser_failed(CommandList *out) {
  free_commandlist(out);
  return PARSER_FAILED;
}

static Separator get_sep_id(char *str) {
  for (int i = 0; i < SEP_LEN; i++) {
    if (strcmp(str, SepStr[i]) == 0)
        return i;
  }
  return S_NONE;
}

 ParserStatus parse_line(CommandList *out) {
  *out = (CommandList){0};
  char *line = NULL;
  read_line(&line);
  int len = strlen(line);
  
  StringVec vec = {0};
  for (int i = 0; i < len; i++) {
    if (line[i] == ' ') continue;
    String now = {0};

    int p = i - 1;
    while (p + 1 < len && line[p + 1] != ' ') {
      ++p;
      append_char_to_string(&now, line[p]);
    }

    append_string_to_stringvec(&vec, now);

    i = p;
  }
  free(line);

  out->raw = vec;
  
  int flag = 0; // if ended with a '&'
  int prev = -1, n = vec.len;
  for (int i = 0; i < n; i++) {
    Separator sep = get_sep_id(vec.arr[i].str);
    if (sep != S_NONE) {
      if (i == (int)n - 1) {
        if (sep == S_BG) {
          flag = 1;
        } else {
          return line_parser_failed(out); // TODO: need to be handled 
        }
      }
      if (prev == i - 1) return line_parser_failed(out);

      StringVec cur = {0};
      for (int j = prev + 1; j < i; j++) {
        append_string_to_stringvec(&cur, vec.arr[j]);
      }
      prev = i;
      Pipeline now = {0};
      ParserStatus status = parse_pipeline(&cur, &now);
      free(cur.arr);

      if (status == PARSER_FAILED)
          return line_parser_failed(out);
      now.sep = sep;

      append_pipeline_to_commandlist(out, now);
    }
  }
  
  if (flag == 0) {
    StringVec cur = {0};
    for (int i = prev + 1; i < n; i++) {
      append_string_to_stringvec(&cur, vec.arr[i]);
    }
    Pipeline now = {0};
    ParserStatus status = parse_pipeline(&cur, &now);
    free(cur.arr);

    if (status == PARSER_FAILED)
        return line_parser_failed(out);
    now.sep = S_SEMI;

    append_pipeline_to_commandlist(out, now);
  }
  
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

// TODO: ls -a>>out.txt is also valid, check token parsing
// TODO: future feature:
// ls|grep c
// echo hi>out.txt
// echo "hello world"
// echo 'a b c'
// echo foo\ bar
// ls -a>>out.txt
