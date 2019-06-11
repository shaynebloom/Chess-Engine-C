// io.c

#include "stdio.h"
#include "defs.h"

char *PrSq(const int sq) {
	
	static char SqStr[3];

	int file = FilesBrd[sq];
	int rank = RanksBrd[sq];

	sprintf_s(SqStr, sizeof(SqStr), "%c%c", ('a' + file), ('1' + rank));

	return SqStr;
}

char *PrMove(const int move) {
	static char MvStr[6];

	int ff = FilesBrd[FROMSQ(move)];
	int rf = RanksBrd[FROMSQ(move)];
	int ft = FilesBrd[TOSQ(move)];
	int rt = RanksBrd[TOSQ(move)];

	int promoted = PROMOTED(move);

	if (promoted) {
		char pchar = 'q';
		if (IsKn(promoted))
			pchar = 'n';
		else if (IsRQ(promoted) && !IsBQ(promoted))
			pchar = 'r';
		else if (!IsRQ(promoted) && IsBQ(promoted))
			pchar = 'b';
		sprintf_s(MvStr, sizeof(MvStr), "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
	}
	else {
		sprintf_s(MvStr, sizeof(MvStr), "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));
	}

	return MvStr;
}

// Takes in move typed in and tries to make it a int move
int ParseMove(char *ptrChar, S_BOARD *pos) {
	if (ptrChar[1] > '8' || ptrChar[1] < '1') // Make sure first 4 characters are valid
		return NOMOVE;
	if (ptrChar[3] > '8' || ptrChar[3] < '1')
		return NOMOVE;
	if (ptrChar[0] > 'h' || ptrChar[0] < 'a')
		return NOMOVE;
	if (ptrChar[2] > 'h' || ptrChar[2] < 'a')
		return NOMOVE;

	int from = FR2SQ(ptrChar[0] - 'a', ptrChar[1] - '1'); // Get from sq data
	int to = FR2SQ(ptrChar[2] - 'a', ptrChar[3] - '1'); // Get to sq data

	ASSERT(SqOnBoard(from) && SqOnBoard(to));

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);
	int Move = 0;
	int PromPce = EMPTY;

	for (int MoveNum = 0; MoveNum < list->count; MoveNum++) { // Loop through all moves to find user entered move
		Move = list->moves[MoveNum].move; // Get move from list
		if (FROMSQ(Move) == from && TOSQ(Move) == to) { // Might be same move, unless promoted to different piece
			PromPce = PROMOTED(Move);
			if (PromPce != EMPTY) {
				if (IsRQ(PromPce) && !IsBQ(PromPce) && ptrChar[4] == 'r')
					return Move;
				else if (!IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'b')
					return Move;
				else if (IsRQ(PromPce) && IsBQ(PromPce) && ptrChar[4] == 'q')
					return Move;
				else if (IsKn(PromPce) && ptrChar[4] == 'n')
					return Move;
				continue;
			}
			return Move;
		}
	}

	return NOMOVE; // Move not found, user entered invalid move
}

void PrintMoveList(const S_MOVELIST *list) {
	int score = 0;
	int move = 0;
	printf("MoveList: %d\n", list->count);

	for (int i = 0; i < list->count; i++) {
		move = list->moves[i].move;
		score = list->moves[i].score;

		printf("Move:%d > %s (score:%d)\n", i + 1, PrMove(move), score);
	}
	printf("MoveList Total %d Moves:\n\n", list->count);
}