#ifndef PARSER_H
#define PARSER_H

#include "utils.h"
#include "stdio.h"

extern const char* RedirStr[];
extern const size_t REDIR_LEN;

typedef enum {
  R_IN = 0,
  R_OUT,
  R_APPEND,
  R_OUT_BOTH,
  R_APPEND_BOTH,
  R_NONE
} RedirType;

typedef enum {
  S_SEMI = 0,   // ;
  S_AND,        // &&
  S_OR,         // ||
  S_BG,         // &
  S_NONE,
} Separator;

typedef enum {
  PARSER_FAILED = 0,
  PARSER_SUCCEEDED,
  PARSER_NONE
} ParserStatus;

// move this to somewhere else maybe
typedef struct Redirection Redirection;
typedef struct Command Command;
typedef struct Pipeline Pipeline;
typedef struct CommandList CommandList;
typedef struct Job Job; // TODO

extern const char* RedirStr[];
extern const char* SepStr[];
extern const size_t REDIR_LEN;
extern const size_t SEP_LEN;

struct Redirection {
  RedirType type;
  String target;
};

struct Command {
  char **argv;
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
  Separator sep; // could be &
};
// connection type to the next pipeline

// ls -l | grep ".c" && pwd ; echo "Done"
//
// Pipelines:
// ls -l | grep ".c"      GO_NEXT_IF_SUCCEEDED
// pwd                    GO_NEXT
// echo "Done"            END

struct CommandList {
  StringVec raw;
  Pipeline *pipes;
  size_t npipes, pipes_cap;
};

// struct Job {
//   Pipeline *pipes;
//   size_t npipes, pipes_cap;
// };

// void read_line(char **line);
ParserStatus parse_line(CommandList *out, char *line);
void print_pipeline(Pipeline *pipe);

#endif
