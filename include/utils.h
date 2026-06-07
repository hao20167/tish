#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define SWAP(x,y) do {   \
  typeof(x) _x = x;      \
  typeof(y) _y = y;      \
  x = _y;                \
  y = _x;                \
} while(0)

typedef struct String String;
typedef struct StringVec StringVec;

struct String {
  char *str;
  size_t len, cap;
};

struct StringVec {
  String *arr;
  size_t len, cap;
};

void free_string(String str);
void free_stringvec(StringVec vec);
void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
char *xstrdup(const char *str);
void reserve_buffer(void **buf, size_t *len, size_t *cap, size_t init, size_t sz);
void append_char_to_string(String *str, char c);
void append_string_to_stringvec(StringVec *vec, String str);

#endif
