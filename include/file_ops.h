#ifndef FILE_OPS_H_
#define FILE_OPS_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
void fopsSave(const char* name, const char* info);
void fopsAppend(const char* name, const char* info);
char* fopsLoad(const char* name);
void touch(const char* file);
#ifdef  __cplusplus
}
#endif
#endif
