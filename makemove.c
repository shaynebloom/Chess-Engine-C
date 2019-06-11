// makemove.c

#include "defs.h"
#include "stdio.h"

#define HASH_PCE(pce,sq) (pos->posKey ^= (PieceKeys[(pce)][(sq)]))
#define HASH_CA (pos->posKey ^= (CastleKeys[(pos->castlePerm)]))
#define HASH_SIDE (pos->posKey ^= (SideKey))
#define HASH_EP (pos->posKey ^= (PieceKeys[EMPTY][(pos->enPas)]))

const int CastlePerm[120] = {
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 13, 15, 15, 15, 12, 15, 15, 14, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15,  7, 15, 15, 15,  3, 15, 15, 11, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static void ClearPiece(int sq, S_BOARD *pos) {
	ASSERT(SqOnBoard(sq)); // Make sure sq on board

	int pce = pos->pieces[sq]; // Get piece

	ASSERT(PieceValid(pce)); // Make sure it's a valid piece

	int col = PieceCol[pce]; // Get piece color
	int index = 0;
	int t_pceNum = -1;

	HASH_PCE(pce, sq); // Hash piece out of pos key

	pos->pieces[sq] = EMPTY; // Set square to empty
	pos->material[col] -= PieceVal[pce]; // Subtract piece value

	if (PieceBig[pce]) {
		pos->bigPce[col]--;
		if (PieceMaj[pce]) {
			pos->majPce[col]--;
		}
		else {
			pos->minPce[col]--;
		}
	}
	else {
		CLRBIT(pos->pawns[col], SQ64(sq));
		CLRBIT(pos->pawns[BOTH], SQ64(sq));
	}

	/*
		pce = wP in this example
		pos->pceNum[wP] == 5 Looping from 0 to 4
		pos->pList[wP][0] == sq0
		pos->pList[wP][1] == sq1
		pos->pList[wP][2] == sq2
		pos->pList[wP][3] == sq3
		pos->pList[wP][4] == sq4

		sq==sq3 so t_pceNum = 3;
	*/

	for (int i = 0; i < pos->pceNum[pce]; i++) { // Loops through [pce][i] until i is square pointing to piece we are deleting
		if (pos->pList[pce][i] == sq) {
			t_pceNum = i;
			break;
		}
	}

	ASSERT(t_pceNum != -1);

	pos->pceNum[pce]--; // Decrement pceNum, because we have one less after being removed.
	// pos->pceNum[wP] == 4

	pos->pList[pce][t_pceNum] = pos->pList[pce][pos->pceNum[pce]];
	//pos->pList[wP][3]	= pos->pList[wP][4] = sq4
	// Replacing removed piece with last piece on list. 
}

static void AddPiece(const int sq, S_BOARD *pos, const int pce) {

	ASSERT(PieceValid(pce)); // Make sure valid piece
	ASSERT(SqOnBoard(sq)); // Make sure valid square

	int col = PieceCol[pce]; // Get piece color

	HASH_PCE(pce, sq); // Hash piece into square

	pos->pieces[sq] = pce; // put piece in pieces

	if (PieceBig[pce]) {
		pos->bigPce[col]++; // increment bigPce if bigPce
		if (PieceMaj[pce]) {
			pos->majPce[col]++; // increment majPce if bigPce
		}
		else {
			pos->minPce[col]++; // increment minPce if bigPce
		}
	}
	else { // Otherwise pawn
		SETBIT(pos->pawns[col], SQ64(sq));
		SETBIT(pos->pawns[BOTH], SQ64(sq));
	}

	pos->material[col] += PieceVal[pce]; // Increase piece value
	pos->pList[pce][pos->pceNum[pce]++] = sq; // Add piece to pList

}

static void MovePiece(const int from, const int to, S_BOARD *pos) {

	ASSERT(SqOnBoard(from));  // Assert valid sq
	ASSERT(SqOnBoard(to)); // Assert valid sq

	int pce = pos->pieces[from]; // Find out what piece is
	int col = PieceCol[pce]; // Get piece color

	#ifdef DEBUG
		int t_PieceNum = FALSE;
	#endif

	HASH_PCE(pce, from);
	pos->pieces[from] = EMPTY; // hash piece out and remove from board array

	HASH_PCE(pce, to);
	pos->pieces[to] = pce; // hash piece in new sq and remove from board array

	if (!PieceBig[pce]) { // Pawn bitboard operations
		CLRBIT(pos->pawns[col], SQ64(from));
		CLRBIT(pos->pawns[BOTH], SQ64(from));
		SETBIT(pos->pawns[col], SQ64(to));
		SETBIT(pos->pawns[BOTH], SQ64(to));
	}

	for (int i = 0; i < pos->pceNum[pce]; ++i) {
		if (pos->pList[pce][i] == from) {
			pos->pList[pce][i] = to; // Find piece in pList and change square location
			#ifdef DEBUG
						t_PieceNum = TRUE;
			#endif
			break;
		}
	}
	ASSERT(t_PieceNum); // To make sure pieces match up with piece list
}

int MakeMove(S_BOARD *pos, int move) {
	ASSERT(CheckBoard(pos)); // Make sure position is okay

	int from = FROMSQ(move);
	int to = TOSQ(move);
	int side = pos->side;

	ASSERT(SqOnBoard(from)); // Asserts
	ASSERT(SqOnBoard(to));
	ASSERT(SideValid(side));
	ASSERT(PieceValid(pos->pieces[from]));

	// 
	pos->history[pos->hisPly].posKey = pos->posKey;

	if (move & MFLAGEP) { // Check if move was EnPas
		if (side == WHITE) {
			ClearPiece(to - 10, pos);
		}
		else {
			ClearPiece(to + 10, pos);
		}
	}
	else if (move & MFLAGCA) {
		switch (to) { // Rook moves for castling
		case C1:
			MovePiece(A1, D1, pos);
			break;
		case C8:
			MovePiece(A8, D8, pos);
			break;
		case G1:
			MovePiece(H1, F1, pos);
			break;
		case G8:
			MovePiece(H8, F8, pos);
			break;
		default: ASSERT(FALSE); break;
		}
	}

	if (pos->enPas != NO_SQ)
		HASH_EP; // Hash out EnPas
	HASH_CA; // Hash out current castle

	// Store history in array
	pos->history[pos->hisPly].move = move;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;

	// Castle perm contains array where every square is 15 except the initial rook and king squares.
	// By moving a piece from one of these squares, you and the special number with the castlePerm,
	// resulting in that option being removed from the castleParm bits.
	pos->castlePerm &= CastlePerm[from];
	pos->castlePerm &= CastlePerm[to];
	pos->enPas = NO_SQ;

	HASH_CA; // Hash in new castle permissions

	int captured = CAPTURED(move); // Get captured piece if any
	pos->fiftyMove++; // Increment fiftyMove, for keeping track of 50 move rule

	if (captured != EMPTY) {
		ASSERT(PieceValid(captured));
		ClearPiece(to, pos);
		pos->fiftyMove = 0; // There was a capture, so reset 50 moves counter
	}

	pos->hisPly++;
	pos->ply++;

	if (PiecePawn[pos->pieces[from]]) {
		pos->fiftyMove = 0;
		if (move & MFLAGPS) {  // If move was a pawn start, set EnPas
			if (side == WHITE) {
				pos->enPas = from + 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_3);
			}
			else {
				pos->enPas = from - 10;
				ASSERT(RanksBrd[pos->enPas] == RANK_6);
			}
			HASH_EP;
		}
	}

	// Everything has now been cleared, now move piece
	MovePiece(from, to, pos);

	int prPce = PROMOTED(move); // Check for promotion
	if (prPce != EMPTY) {
		ASSERT(PieceValid(prPce) && !PiecePawn[prPce]);
		ClearPiece(to, pos);
		AddPiece(to, pos, prPce);
	}


	if (PieceKing[pos->pieces[to]]) {
		pos->KingSq[pos->side] = to;
	}

	pos->side ^= 1; // Change side
	HASH_SIDE; // Hash in side

	ASSERT(CheckBoard(pos)); // Validate board


	if (SqAttacked(pos->KingSq[side], pos->side, pos)) {
		TakeMove(pos);
		return FALSE;  // Check if move was made while king is in check and still in check after move
						// if so return false to say move isn't possible
	}

	return TRUE; // Move was okay, return true
}

