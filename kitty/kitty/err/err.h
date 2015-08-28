#ifndef DOSSIER_ERR_H
#define DOSSIER_ERR_H

#include <string>
#include <kitty/util/thread_local.h>

namespace err {
/*
 * Thread safe error functions
 */
const char *current();

typedef enum {
  OK,
  TIMEOUT,
  BREAK, // Break from eachByte()
  FILE_CLOSED,
  OUT_OF_BOUNDS,
  INPUT_OUTPUT,
  UNAUTHORIZED,
  LIB_USER,
  LIB_SYS,
  LIB_SSL
} code_t;

extern THREAD_LOCAL util::ThreadLocal<code_t> code;

void set(const char *error);
void set(std::string &error);
void set(std::string &&error);
};
#endif
