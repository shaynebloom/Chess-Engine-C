// attack.c

#include "defs.h"

const int KnDir[8] = { -8,	-19,	-21,	-12,	8,	19,		21,	12 };
const int RkDir[4] = { -1,	-10,	1,		10 };
const int BiDir[4] = { -9,	-11,	11,		9 };
const int KiDir[8] = { -1,	-10,	1,		10,		-9,	-11,	11,	2 };

int SqAttacked(const int sq, const int side, const S_BOARD *pos) {

	int pce, t_sq, dir;

	ASSERT(SqOnBoard(sq));
	ASSERT(SideValid(side));
	ASSERT(CheckBoard(pos));
	// Pawns
	if (side == WHITE) {
		if (pos->pieces[sq - 11] == wP || pos->pieces[sq - 9] == wP)
			return TRUE;
	}
	else {
		if (pos->pieces[sq + 11] == bP || pos->pieces[sq + 9] == bP)
			return TRUE;
	}

	// Knights
	for (int i = 0; i < 8; i++) { // Iterate through knight attack pattern
		pce = pos->pieces[sq + KnDir[i]];
		if (pce != OFFBOARD && IsKn(pce) && PieceCol[pce] == side)
			return TRUE;
	}

	// Bishop and Queen
	for (int i = 0; i < 4; i++) {
		dir = BiDir[i]; // Set direction to attack
		t_sq = sq + dir; // Set square to check
		pce = pos->pieces[t_sq]; // Get piece from square
		while (pce != OFFBOARD) {
			if (pce != EMPTY) {
				if (IsBQ(pce) && PieceCol[pce] == side) // Check if enemy rook or queen
					return TRUE;
				break; // Found a blocker, so no need to check further
			}
			t_sq += dir;
			pce = pos->pieces[t_sq];
		}
	}

	// Rook and Queen
	for (int i = 0; i < 4; i++) {
		dir = RkDir[i]; // Set direction to attack
		t_sq = sq + dir; // Set square to check
		pce = pos->pieces[t_sq]; // Get piece from square
		while (pce != OFFBOARD) { 
			if (pce != EMPTY) {
				if (IsRQ(pce) && PieceCol[pce] == side) // Check if enemy rook or queen
					return TRUE;
				break; // Found a blocker, so no need to check further
			}
			t_sq += dir;
			pce = pos->pieces[t_sq];
		}
	}

	// King
	for (int i = 0; i < 8; i++) { // Iterate through knight attack pattern
		pce = pos->pieces[sq + KiDir[i]];
		if (pce != OFFBOARD && IsKi(pce) && PieceCol[pce] == side)
			return TRUE;
	}

	return FALSE;
}