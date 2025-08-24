#define _POSIX_C_SOURCE 200809L
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "utils.h"
#include "file_ops.h"

static char **history = NULL;
static int historyCount = 0;
static int historyCapacity = 0;
static int historyIndex = 0;
static struct termios originalTermios;
static int isRawMode = 0;
int readlineEOFFlag = 0;

void disableRawModeOnExit(void) {
	if (isRawMode) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
		isRawMode = 0;
		write(STDOUT_FILENO, "\x1b[?25h", 6);
		fflush(stdout);
	}
}

int enableRawMode(void) {
	if (!isatty(STDIN_FILENO)) {
		errno = ENOTTY;
		return -1;
	}
	static int cleanup_registered = 0;
	if (!cleanup_registered) {
		atexit(disableRawModeOnExit);
		cleanup_registered = 1;
	}

	if (tcgetattr(STDIN_FILENO, &originalTermios) == -1) return -1;

	struct termios raw = originalTermios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return -1;

	isRawMode = 1;
	return 0;
}

void disableRawMode(void) {
	if (isRawMode && tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios) != -1) {
		isRawMode = 0;
		write(STDOUT_FILENO, "\x1b[?25h", 6);
		fflush(stdout);
	}
}

void freeHistory(void) {
	if (history) {
		//DPRINT("clearing history\n");
		for (int i = 0; i < historyCount; i++) {
			free(history[i]);
		}
		free(history);
		history = NULL;
		historyCount = 0;
		historyCapacity = 0;
	}
}

void registerSaveHistory(void) {
	static int history_cleanup_registered = 0;
	if (!history_cleanup_registered) {
		//DPRINT("freeHistory registered\n");
		atexit(freeHistory);
		history_cleanup_registered = 1;
	}
}

int addToHistory(const char *line) {
	registerSaveHistory();
	if (!line || line[0] == '\0' || (historyCount > 0 && strcmp(history[historyCount - 1], line) == 0)) {
		return 0;
	}

	if (history == NULL) {
		history = malloc(INITIAL_HISTORY_CAPACITY * sizeof(char *));
		if (!history) return -1;
		historyCapacity = INITIAL_HISTORY_CAPACITY;
	} else if (historyCount >= historyCapacity) {
		historyCapacity *= 2;
		char **new_history = realloc(history, historyCapacity * sizeof(char *));
		if (!new_history) {
			historyCapacity /= 2;
			return -1;
		}
		history = new_history;
	}

	history[historyCount] = strdup(line);
	if (!history[historyCount]) return -1;
	historyCount++;
	return 0;
}

int loadHistoryFile(const char *filename) {
	touch(filename);
	registerSaveHistory();
	char *file_contents = fopsLoad(filename);
	if (!file_contents)
		return -1;

	char *line = file_contents;
	char *next_line;

	while (line && *line) {
		next_line = strchr(line, '\n');
		if (next_line)
			*next_line = 0;

		addToHistory(line);

		if (next_line)
			line = next_line + 1;
		else
			break;
	}

	free(file_contents);
	return 0;
}

int loadHistoryFromConf(void) {
	int freehome = 0;
	char *home = getenv("HOME");
	if (!home) {
		home = getcwd(NULL, 0);
		if (!home) {
			perror("getcwd");
			return -1;
		} else
			freehome = 1;
	}

	int path_size = strlen(home) + strlen(HISTORY_FILE) + 2;

	char *path = calloc(path_size, 1);
	snprintf(path, path_size, "%s/%s", home, HISTORY_FILE);

	int return_code = loadHistoryFile(path);

	free(path);
	if (freehome)
		free(home);
	return return_code;
}

