#ifndef READLINE_H
#define READLINE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
char *readline(const char *prompt, char *buffer, size_t size);
int readlineStrcmp(const char *prompt, char *buffer, size_t size, const char* sentinel);
void enableRawMode(void);
void disableRawMode(void);
int addToHistory(const char *line);
int loadHistoryFile(const char *filename);
int loadHistoryFromConf(void);
void saveHistory(void);
extern int readlineEOFFlag;
#ifdef __cplusplus
}
#endif
#endif