void TakeMove(S_BOARD *pos) {
	ASSERT(CheckBoard(pos)); // Check board

	pos->hisPly--; // Decrement turns
	pos->ply--;

	// Get move from history array
	int move = pos->history[pos->hisPly].move;
	int from = FROMSQ(move);
	int to = TOSQ(move);

	ASSERT(SqOnBoard(from));
	ASSERT(SqOnBoard(to));

	if (pos->enPas != NO_SQ) // If EnPas sq is set, hash it out
		HASH_EP;
	HASH_CA; // Hash out castle perms

	// Get permissions from history
	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ) // If EnPas sq is in old position, hash back in
		HASH_EP;
	HASH_CA; // Hash in castle permissions

	pos->side ^= 1; // Change side
	HASH_SIDE; // Hash side

	if (MFLAGEP & move) { // If move was EnPas capture, add back in captured pawn
		if (pos->side == WHITE) {
			AddPiece(to - 10, pos, bP);
		}
		else {
			AddPiece(to + 10, pos, wP);
		}
	}
	else if (MFLAGCA & move) { // If castle, move rook back to starting square
		switch (to) {
		case C1: MovePiece(D1, A1, pos); break;
		case C8: MovePiece(D8, A8, pos); break;
		case G1: MovePiece(F1, H1, pos); break;
		case G8: MovePiece(F8, H8, pos); break;
		default: ASSERT(FALSE); break;
		}
	}

	MovePiece(to, from, pos); // Move piece back

	if (PieceKing[pos->pieces[from]]) {
		pos->KingSq[pos->side] = from;
	}

	int captured = CAPTURED(move); // Get captured bit
	if (captured != EMPTY) { // Add captured piece back
		ASSERT(PieceValid(captured));
		AddPiece(to, pos, captured);
	}

	if (PROMOTED(move) != EMPTY) { // Clear piece
		ASSERT(PieceValid(PROMOTED(move)) && !PiecePawn[PROMOTED(move)]);
		ClearPiece(from, pos);
		AddPiece(from, pos, (PieceCol[PROMOTED(move)] == WHITE ? wP : bP)); // Add a pawn to from square
	}

	ASSERT(CheckBoard(pos)); // Make sure board is okay
}

