// board.c

#include "stdio.h"
#include "defs.h"

int CheckBoard(const S_BOARD *pos) {
	// t_ for temporary
	int t_pceNum[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int t_bigPce[2] = { 0,0 };
	int t_majPce[2] = { 0,0 };
	int t_minPce[2] = { 0,0 };
	int t_material[2] = { 0,0 };

	int sq64, t_piece, sq120, color, pcount;

	U64 t_pawns[3] = { 0ULL, 0ULL, 0ULL };

	t_pawns[WHITE] = pos->pawns[WHITE];
	t_pawns[BLACK] = pos->pawns[BLACK];
	t_pawns[BOTH] = pos->pawns[BOTH];

	// Check piece lists
	for (t_piece = wP; t_piece <= bK; t_piece++) {
		for (int t_pce_num = 0; t_pce_num < pos->pceNum[t_piece]; t_pce_num++) {
			sq120 = pos->pList[t_piece][t_pce_num];
			ASSERT(pos->pieces[sq120] == t_piece);
		}
	}

	for (int sq64 = 0; sq64 < 64; sq64++) {
		sq120 = SQ120(sq64);
		t_piece = pos->pieces[sq120];
		t_pceNum[t_piece]++;
		color = PieceCol[t_piece];
		if (PieceBig[t_piece])
			t_bigPce[color]++;
		if (PieceMin[t_piece])
			t_minPce[color]++;
		if (PieceMaj[t_piece])
			t_majPce[color]++;

		t_material[color] += PieceVal[t_piece];
	}

	for (t_piece = wP; t_piece <= bK; t_piece++)
		ASSERT(t_pceNum[t_piece] == pos->pceNum[t_piece]);

	// Check bitboards count
	pcount = CNT(t_pawns[WHITE]); // Count bits in white pawn bb
	ASSERT(pcount == pos->pceNum[wP]);
	pcount = CNT(t_pawns[BLACK]); // Count bits in black pawn bb
	ASSERT(pcount == pos->pceNum[bP]);
	pcount = CNT(t_pawns[BOTH]); // Count bits in both
	ASSERT(pcount == pos->pceNum[wP] + pos->pceNum[bP]);

	// Check bitboards squares
	while (t_pawns[WHITE]) {
		sq64 = POP(&t_pawns[WHITE]);
		ASSERT(pos->pieces[SQ120(sq64)] == wP);
	}

	while (t_pawns[BLACK]) {
		sq64 = POP(&t_pawns[BLACK]);
		ASSERT(pos->pieces[SQ120(sq64)] == bP);
	}
	while (t_pawns[BOTH]) {
		sq64 = POP(&t_pawns[BOTH]);
		ASSERT( (pos->pieces[SQ120(sq64)] == bP) || (pos->pieces[SQ120(sq64)] == wP) );
	}

	ASSERT(t_material[WHITE] == pos->material[WHITE] && t_material[BLACK] == pos->material[BLACK]);
	ASSERT(t_minPce[WHITE] == pos->minPce[WHITE] && t_minPce[BLACK] == pos->minPce[BLACK]);
	ASSERT(t_majPce[WHITE] == pos->majPce[WHITE] && t_majPce[BLACK] == pos->majPce[BLACK]);
	ASSERT(t_bigPce[WHITE] == pos->bigPce[WHITE] && t_bigPce[BLACK] == pos->bigPce[BLACK]);

	ASSERT(pos->side == WHITE || pos->side == BLACK);
	ASSERT(GeneratePosKey(pos) == pos->posKey);

	ASSERT(pos->enPas == NO_SQ || (RanksBrd[pos->enPas] == RANK_6 && pos->side == WHITE)
		|| (RanksBrd[pos->enPas] == RANK_3 && pos->side == BLACK));

	ASSERT(pos->pieces[pos->KingSq[WHITE]] == wK);
	ASSERT(pos->pieces[pos->KingSq[BLACK]] == bK);

	return TRUE;
}

void UpdateListsMaterial(S_BOARD *pos) {
	int piece, sq,color;
	pos->material[BLACK] = 0;
	for (int i = 0; i < BRD_SQ_NUM; i++) {
		sq = i;
		piece = pos->pieces[i];
		if (piece != OFFBOARD && piece != EMPTY) {
			color = PieceCol[piece];
			if (PieceBig[piece])
				pos->bigPce[color]++;
			if (PieceMin[piece])
				pos->minPce[color]++;
			if (PieceMaj[piece])
				pos->majPce[color]++;
			pos->material[color] += PieceVal[piece];

			// piece list
			pos->pList[piece][pos->pceNum[piece]] = sq;
			pos->pceNum[piece]++;

			if (piece == wK)
				pos->KingSq[WHITE] = sq;
			if (piece == bK)
				pos->KingSq[BLACK] = sq;

			if (piece == wP) {
				SETBIT(pos->pawns[WHITE], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			} else if (piece == bP) {
				SETBIT(pos->pawns[BLACK], SQ64(sq));
				SETBIT(pos->pawns[BOTH], SQ64(sq));
			}
		}
	}
}

int ParseFen(char *fen, S_BOARD *pos) {
	ASSERT(fen != NULL);
	ASSERT(pos != NULL);

	int rank = RANK_8;
	int file = FILE_A;
	int piece = 0;
	int count = 0;
	int sq64 = 0;
	int sq120 = 0;

	ResetBoard(pos);

	while ((rank >= RANK_1) && *fen) {
		count = 1;
		switch (*fen) {
			case 'p': piece = bP; break;
			case 'r': piece = bR; break;
			case 'n': piece = bN; break;
			case 'b': piece = bB; break;
			case 'k': piece = bK; break;
			case 'q': piece = bQ; break;
			case 'P': piece = wP; break;
			case 'R': piece = wR; break;
			case 'N': piece = wN; break;
			case 'B': piece = wB; break;
			case 'K': piece = wK; break;
			case 'Q': piece = wQ; break;

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
				piece = EMPTY;
				count = *fen - '0';
				break;

			case '/':
			case ' ':
				rank--;
				file = FILE_A;
				fen++;
				continue;

			default:
				printf("FEN error \n");
				return -1;
		}

		for (int i = 0; i < count; i++) {
			sq64 = rank * 8 + file;
			sq120 = SQ120(sq64);
			if (piece != EMPTY) {
				ASSERT(piece >= wP && piece <= bK);
				pos->pieces[sq120] = piece;
			}
			file++;
		}
		fen++;
	}

	ASSERT(*fen == 'w' || *fen == 'b');

	pos->side = (*fen == 'w') ? WHITE : BLACK;
	fen += 2;

	for (int i = 0; i < 4; i++) {
		if (*fen == ' ')
			break;
		switch (*fen) {
			case 'K': pos->castlePerm |= WKCA; break;
			case 'Q': pos->castlePerm |= WQCA; break;
			case 'k': pos->castlePerm |= BKCA; break;
			case 'q': pos->castlePerm |= BQCA; break;
			default: break;
		}
		fen++;
	}
	fen++;

	ASSERT(pos->castlePerm >= 0 && pos->castlePerm <= 15);

	if (*fen != '-') {
		file = fen[0] - 'a';
		rank = fen[1] - '1';

		ASSERT(file >= FILE_A && file <= FILE_H);
		ASSERT(rank >= RANK_1 && rank <= RANK_8);

		pos->enPas = FR2SQ(file, rank);
	}

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

	return 0;
}

S_BOARD * GenBoard() {
	S_BOARD * board = malloc(sizeof(*board));
	//S_BOARD * board = (S_BOARD*)malloc(sizeof(S_BOARD));
		/* Only initialize things needed
		pos->pieces[BRD_SQ_NUM] = NULL;
		pos->pawns[3];
		pos->KingSq[2];
		pos->side;
		pos->enPas;
		pos->fiftyMove;
		pos->ply;
		pos->hisPly;
		pos->castlePerm;
		pos->posKey;
		pos->pceNum[13];
		pos->bigPce[2];
		pos->majPce[2];
		pos->minPce[2];
		pos->material[2];
		pos->history[MAXGAMEMOVES];
		pos->pList[13][10];
		*/
	board->HashTable->pTable = NULL;
	return board;
}

void ResetBoard(S_BOARD *pos) {
	//InitBoardValues(pos);
	
	for (int i = 0; i < BRD_SQ_NUM; i++)
		pos->pieces[i] = OFFBOARD;

	for (int i = 0; i < 64; i++)
		pos->pieces[SQ120(i)] = EMPTY;

	for (int i = 0; i < 2; i++) {
		pos->bigPce[i] = 0;
		pos->majPce[i] = 0;
		pos->minPce[i] = 0;
		pos->material[i] = 0;
	}
	
	for (int i = 0; i<3; i++)
		pos->pawns[i] = 0ULL;

	for (int i = 0; i < 13; i++)
		pos->pceNum[i] = 0;

	pos->KingSq[WHITE] = pos->KingSq[BLACK] = NO_SQ;

	pos->side = BOTH;
	pos->enPas = NO_SQ;
	pos->fiftyMove = 0;

	pos->ply = 0;
	pos->hisPly = 0;

	pos->castlePerm = 0;

	pos->posKey = 0ULL;
}

void PrintBoard(const S_BOARD *pos) {
	printf("\nGame Board:\n\n");
	int sq, piece;

	for (int rank = RANK_8; rank >= RANK_1; rank--) {
		printf("%d ", rank + 1);
		for (int file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file, rank);
			piece = pos->pieces[sq];
			printf("%3c", PceChar[piece]);
		}
		printf("\n");
	}
	
	printf("\n   ");
	for (int file = FILE_A; file <= FILE_H; file++)
		printf("%3c", 'a' + file);
	printf("\n");
	printf("side:%c\n", SideChar[pos->side]);
	printf("enPas:%d\n", pos->enPas);
	printf("castle:%c%c%c%c\n",
		pos->castlePerm&WKCA ? 'K' : '-',
		pos->castlePerm&WQCA ? 'Q' : '-',
		pos->castlePerm&BKCA ? 'k' : '-',
		pos->castlePerm&BKCA ? 'q' : '-'
	);
	printf("PosKey:%llX\n", pos->posKey);
}

void MirrorBoard(S_BOARD *pos) {

	int tempPiecesArray[64];
	int tempSide = pos->side ^ 1;
	int SwapPiece[13] = { EMPTY, bP, bN, bB, bR, bQ, bK, wP, wN, wB, wR, wQ, wK };
	int tempCastlePerm = 0;
	int tempEnPas = NO_SQ;

	int sq;
	int tp;

	if (pos->castlePerm & WKCA) tempCastlePerm |= BKCA;
	if (pos->castlePerm & WQCA) tempCastlePerm |= BQCA;

	if (pos->castlePerm & BKCA) tempCastlePerm |= WKCA;
	if (pos->castlePerm & BQCA) tempCastlePerm |= WQCA;

	if (pos->enPas != NO_SQ) {
		tempEnPas = SQ120(Mirror64[SQ64(pos->enPas)]);
	}

	for (sq = 0; sq < 64; sq++) {
		tempPiecesArray[sq] = pos->pieces[SQ120(Mirror64[sq])];
	}

	ResetBoard(pos);

	for (sq = 0; sq < 64; sq++) {
		tp = SwapPiece[tempPiecesArray[sq]];
		pos->pieces[SQ120(sq)] = tp;
	}

	pos->side = tempSide;
	pos->castlePerm = tempCastlePerm;
	pos->enPas = tempEnPas;

	pos->posKey = GeneratePosKey(pos);

	UpdateListsMaterial(pos);

	ASSERT(CheckBoard(pos));
}