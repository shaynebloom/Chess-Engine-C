// movegen.c

#include "stdio.h"
#include "defs.h"

#define MOVE(f,t,ca,pro,fl) ( (f) | ((t)<<7) | ( (ca) << 14) | ( (pro) << 20) | (fl) )
#define SQOFFBOARD(sq) (FilesBrd[(sq)]==OFFBOARD)

/*

MoveGen(board, list)
		Loop all pieces
			-> Slider loop each dir add move
				-> AddMove list->moves[list->count] = move; list->count++
*/
// int LoopSlidePce[8] = { wB, wR, wQ, EMPTY, bB, bR, bQ, EMPTY };

// int LoopSlideIndex[2] = { 0, 4 };

const int NumDir[13] = {
 0, 0, 8, 4, 4, 8, 8, 0, 8, 4, 4, 8, 8
};

const int PceDir[13][8] = {
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ 0, 0, 0, 0, 0, 0, 0 },
	{ -8, -19,	-21, -12, 8, 19, 21, 12 },
	{ -9, -11, 11, 9, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, 0, 0, 0, 0 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 },
	{ -1, -10,	1, 10, -9, -11, 11, 9 }
};

const int LoopSlidePce[2][3] = { { wB, wR, wQ }, { bB, bR, bQ } }; // Two dimensional array for Slide piece values [color][piece]
const int LoopNonSlidePce[2][2] = { { wN, wK }, { bN, bK } };// Two dimensional array for Non Slide piece values [color][piece]

const int VictimScore[13] = { 0, 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 };

static int MvvLvaScores[13][13];

/*
PV move
cap -> MvvLVA (Most valuable victim, least valuable attacker)
Killers (Moves with alpha beta cutoff
HistoryScore

MvvLVA
	P x Q
	N x Q
	B x Q
	...
	Start searching moves which involve capturing a valuable piece with 
	the least valuable attacker first


*/

int InitMvvLva() {
	for (int Attacker = wP; Attacker <= bK; ++Attacker) {
		for (int Victim = wP; Victim <= bK; ++Victim) {
			MvvLvaScores[Victim][Attacker] = VictimScore[Victim] + 6 - (VictimScore[Attacker] / 100);
		}
	}
	return 0;
}

int MoveExists(S_BOARD *pos, const int move) {

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int MoveNum = 0;
	for (MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue;
		}
		TakeMove(pos);
		if (list->moves[MoveNum].move == move) {
			return TRUE;
		}
	}
	return FALSE;
}

static void AddQuietMove(const S_BOARD *pos, int move, S_MOVELIST *list) {
	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));

	list->moves[list->count].move = move;

	if (pos->searchKillers[0][pos->ply] == move) {
		list->moves[list->count].score = 900000;
	}
	else if (pos->searchKillers[1][pos->ply] == move) {
		list->moves[list->count].score = 800000;
	}
	else {
		list->moves[list->count].score = pos->searchHistory[pos->pieces[FROMSQ(move)]][TOSQ(move)];
	}

	list->count++;
}

static void AddCaptureMove(const S_BOARD *pos, int move, S_MOVELIST *list) {
	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));
	ASSERT(PieceValid(CAPTURED(MOVE)));

	list->moves[list->count].move = move;
	list->moves[list->count].score = MvvLvaScores[CAPTURED(move)][pos->pieces[FROMSQ(move)]] + 1000000;
	list->count++;
}

static void AddEnPassantMove(const S_BOARD *pos, int move, S_MOVELIST *list) {
	ASSERT(SqOnBoard(FROMSQ(move)));
	ASSERT(SqOnBoard(TOSQ(move)));

	list->moves[list->count].move = move;
	list->moves[list->count].score = 105 + 1000000; // Pawn takes pawn, set score
	list->count++;
}

static void AddWhitePawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {
	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_7) {
		AddCaptureMove(pos, MOVE(from, to, cap, wQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, wN, 0), list);
	}
	else {
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
	}
}

static void AddWhitePawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_7) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, wQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, wN, 0), list);
	}
	else {
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
	}
}

static void AddBlackPawnCapMove(const S_BOARD *pos, const int from, const int to, const int cap, S_MOVELIST *list) {
	ASSERT(PieceValidEmpty(cap));
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_2) {
		AddCaptureMove(pos, MOVE(from, to, cap, bQ, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bR, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bB, 0), list);
		AddCaptureMove(pos, MOVE(from, to, cap, bN, 0), list);
	}
	else {
		AddCaptureMove(pos, MOVE(from, to, cap, EMPTY, 0), list);
	}
}