int redrawLine(const char *prompt, char *buf, size_t len, size_t pos, int prevRows) {
	if (prevRows > 0) {
		char seq[32];
		snprintf(seq, sizeof(seq), "\x1b[%dA", prevRows);
		write(STDOUT_FILENO, seq, strlen(seq));
	}

	write(STDOUT_FILENO, "\r\x1b[J", 4);

	size_t promptLen = strlen(prompt);
	write(STDOUT_FILENO, prompt, promptLen);
	write(STDOUT_FILENO, buf, len);

	int termWidth = getTermWidth();
	int currentTotalRows = (promptLen + len) / termWidth;
	int cursorCol = (promptLen + pos) % termWidth;

	char seq[32];
	snprintf(seq, sizeof(seq), "\r");
	write(STDOUT_FILENO, seq, strlen(seq));

	if (cursorCol > 0) {
		char seq[32];
		snprintf(seq, sizeof(seq), "\x1b[%dC", cursorCol);
		write(STDOUT_FILENO, seq, strlen(seq));
	}

	fflush(stdout);

	return currentTotalRows;
}

void saveHistory(void) {
	int freehome = 0;
	char *home = getenv("HOME");
	if (!home) {
		home = getcwd(NULL, 0);
		if (!home) {
			perror("getcwd");
			return;
		} else
			freehome = 1;
	}

	int path_size = strlen(home) + strlen(HISTORY_FILE) + 2;

	char *path = calloc(path_size, 1);
	snprintf(path, path_size, "%s/%s", home, HISTORY_FILE);

	char *joined = joinWithNewline(history, historyCount);
	if (!joined) {
		//DPRINT("join failed\n");
		//DPRINTF("history entries: %d\n", historyCount);
		for (int i = 0; i < historyCount; i++)
			//DPRINTF("history entry %d: %s\n", i, history[i]);

		free(path);
		if (freehome)
			free(home);
		return;
	}
	fopsSave(path, joined);
	//DPRINTF("path: %s\n%s\n", path, joined);
	free(path);
	free(joined);
	if (freehome)
		free(home);
}

