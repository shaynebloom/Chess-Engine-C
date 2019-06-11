// perft.c

#include "defs.h"
#include "stdio.h"

long leafNodes;

void Perft(int depth, S_BOARD *pos) {

	ASSERT(CheckBoard(pos)); // Make sure valid position

	if (depth == 0) {
		leafNodes++;
		return;
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	for (int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move)) { // Generate moves
			continue;
		}
		Perft(depth - 1, pos); // Recursive call
		TakeMove(pos);
	}

	return;
}

void PerftTest(int depth, S_BOARD *pos) {

	ASSERT(CheckBoard(pos)); // Make sure valid position

	PrintBoard(pos);
	printf("\nStarting Test To Depth:%d\n", depth);
	leafNodes = 0;
	int start = GetTimeMs();

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int move;
	for ( int MoveNum = 0; MoveNum < list->count; ++MoveNum) {
		move = list->moves[MoveNum].move;
		if (!MakeMove(pos, move)) {
			continue;
		}
		long cumnodes = leafNodes;
		Perft(depth - 1, pos);
		TakeMove(pos);
		long oldnodes = leafNodes - cumnodes;
		printf("move %d : %s : %ld\n", MoveNum + 1, PrMove(move), oldnodes);
	}

	printf("\nTest Complete : %ld nodes visited in %dms\n", leafNodes, GetTimeMs() - start);

	return;
}