static void AddBlackPawnMove(const S_BOARD *pos, const int from, const int to, S_MOVELIST *list) {
	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (RanksBrd[from] == RANK_2) {
		AddQuietMove(pos, MOVE(from, to, EMPTY, bQ, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bR, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bB, 0), list);
		AddQuietMove(pos, MOVE(from, to, EMPTY, bN, 0), list);
	}
	else {
		AddQuietMove(pos, MOVE(from, to, EMPTY, EMPTY, 0), list);
	}
}

void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list) {
	ASSERT(CheckBoard(pos));

	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;

	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if (side == WHITE) {
		for (int pceNum = 0; pceNum < pos->pceNum[wP]; pceNum++) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq + 10] == EMPTY) { // Pawn moves moving forward
				AddWhitePawnMove(pos, sq, sq + 10, list);
				if (RanksBrd[sq] == RANK_2 && pos->pieces[sq + 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq, (sq + 20), EMPTY, EMPTY, MFLAGPS), list);
				}
			}

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);
			}
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);
			}
			if (pos->enPas != NO_SQ) { // NO_SQ is on edge of board, so could be used in rare cases
				if (sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				else if (sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

		if (pos->castlePerm & WKCA) { // Checking bitwise flag for white kingside castle
			if (pos->pieces[F1] == EMPTY && pos->pieces[G1] == EMPTY) { // Check stipulations in order that eliminates options quicker
				if (!SqAttacked(E1, BLACK, pos) && !SqAttacked(F1, BLACK, pos)) {
					AddQuietMove(pos, MOVE(E1, G1, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if (pos->castlePerm & WQCA) { // Checking bitwise flag for white queenside castle
			if ((pos->pieces[D1] == EMPTY && pos->pieces[C1] == EMPTY && pos->pieces[B1] == EMPTY) && 
				!SqAttacked(E1, BLACK, pos) && !SqAttacked(D1, BLACK, pos)) { // Short curcuit approach
				AddQuietMove(pos, MOVE(E1, C1, EMPTY, EMPTY, MFLAGCA), list); // Cound probably add this to the short circuit as well
																			// but it would be confusing
			}
		}
	}
	else {
		for (int pceNum = 0; pceNum < pos->pceNum[bP]; pceNum++) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (pos->pieces[sq - 10] == EMPTY) { // Pawn moves moving forward
				AddBlackPawnMove(pos, sq, sq - 10, list);
				if (RanksBrd[sq] == RANK_7 && pos->pieces[sq - 20] == EMPTY) {
					AddQuietMove(pos, MOVE(sq, (sq - 20), EMPTY, EMPTY, MFLAGPS), list);
				}
			}

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 9, pos->pieces[sq - 9], list);
			}
			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 11, pos->pieces[sq - 11], list);
			}
			if (pos->enPas != NO_SQ) { // NO_SQ is on edge of board, so could be used in rare cases
				if (sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				else if (sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

		if (pos->castlePerm &  BKCA) { // Black Kingside castle, about the same as white
			if (pos->pieces[F8] == EMPTY && pos->pieces[G8] == EMPTY) {
				if (!SqAttacked(E8, WHITE, pos) && !SqAttacked(F8, WHITE, pos)) {
					AddQuietMove(pos, MOVE(E8, G8, EMPTY, EMPTY, MFLAGCA), list);
				}
			}
		}

		if (pos->castlePerm &  BQCA) { // Black Queenside castle
			if (pos->pieces[D8] == EMPTY && pos->pieces[C8] == EMPTY && pos->pieces[B8] == EMPTY &&
				!SqAttacked(E8, WHITE, pos) && !SqAttacked(D8, WHITE, pos)) {
				AddQuietMove(pos, MOVE(E8, C8, EMPTY, EMPTY, MFLAGCA), list);
			}
		}
	}

	/* Loop for slide pieces */
	for (int i = 0; i < 3; i++) {
		pce = LoopSlidePce[side][i];
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; pceNum++) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (int j = 0; j < NumDir[pce]; j++) {
				dir = PceDir[pce][j];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE		WHITE ^ 1 == BLACK
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
					t_sq += dir;
				}

			}
		}
	}
	
	/* Loop for non slide */
	for (int i = 0; i < 2; i++) {
		pce = LoopNonSlidePce[side][i];
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; pceNum++) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (int j = 0; j < NumDir[pce]; j++) {
				dir = PceDir[pce][j];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq)) { // Go to next loop, move off board
					continue;
				}

				// BLACK ^ 1 == WHITE		WHITE ^ 1 == BLACK
				if (pos->pieces[t_sq] != EMPTY) { // Piece on target square
					if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) { // Check if enemy piece or not
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue; // Friendly piece, go to next loop
				}
				AddQuietMove(pos, MOVE(sq, t_sq, EMPTY, EMPTY, 0), list);
			}
		}
	}
}