char *readline(const char *prompt, char *buffer, size_t size) {
	static int isHistoryExitRegistered = 0;
	if (!isHistoryExitRegistered) {
		//DPRINT("saveHistory registered\n");
		isHistoryExitRegistered = 1;
		atexit(saveHistory);
	}

	if (!buffer || size <= 0) {
		errno = EINVAL;
		return NULL;
	}
	if (!prompt) prompt = "";

	if (!isatty(STDIN_FILENO)) {
		return fgets(buffer, size, stdin);
	}

	if (enableRawMode() == -1) {
		if (buffer)
			return fgets(buffer, size, stdin);
		else
			return NULL;
	}

	size_t bufpos = 0;
	size_t buflen = 0;
	int original_historyIndex = historyCount;
	historyIndex = historyCount;
	char temp_buffer[size];
	temp_buffer[0] = '\0';
	buffer[0] = '\0';

	int renderedRows = 0;

	write(STDOUT_FILENO, prompt, strlen(prompt));
	fflush(stdout);

	while (1) {
		char c;
		ssize_t nread = read(STDIN_FILENO, &c, 1);

		if (nread == 0) {
			continue;
		}

		if (nread < 0) {
			if (errno == EAGAIN) {
				continue;
			} else {
				disableRawMode();
				perror("\ninteractive_readline: read error");
				return NULL;
			}
		}

		switch (c) {
			case '\r':
			case '\n':
				/*printf("DEBUG: buffer: %s\n", buffer);*/
				disableRawMode();
				buffer[buflen] = 0;
				write(STDOUT_FILENO, "\n", 1); fflush(stdout);
				/*printf("DEBUG: buffer: %s\n", buffer);*/

				/*const char* final_line = (historyIndex == original_historyIndex) ? buffer : temp_buffer;
				  /if (final_line[0])*/
				addToHistory(buffer);
				if (!buffer[0])
					return NORUN_SENTINEL;
				/*if (historyIndex != original_historyIndex) {
				  strncpy(buffer, temp_buffer, size -1);
				  buffer[size - 1] = '\0';
				  }*/
				/*printf("DEBUG: buffer: %s\n", buffer);*/
				return buffer;

			case 127:
			case 8:
				if (bufpos > 0) {
					memmove(buffer + bufpos - 1, buffer + bufpos, buflen - bufpos);
					bufpos--;
					buflen--;
					buffer[buflen] = '\0';
					if(historyIndex != original_historyIndex) strcpy(temp_buffer, buffer);
					renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);
				}
				break;

			case 3:
				disableRawMode();
				errno = EINTR;
				buffer[buflen] = '\0';
				printf("\n");
				return NORUN_SENTINEL;
			case 4:
				if (buflen == 0) {
					disableRawMode();
					errno = EIO;
					readlineEOFFlag = 1;
					write(STDOUT_FILENO, "^D\n", 3);
					return NORUN_SENTINEL;
				}
				break;
			case 12:
				printf("\n\x1b[H\x1b[2J");
				fflush(stdout);
				renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);
				fflush(stdout);
				break;
			case '\x1b': {
				 char seq[3];
				 if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
				 if (seq[0] != '[') continue;
				 if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

				 if (seq[1] >= '0' && seq[1] <= '9') {
					 if (read(STDIN_FILENO, &seq[2], 1) != 1) continue;
					 if (seq[2] == '~') {
						 switch (seq[1]) {
							 case '1': case '7':
								 if (bufpos > 0) { bufpos = 0; renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows); }
								 break;
							 case '4': case '8':
								 if (bufpos < buflen) { bufpos = buflen; renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows); }
								 break;
							 case '3':
								 if (bufpos < buflen) {
									 memmove(buffer + bufpos, buffer + bufpos + 1, buflen - bufpos);
									 buflen--;
									 buffer[buflen] = '\0';
									 if(historyIndex != original_historyIndex) strcpy(temp_buffer, buffer);
									 renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);
								 }
								 break;
						 }
					 }
				 } else {
					 switch (seq[1]) {
						 case 'A':
							 if (historyCount > 0 && historyIndex > 0) {
								 if(historyIndex == original_historyIndex) strcpy(temp_buffer, buffer);
								 historyIndex--;
								 strncpy(buffer, history[historyIndex], size - 1);
								 buffer[size - 1] = '\0';
								 buflen = strlen(buffer);
								 bufpos = buflen;
								 /*printf("DEBUG: buffer: %s\n", buffer);*/
								 renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);								 fflush(stdout);
							 }
							 break;
						 case 'B':
							 if (historyIndex < original_historyIndex) {
								 historyIndex++;
								 if (historyIndex == original_historyIndex) {
									 strncpy(buffer, temp_buffer, size - 1);
								 } else {
									 strncpy(buffer, history[historyIndex], size - 1);
								 }
								 buffer[size - 1] = '\0';
								 buflen = strlen(buffer);
								 bufpos = buflen;
								 renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);								 fflush(stdout);
							 }
							 break;
						 case 'C':
							 if (bufpos < buflen) {
								 bufpos++;
								 write(STDOUT_FILENO, "\x1b[C", 3);
								 fflush(stdout);
							 }
							 break;
						 case 'D':
							 if (bufpos > 0) {
								 bufpos--;
								 write(STDOUT_FILENO, "\x1b[D", 3);
								 fflush(stdout);
							 }
							 break;
					 }
				 }
				 break;
				}

			default:
				if (isprint(c) && buflen < size - 1) {
					if (bufpos == buflen) {
						buffer[bufpos] = c;
						bufpos++;
						buflen++;
						buffer[buflen] = '\0';
						write(STDOUT_FILENO, &c, 1);
					} else {
						memmove(buffer + bufpos + 1, buffer + bufpos, buflen - bufpos);
						buffer[bufpos] = c;
						bufpos++;
						buflen++;
						buffer[buflen] = '\0';
						renderedRows = redrawLine(prompt, buffer, buflen, bufpos, renderedRows);
					}
					if(historyIndex != original_historyIndex) strcpy(temp_buffer, buffer);
					fflush(stdout);
				}
				break;
		}
	}

	disableRawMode();
	return NULL;
}
int readlineStrcmp(const char *prompt, char *buffer, size_t size, const char* sentinel) {
	char *ret = readline(prompt, buffer, size);
	int toBeReturned;
	//DPRINTF("returned value from readline: %s\n", ret);
	if (ret) {
		toBeReturned = strcmp(ret, sentinel);
		//DPRINTF("ret is not null, returning %d\n", toBeReturned);
	} else {
		toBeReturned = 0;
		//DPRINTF("ret is null, returning %d\n", toBeReturned);
	}
	return toBeReturned;
}
