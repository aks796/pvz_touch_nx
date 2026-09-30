#ifndef __SUBSTRATEHOOK_H__
#define __SUBSTRATEHOOK_H__

#include <stdlib.h>

#define _extern extern "C" __attribute__((__visibility__("hidden")))

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void MSHookFunction(void *symbol, void *replace, void **result);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SUBSTRATEHOOK_H__
