#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"
#include "parser.h"
#include "builtins.h"


char* get_current_path() {
  return getcwd(NULL, 0);
}

void print_prompt() {
  printf("~%s\n", get_current_path());
  printf("❯ ");
  fflush(stdout);
}

void read_command(char **inp) {
  size_t sz = 0;
  ssize_t nread;
  do {
    nread = getline(inp, &sz, stdin);
    if (nread == -1) exit(EXIT_FAILURE);
    if (nread == 0) continue;
    (*inp)[nread - 1] = '\0';
    break;
  } while (1);
}

void print_error(char* err) {
  printf("tish: %s\n", err);
}

int main() {
  // while (1) {
  //   print_prompt();
  //   
  //   char *inp = NULL;
  //   read_command(&inp);
  //   
  //   // printf("tish: %s\n", inp);
  //   // for (int i = 0; i < strlen(inp); i++) {
  //   //   printf("%d ", inp[i]);
  //   // }
  //   // printf("\n");
  //   
  //   free(inp);
  //   printf("\n");
  // }


  char *str = NULL;
  read_command(&str);
  printf("> %s", str);

  int len = strlen(str);

  StringVec vec = {0};
  for (int i = 0; i < len; i++) {
    if (str[i] == ' ') continue;
    String now = {0};
    
    int p = i - 1;
    while (p + 1 < len && str[p + 1] != ' ') {
      ++p;
      append_char_to_string(&now, str[p]);
    }

    append_string_to_stringvec(&vec, now);

    i = p;
  }

  int n = vec.len;
  for (int i = 0; i < n; i++) {
    printf("%s\n", vec.arr[i].str);
  }

  Pipeline pipe = {0};
  parse_pipeline(&vec, &pipe);
  print_pipeline(&pipe);

  return 0;
}
