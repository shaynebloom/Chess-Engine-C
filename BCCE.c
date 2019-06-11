#include "stdio.h"
#include "defs.h"
#include "string.h"



int main() {
	AllInit();

	S_BOARD * pos = GenBoard();
	S_SEARCHINFO info[1];
	{
		info->depth = 0;
		info->depthset = 0;
		info->fh = 0;
		info->fhf = 0;
		info->movestogo = 0;
		info->nodes = 0;
		info->quit = 0;
		info->starttime = 0;
		info->stopped = 0;
		info->stoptime = 0;
		info->timeset = 0;
		info->infinite = 0;
		info->GAME_MODE = 0;
		info->POST_THINKING = 0;
	}
	InitHashTable(pos->HashTable);
	setvbuf(stdin, NULL, _IOFBF, 256);
	setvbuf(stdout, NULL, _IOFBF, 256);

	printf("Welcome to BCCE! Type 'bcce' for console mode...\n");

	char line[256];
	while (TRUE) {
		memset(&line[0], 0, sizeof(line));

		fflush(stdout);
		if (!fgets(line, 256, stdin))
			continue;
		if (line[0] == '\n')
			continue;
		if (!strncmp(line, "uci", 3)) {
			Uci_Loop(pos, info);
			if (info->quit == TRUE) break;
			continue;
		}
		else if (!strncmp(line, "xboard", 6)) {
			XBoard_Loop(pos, info);
			if (info->quit == TRUE) break;
			continue;
		}
		else if (!strncmp(line, "bcce", 4)) {
			Console_Loop(pos, info);
			if (info->quit == TRUE) break;
			continue;
		}
		else if (!strncmp(line, "quit", 4)) {
			break;
		}
	}

	free(pos->HashTable->pTable);
	free(pos);
	return 0;
}