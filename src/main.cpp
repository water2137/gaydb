#include "config.h"
#include "readline.h"
#include "exec.hpp"

int main(void) {
	loadHistoryFromConf();
	while (1) {
		if (readlineEOFFlag) break;
		char code_line[MAX_CODE_SIZE];
		if (!readlineStrcmp(PROMPT, code_line, sizeof(code_line), NORUN_SENTINEL))
			continue;
		execOwenSQL::runCMD(code_line);
	}
	return 0;
}
