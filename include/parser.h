#ifndef PARSER_H
#define PARSER_H

#include "stdio.h"

#define REDIR_LEN 5

extern const char* RedirStr[];

typedef enum {
  R_IN = 0,
  R_OUT,
  R_APPEND,
  R_OUT_BOTH,
  R_APPEND_BOTH,
  R_END
} RedirType;

enum {
  PARSER_FAILED = 0,
  PARSER_SUCESSED,
  PARSER_END
};

typedef struct String String;

typedef struct Redirection Redirection;

typedef struct Command Command;

typedef struct Pipeline Pipeline;

typedef struct Job Job; // TODO

int parse_line(const char *str, Pipeline *out);

#endif
