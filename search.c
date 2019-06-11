// search.c

#include "stdio.h"
#include "defs.h"

static void CheckUp(S_SEARCHINFO *info) {
	// .. Check if time up or interrupt from GUI
	if(info->timeset == TRUE && GetTimeMs() > info->stoptime) {
		info->stopped = TRUE;
	}

	ReadInput(info);
}

static void PickNextMove(int moveNum, S_MOVELIST *list) {

	S_MOVE temp;
	int bestScore = 0;
	int bestNum = moveNum;

	for (int i = moveNum; i < list->count; i++) {
		if (list->moves[i].score > bestScore) {
			bestScore = list->moves[i].score;
			bestNum = i;
		}
	}
	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}

static int IsRepetition(const S_BOARD *pos) {
	for (int i = pos->hisPly - pos->fiftyMove; i < pos->hisPly - 1; i++) { // Start from last time 50 move was updated due to pawn move
		ASSERT(i >= 0 && i < MAXGAMEMOVES);
		if (pos->posKey == pos->history[i].posKey)
			return TRUE;
	}
	return FALSE;
}

static void ClearForSearch(S_BOARD *pos, S_SEARCHINFO *info) {
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < BRD_SQ_NUM; j++) {
			pos->searchHistory[i][j] = 0;
		}
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < MAXDEPTH; j++) {
			pos->searchKillers[i][j] = 0;
		}
	}

	pos->HashTable->overWrite = 0;
	pos->HashTable->hit = 0;
	pos->HashTable->cut = 0;
	pos->ply = 0;

	info->starttime = GetTimeMs();
	info->stopped = 0;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
}

static int Quienscence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info) {
	ASSERT(CheckBoard(pos));

	if ((info->nodes & 2047) == 0) {
		CheckUp(info);
	}

	info->nodes++;

	if ((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) { // Draw, return 0
		return 0;
	}

	if (pos->ply > MAXDEPTH - 1) { // Gone too deep, just return
		return EvalPosition(pos);
	}

	int Score = EvalPosition(pos);

	if (Score >= beta) {
		return beta;
	}

	if (Score > alpha) {
		alpha = Score;
	}

	S_MOVELIST list[1];
	GenerateAllCaps(pos, list);

	int Legal = 0; // Increment on legal moves found, if still 0, checkmate or stalemate
	int OldAlpha = alpha; // Used to compare alpha to old alpha, if alpah is better, new better move was found, store that
	int BestMove = NOMOVE;
	Score = -INFINITE;

	for (int MoveNum = 0; MoveNum < list->count; MoveNum++) {

		PickNextMove(MoveNum, list);

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue; // If not a legal move
		}

		Legal++;
		Score = -Quienscence(-beta, -alpha, pos, info); // Flip for opposite side
		TakeMove(pos);

		if (Score > alpha) { // Check for cutoff
			if (Score >= beta) { // Check for beta cutoff
				if (Legal == 1) {
					info->fhf++;
				}
				info->fh++;
				return beta;
			}
			alpha = Score;
			BestMove = list->moves[MoveNum].move;
		}
	}

	return alpha;
}