void MakeNullMove(S_BOARD *pos) {

	ASSERT(CheckBoard(pos));
	ASSERT(!SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos));

	pos->ply++;
	pos->history[pos->hisPly].posKey = pos->posKey;

	if (pos->enPas != NO_SQ) HASH_EP;

	pos->history[pos->hisPly].move = NOMOVE;
	pos->history[pos->hisPly].fiftyMove = pos->fiftyMove;
	pos->history[pos->hisPly].enPas = pos->enPas;
	pos->history[pos->hisPly].castlePerm = pos->castlePerm;
	pos->enPas = NO_SQ;

	pos->side ^= 1;
	pos->hisPly++;
	HASH_SIDE;

	ASSERT(CheckBoard(pos));

	return;
}

void TakeNullMove(S_BOARD *pos) {
	ASSERT(CheckBoard(pos));

	pos->hisPly--;
	pos->ply--;

	if (pos->enPas != NO_SQ) HASH_EP;

	pos->castlePerm = pos->history[pos->hisPly].castlePerm;
	pos->fiftyMove = pos->history[pos->hisPly].fiftyMove;
	pos->enPas = pos->history[pos->hisPly].enPas;

	if (pos->enPas != NO_SQ) HASH_EP;
	pos->side ^= 1;
	HASH_SIDE;

	ASSERT(CheckBoard(pos));
}