#include "defs.h"
#include "stdlib.h"

// 0000 000000000000000 000000000000000 000000000000000 000000000000000
// First line fills right 15 bits with random bits then adds
// Second line which fills 15 bits with random bits, shifts 15 left, then adds
// Third line, which fills 15 bits with random bits, shifts 30 left, then adds
// Fourth line, which fills 15 bits with random bits, shifts 45 left, then adds
// Fifth line, which fills 15 bits with random bits, ands with 0xf to delete all but first 4 bits, then shifts 60 left
#define RAND_64 (	(U64)rand() | \
					((U64)rand() << 15) | \
					((U64)rand() << 30) | \
					((U64)rand() << 45) | \
					(((U64)rand() & 0xf) << 60)	)

int Sq120ToSq64[BRD_SQ_NUM]; // For converting between 120 index array to 64 bit board
int Sq64ToSq120[64]; // For converting between 64 bit board to 120 index array

U64 SetMask[64];
U64 ClearMask[64];

U64 PieceKeys[13][120]; // Zobrist hashing
U64 SideKey;
U64 CastleKeys[16];

int FilesBrd[BRD_SQ_NUM];
int RanksBrd[BRD_SQ_NUM];

U64 FileBBMask[8];
U64 RankBBMask[8];

U64 BlackPassedMask[64];
U64 WhitePassedMask[64];
U64 IsolatedMask[64];

void InitEvalMasks() {
	int tsq;

	for (int sq = 0; sq < 8; ++sq) {
		FileBBMask[sq] = 0ULL;
		RankBBMask[sq] = 0ULL;
	}
	tsq = 0;
	for (int r = RANK_8; r >= RANK_1; r--) {
		for (int f = FILE_A; f <= FILE_H; f++) {
			int sq = r * 8 + f;
			FileBBMask[f] |= (1ULL << sq);
			RankBBMask[r] |= (1ULL << sq);
		}
	}
	tsq = 0;
	for (int sq = 0; sq < 64; ++sq) {
		IsolatedMask[sq] = 0ULL;
		WhitePassedMask[sq] = 0ULL;
		BlackPassedMask[sq] = 0ULL;
	}
	tsq = 0;
	for (int sq = 0; sq < 64; ++sq) {
		tsq = sq + 8;

		while (tsq < 64) {
			WhitePassedMask[sq] |= (1ULL << tsq);
			tsq += 8;
		}

		tsq = sq - 8;
		while (tsq >= 0) {
			BlackPassedMask[sq] |= (1ULL << tsq);
			tsq -= 8;
		}

		if (FilesBrd[SQ120(sq)] > FILE_A) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] - 1];

			tsq = sq + 7;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 9;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}

		if (FilesBrd[SQ120(sq)] < FILE_H) {
			IsolatedMask[sq] |= FileBBMask[FilesBrd[SQ120(sq)] + 1];

			tsq = sq + 9;
			while (tsq < 64) {
				WhitePassedMask[sq] |= (1ULL << tsq);
				tsq += 8;
			}

			tsq = sq - 7;
			while (tsq >= 0) {
				BlackPassedMask[sq] |= (1ULL << tsq);
				tsq -= 8;
			}
		}
	}
}

void InitFilesRanksBrd() {
	int sq = A1;
	int sq64 = 0;

	for (int i = 0; i < BRD_SQ_NUM; i++) {
		FilesBrd[i] = OFFBOARD;
		RanksBrd[i] = OFFBOARD;
	}

	for (int rank = RANK_1; rank <= RANK_8; rank++) {
		for (int file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file, rank);
			FilesBrd[sq] = file;
			RanksBrd[sq] = rank;
		}
	}
	/* Print for testing
	printf("FilesBrd\n");
	for (int i = 0; i < BRD_SQ_NUM; i++) {
		if (i % 10 == 0 && i != 0)
			printf("\n");
		printf("%4d", FilesBrd[i]);
	}
	printf("\n");
	printf("\n");
	printf("RanksBrd\n");
	for (int i = 0; i < BRD_SQ_NUM; i++) {
		if (i % 10 == 0 && i != 0)
			printf("\n");
		printf("%4d", RanksBrd[i]);
	}
	*/
}

void InitHashKey() { // Zobrist hashing, generate numbers
	for (int i = 0; i < 13; i++)
		for (int j = 0; j < 120; j++)
			PieceKeys[i][j] = RAND_64;
	SideKey = RAND_64;
	for (int i = 0; i < 16; i++)
		CastleKeys[i] = RAND_64;
}

void InitBitMasks() {
	for (int index = 0; index < 64; index++) {
		SetMask[index] = 0ULL;
		ClearMask[index] = 0ULL;
	}

	for (int index = 0; index < 64; index++) {
		SetMask[index] |= (1ULL << index);
		ClearMask[index] = ~SetMask[index];
	}
}

void InitSq120To64() { // Initialize conversion arrays
	int sq = A1;
	int sq64 = 0;
	for (int index = 0; index < BRD_SQ_NUM; index++) {
		Sq120ToSq64[index] = 65;
	}

	for (int index = 0; index < 64; index++) {
		Sq64ToSq120[index] = 120;
	}

	for (int rank = RANK_1; rank <= RANK_8; rank++) {
		for (int file = FILE_A; file <= FILE_H; file++) {
			sq = FR2SQ(file, rank);
			Sq64ToSq120[sq64] = sq;
			Sq120ToSq64[sq] = sq64;
			sq64++;
		}
	}
}

void AllInit() { // Simple function for initializing everything
	InitSq120To64();
	InitBitMasks();
	InitHashKey();
	InitFilesRanksBrd();
	InitMvvLva();
	InitEvalMasks();
}