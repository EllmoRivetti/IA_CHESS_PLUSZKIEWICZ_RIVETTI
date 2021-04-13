#include "src\Util.h"

void AssertMovesCount()
{
	int moveCount = 0, i = 0;
	for (; i < HTLearning_SIZE;)
	{
		HtLearning e = HTLearning[i];
		if (e.hash != 0)
			moveCount++;
		++i;
	}
	printf("Saved moves count: %d\n", moveCount);
}

HtTyp* getTT()
{
	HtTyp* pTransp;
	pTransp = &HT[hash % HT_SIZE];

	if (pTransp->hash == hash)
		return pTransp;
	return NULL;
}

void putTT(int depth, int eval, move m, int alpha, int beta)
{
	HtTyp* pTransp;
	pTransp = &HT[hash % HT_SIZE];

	if (pTransp->depth <= depth)
	{
		pTransp->hash = hash;
		pTransp->depth = depth;
		pTransp->move = m;

		if (eval <= alpha)
		{
			pTransp->flag = FLAG_U_BOUND;
			SCALE_MATE_VALUE(eval);
			pTransp->score = eval;
		}
		else
		{
			if (eval >= beta)
			{
				pTransp->flag = FLAG_L_BOUND;
				SCALE_MATE_VALUE(eval);
				pTransp->score = eval;
			}
			else
			{
				SCALE_MATE_VALUE(eval);
				pTransp->score = eval;
				pTransp->flag = FLAG_VALID;
			}
		}
	}
}

void InitHT()
{
	memset(HT, 0, sizeof(HT));
}

HashType Aleatoire64(void)
{
	unsigned long ul1, ul2;
	static unsigned long y[55];
	static int j, k, initAl = 1;
	if (initAl) {
		int i;
		initAl = 0;
		for (i = 0; i < 55; i++) y[i] = x[i];
		j = 24 - 1;
		k = 55 - 1;
	}
	ul1 = (y[k] += y[j]);
	if (--j < 0) j = 55 - 1;
	if (--k < 0) k = 55 - 1;

	ul2 = (y[k] += y[j]);
	if (--j < 0) j = 55 - 1;
	if (--k < 0) k = 55 - 1;

	HashType val = (HashType)ul1 << 32 | ul2;
	return(val);
}

HtLearning* getLearn(HashType hash)
{
	HtLearning* pLearn;
	pLearn = &HTLearning[hash % (HTLearning_SIZE)];

	if (pLearn->hash == hash)
		return pLearn;
	return NULL;
}
void learn(HashType hash, int depth, int eval)
{
	HtLearning* pLearn;
	pLearn = &HTLearning[hash % (HTLearning_SIZE)];

	if (pLearn->depth <= depth)
	{
		pLearn->hash = hash;
		pLearn->depth = depth;
		SCALE_MATE_VALUE(eval);
		pLearn->score = eval;
	}
}

void initHTLearning()
{
	memset(HTLearning, 0, sizeof(HTLearning));
}

void restoreLearning()
{
	FILE* f = fopen("saved_moves.bin", "rb");
	if (f) {
		fread(HTLearning, sizeof(HTLearning), 1, f);
		fclose(f);
	}
}

void saveLearning()
{
	FILE* f = fopen("saved_moves.bin", "wb");
	if (f) {
		fwrite(HTLearning, sizeof(HTLearning), 1, f);
		fclose(f);
	}
}

void testLearn()
{
	learn(1, 10, 100);
	initHTLearning();
	//hash = 1; // Hack pour test
	HtLearning* pLearn = getLearn(1);
	ASSERT(
		pLearn->hash == 1 && 
		pLearn->depth == 10 && 
		pLearn->score == 100);

	learn(2, 20, 200);
	initHTLearning();
	//hash = 2; // Hack pour test
	pLearn = getLearn(2);
	ASSERT(
		pLearn->hash == 2 && 
		pLearn->depth == 20 && 
		pLearn->score == 200);

	//hash = 1; // Hack pour test
	pLearn = getLearn(1);
	ASSERT(
		pLearn->hash == 1 && 
		pLearn->depth == 10 && 
		pLearn->score == 100);
}

