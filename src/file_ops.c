#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int touch(const char* filename) {
	int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd == -1) {
		/*perror(filename);*/
		return -1;
	} else {
		close(fd);
		return 0;
	}
}

void fopsSave(const char* name, const char* info) {
	FILE *file = fopen(name, "w");
	if (file != NULL) {
		fprintf(file, "%s", info);		
		fclose(file);
	}
}

void fopsAppend(const char* name, const char* info) {
	FILE *file = fopen(name, "a");
	if (file != NULL) {
		fprintf(file, "%s\n", info);		
		fclose(file);
	}
}

char* fopsLoad(const char* name) {
	FILE *file = fopen(name, "rb");
	if (!file) {
		perror("fopen");
		return NULL;
	}
	struct stat st;

	if (fstat(fileno(file), &st)) {
		fclose(file);
		perror("stat");
		return NULL;
	}

	char *buffer = malloc(st.st_size + 1);
	if (!buffer) {
		fclose(file);
		perror("malloc");
		return NULL;
	}

	fread(buffer, 1, st.st_size, file);
	buffer[st.st_size] = 0;

	fclose(file);
	return buffer;
}