static int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_SEARCHINFO *info, int DoNull) {
	ASSERT(CheckBoard(pos)); // Check board

	if (depth <= 0) {
		return Quienscence(alpha, beta, pos, info);
	}

	if ((info->nodes & 2047) == 0) {
		CheckUp(info);
	}

	info->nodes++;

	if (IsRepetition(pos) || pos->fiftyMove >= 100) { // Draw, return 0
		return 0;
	}

	if (pos->ply > MAXDEPTH - 1) { // Gone too deep, just return
		return EvalPosition(pos);
	}

	int InCheck = SqAttacked(pos->KingSq[pos->side], pos->side ^ 1, pos);
	if (InCheck == TRUE) {
		depth++;
	}

	int Score = -INFINITE;
	int PvMove = NOMOVE;

	if (ProbeHashEntry(pos, &PvMove, &Score, alpha, beta, depth) == TRUE) {
		pos->HashTable->cut++;
		return Score;
	}

	if (DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 0) && depth >= 4) {
		// DoNull only set if making a null move, not in check, only do at certain depth
		MakeNullMove(pos);
		Score = -AlphaBeta(-beta, -beta + 1, depth - 4, pos, info, FALSE);
		TakeNullMove(pos);
		if (info->stopped == TRUE) {
			return 0;
		}
		if (Score >= beta && abs(Score) < ISMATE) {
			return beta;
		}
	}

	S_MOVELIST list[1];
	GenerateAllMoves(pos, list);

	int Legal = 0; // Increment on legal moves found, if still 0, checkmate or stalemate
	int OldAlpha = alpha; // Used to compare alpha to old alpha, if alpah is better, new better move was found, store that
	int BestMove = NOMOVE;
	Score = -INFINITE;
	int BestScore = -INFINITE;

	if (PvMove != NOMOVE) {
		for (int MoveNum = 0; MoveNum < list->count; MoveNum++) {
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
			}
		}
	}

	for (int MoveNum = 0; MoveNum < list->count; MoveNum++) {

		PickNextMove(MoveNum, list);

		if (!MakeMove(pos, list->moves[MoveNum].move)) {
			continue; // If not a legal move
		}

		Legal++;
		Score = -AlphaBeta(-beta, -alpha, depth - 1, pos, info, TRUE); // Flip for opposite side
		TakeMove(pos);
		if (info->stopped == TRUE) {
			return 0;
		}

		if (Score > alpha) { // Check for cutoff
			BestScore = Score;
			BestMove = list->moves[MoveNum].move;
			if (Score >= beta) { // Check for beta cutoff
				if (Legal == 1) {
					info->fhf++;
				}
				info->fh++;

				if (!(list->moves[MoveNum].move & MFLAGCAP)) {
					pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
					pos->searchKillers[0][pos->ply] = list->moves[MoveNum].move;
				}

				StoreHashEntry(pos, BestMove, beta, HFBETA, depth);

				return beta;
			}
			alpha = Score;
			//BestMove = list->moves[MoveNum].move;
			if (!(list->moves[MoveNum].move & MFLAGCAP)) {
				pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
			}
		}
	}

	if (Legal == 0) { // Check for mate or stalemate
		if (InCheck) {
			return -INFINITE + pos->ply;
		}
		else {
			return 0;
		}
	}

	if (alpha != OldAlpha) { // Found new best move, store in hash table
		StoreHashEntry(pos, BestMove, BestScore, HFEXACT, depth);
	}
	else {
		StoreHashEntry(pos, BestMove, alpha, HFALPHA, depth);
	}

	return alpha;
}

void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info) {
	// Iterative deepening
	// for depth = 1 to maxDepth
		// search AlphaBeta

	int bestMove = NOMOVE;
	int bestScore = -INFINITE;
	int currentDepth = 0;
	int pvMoves = 0;
	int pvNum = 0;
	ClearForSearch(pos, info);

	for (currentDepth = 1; currentDepth <= info->depth; currentDepth++) {
		bestScore = AlphaBeta(-INFINITE, INFINITE, currentDepth, pos, info, TRUE);

		if (info->stopped == TRUE) {
			break;
		}

		pvMoves = GetPvLine(currentDepth, pos);
		bestMove = pos->PvArray[0];
		if (info->GAME_MODE == UCIMODE) {
			printf("info score cp %d depth %d nodes %ld time %d ",
				bestScore, currentDepth, info->nodes, GetTimeMs() - info->starttime);
		}
		else if (info->GAME_MODE == XBOARDMODE && info->POST_THINKING == TRUE) {
			printf("%d %d %d %ld ",
				currentDepth, bestScore, (GetTimeMs() - info->starttime) / 10, info->nodes);
		}
		else if (info->POST_THINKING == TRUE) {
			printf("score:%d depth:%d nodes:%ld time:%d(ms) ",
				bestScore, currentDepth, info->nodes, GetTimeMs() - info->starttime);
		}
		if (info->GAME_MODE == UCIMODE || info->POST_THINKING == TRUE) {
			pvMoves = GetPvLine(currentDepth, pos);
			printf("pv");
			for (pvNum = 0; pvNum < pvMoves; ++pvNum) {
				printf(" %s", PrMove(pos->PvArray[pvNum]));
			}
			printf("\n");
		}
	}

	if (info->GAME_MODE == UCIMODE) {
		printf("bestmove %s\n", PrMove(bestMove));
	}
	else if (info->GAME_MODE == XBOARDMODE) {
		printf("move %s\n", PrMove(bestMove));
		MakeMove(pos, bestMove);
	}
	else {
		printf("\n\n***!! BCCE makes move %s !!***\n\n", PrMove(bestMove));
		MakeMove(pos, bestMove);
		PrintBoard(pos);
	}
}