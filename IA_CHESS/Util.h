#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "defs.h"
#include "data.h"
#include "protos.h"
#include <sys/timeb.h>



/* Permet de savoir combien de move sont sauvegardé dans le fichier */
void AssertMovesCount();
HtTyp* getTT();
void putTT(int depth, int eval, move m, int alpha, int beta);
void InitHT();
HashType Aleatoire64(void);
HtLearning* getLearn(HashType hash);
void learn(HashType hash, int depth, int eval);
void initHTLearning();
void restoreLearning();
void saveLearning();
void testLearn();

#endif // UTIL_H

