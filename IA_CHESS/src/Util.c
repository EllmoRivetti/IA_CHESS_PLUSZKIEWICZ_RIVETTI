#include "Util.h"


/* get_ms() returns the milliseconds elapsed since midnight,
   January 1, 1970. */

BOOL ftime_ok = FALSE;  /* does ftime return milliseconds? */

long long get_ms()
{
	struct timeb timebuffer;
	ftime(&timebuffer);
	if (timebuffer.millitm != 0)
		ftime_ok = TRUE;
	return (timebuffer.time * 1000) + timebuffer.millitm;
}
/* Permet de savoir combien de move sont sauvegardé dans le fichier */

void assertMovesCount()
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
