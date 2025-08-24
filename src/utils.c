#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>

int getTermWidth(void) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return 80;
	} else {
		return ws.ws_col;
	}
}

char *joinWithNewline(char **arr, int count) {
	if (count == 0) return NULL;

	size_t total_len = 0;
	for (int i = 0; i < count; i++) {
		total_len += strlen(arr[i]) + 1;
	}

	char *result = malloc(total_len + 1);
	if (!result) return NULL;

	char *p = result;
	for (int i = 0; i < count; i++) {
		size_t len = strlen(arr[i]);
		memcpy(p, arr[i], len);
		p += len;
		*p++ = '\n';
	}
	*(p - 1) = '\0';

	return result;
}