void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list) {
	ASSERT(CheckBoard(pos));

	list->count = 0;

	int pce = EMPTY;
	int side = pos->side;
	int sq = 0; int t_sq = 0;
	int pceNum = 0;

	int dir = 0;
	int index = 0;
	int pceIndex = 0;

	if (side == WHITE) {
		for (int pceNum = 0; pceNum < pos->pceNum[wP]; pceNum++) {
			sq = pos->pList[wP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq + 9) && PieceCol[pos->pieces[sq + 9]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 9, pos->pieces[sq + 9], list);
			}
			if (!SQOFFBOARD(sq + 11) && PieceCol[pos->pieces[sq + 11]] == BLACK) {
				AddWhitePawnCapMove(pos, sq, sq + 11, pos->pieces[sq + 11], list);
			}
			if (pos->enPas != NO_SQ) { // NO_SQ is on edge of board, so could be used in rare cases
				if (sq + 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				else if (sq + 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq + 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

	}
	else {
		for (int pceNum = 0; pceNum < pos->pceNum[bP]; pceNum++) {
			sq = pos->pList[bP][pceNum];
			ASSERT(SqOnBoard(sq));

			if (!SQOFFBOARD(sq - 9) && PieceCol[pos->pieces[sq - 9]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 9, pos->pieces[sq - 9], list);
			}
			if (!SQOFFBOARD(sq - 11) && PieceCol[pos->pieces[sq - 11]] == WHITE) {
				AddBlackPawnCapMove(pos, sq, sq - 11, pos->pieces[sq - 11], list);
			}
			if (pos->enPas != NO_SQ) { // NO_SQ is on edge of board, so could be used in rare cases
				if (sq - 9 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 9, EMPTY, EMPTY, MFLAGEP), list);
				}
				else if (sq - 11 == pos->enPas) {
					AddEnPassantMove(pos, MOVE(sq, sq - 11, EMPTY, EMPTY, MFLAGEP), list);
				}
			}
		}

	}

	/* Loop for slide pieces */
	for (int i = 0; i < 3; i++) {
		pce = LoopSlidePce[side][i];
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; pceNum++) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (int j = 0; j < NumDir[pce]; j++) {
				dir = PceDir[pce][j];
				t_sq = sq + dir;

				while (!SQOFFBOARD(t_sq)) {
					// BLACK ^ 1 == WHITE		WHITE ^ 1 == BLACK
					if (pos->pieces[t_sq] != EMPTY) {
						if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) {
							AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
						}
						break;
					}
					t_sq += dir;
				}

			}
		}
	}

	/* Loop for non slide */
	for (int i = 0; i < 2; i++) {
		pce = LoopNonSlidePce[side][i];
		ASSERT(PieceValid(pce));

		for (pceNum = 0; pceNum < pos->pceNum[pce]; pceNum++) {
			sq = pos->pList[pce][pceNum];
			ASSERT(SqOnBoard(sq));

			for (int j = 0; j < NumDir[pce]; j++) {
				dir = PceDir[pce][j];
				t_sq = sq + dir;

				if (SQOFFBOARD(t_sq)) { // Go to next loop, move off board
					continue;
				}

				// BLACK ^ 1 == WHITE		WHITE ^ 1 == BLACK
				if (pos->pieces[t_sq] != EMPTY) { // Piece on target square
					if (PieceCol[pos->pieces[t_sq]] == (side ^ 1)) { // Check if enemy piece or not
						AddCaptureMove(pos, MOVE(sq, t_sq, pos->pieces[t_sq], EMPTY, 0), list);
					}
					continue; // Friendly piece, go to next loop
				}
			}
		}
	}
}