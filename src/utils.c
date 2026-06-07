#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_string(String str) {
  free(str.str);
}

void free_stringvec(StringVec vec) {
  for (int i = 0; i < vec.len; i++)
    free_string(vec.arr[i]);
  free(vec.arr);
}

void *xmalloc(size_t size) {
  void *tmp = malloc(size);
  if (tmp == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

void *xrealloc(void *buf, size_t size) {
  void *tmp = realloc(buf, size);
  if (tmp == NULL) {
    perror("realloc");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

char *xstrdup(const char *str) {
  char *tmp = strdup(str);
  if (tmp == NULL) {
    perror("strdup");
    exit(EXIT_FAILURE);
  }
  return tmp;
}

// buf: pointer to the position of array
void reserve_buffer(void **buf, size_t *len, size_t *cap, size_t init, size_t sz) {
  if (*cap == 0) {
    *buf = xmalloc(init * sz);
    *cap = init;
  }
  if (*len + 2 > *cap) {
    *buf = xrealloc(*buf, (*cap) * 2 * sz);
    (*cap) *= 2;
  }
}

void append_char_to_string(String *str, char c) {
  reserve_buffer((void**)(&str->str), &str->len, &str->cap, 32, sizeof(char));
  (str->str)[str->len++] = c;
  (str->str)[str->len] = '\0';
}

void append_string_to_stringvec(StringVec *vec, String str) {
  reserve_buffer((void**)(&vec->arr), &vec->len, &vec->cap, 16, sizeof(String));
  (vec->arr)[vec->len++] = str;
}
