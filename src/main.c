#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


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
  while (1) {
    print_prompt();
    
    char *inp = NULL;
    read_command(&inp);
    
    // printf("tish: %s\n", inp);
    // for (int i = 0; i < strlen(inp); i++) {
    //   printf("%d ", inp[i]);
    // }
    // printf("\n");
    
    free(inp);
    printf("\n");
  }

  return 0;
}
