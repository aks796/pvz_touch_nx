#ifndef _SYMBOL_FINDER_H_
#define _SYMBOL_FINDER_H_

#include <unistd.h>

extern int find_name(pid_t pid, const char *name, const char *libn, unsigned long *addr);
extern int find_libbase(pid_t pid, const char *libn, unsigned long *addr);

#endif // _SYMBOL_FINDER_H_
