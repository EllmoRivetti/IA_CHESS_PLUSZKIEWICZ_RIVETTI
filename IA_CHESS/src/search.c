/*
 *	SEARCH.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

//Used to print test data
#include <inttypes.h>


/* see the beginning of think() */
#include <setjmp.h>
jmp_buf env;
BOOL stop_search;
#undef USE_LOGS
void mylog(char* line)
{
#ifdef USE_LOGS
	FILE* f = fopen("logs", "ab");
	if (f)
	{
		int len = fwrite(line, 1, strlen(line), f);
		fclose(f);
	}
#endif
}

int lastPlayedScore;
int previousScore = MOINS_INFINI;
int lastDepth;
void checkLearning() // Fonction v�rifiant si le score a chut� et si on doit apprendre ou pas le
//score de la position � P �
{
	char tmp[1024];
	sprintf(tmp, "coup %d current %d previous %d diff %d\n", hply/2 + 1, lastPlayedScore, previousScore, lastPlayedScore - previousScore);
	mylog(tmp);
	if (lastPlayedScore < previousScore - 100) // C�est mieux d�utiliser hist_dat�
	{
		mylog("Learning\n");
		printf("On doit apprendre\n");
		takeback();
		learn(hash, lastDepth, -lastPlayedScore);
		makemove(hist_dat[hply].m.b);
		saveLearning();
	}
	previousScore = lastPlayedScore;
}
/* think() calls search() iteratively. Search statistics
   are printed depending on the value of output:
   0 = no output
   1 = normal output
   2 = xboard format output 
*/
void think(int output)
{
	int i, j, x;

	/* try the opening book first */
	pv[0][0].u = book_move();
	if (pv[0][0].u != -1)
		return;

	/* some code that lets us longjmp back here and return
	   from think() when our time is up */
	stop_search = FALSE;
	setjmp(env);
	if (stop_search) {
		
		/* make sure to take back the line we were searching */
		while (ply)
			takeback();

		checkLearning();//Calling check learning first case. Timeout
		return;
	}

	start_time = get_ms();
	stop_time = start_time + max_time;

	ply = 0;
	nodes = 0;

	memset(pv, 0, sizeof(pv));
	memset(history, 0, sizeof(history));
	InitHT();
	if (output == 1)
		printf("ply      nodes  score  time pv\n");
	for (i = 1; i <= max_depth; ++i) {
		follow_pv = TRUE;
		x = search(MOINS_INFINI, PLUS_INFINI, i);
		lastPlayedScore = x;
		lastDepth = i;
		if (output == 1)
			printf("%3d  %9lld  %5d %10.3f", i, nodes, x, (float)(get_ms() - start_time)/1000.0);
		else if (output == 2)
			printf("%d %d %lld %lld",
					i, x, (get_ms() - start_time) / 10, nodes);
		if (output) {
			for (j = 0; j < pv_length[0]; ++j)
				printf(" %s", move_str(pv[0][j].b));
			printf("\n");
			fflush(stdout);
		}
		if (x > PLUS_INFINI-1000 || x < MOINS_INFINI+1000)
			break;
	}

	//Calling check learning other case. Maxdepth atteint
	checkLearning();
}


/* search() does just that, in negamax fashion */

int search(int alpha, int beta, int depth)
{
	int i, j, x;
	BOOL c, f;

	/* we're as deep as we want to be; call quiesce() to get
	   a reasonable score and return it. */
	if (depth<=0)
		return quiesce(alpha,beta);
	++nodes;

	/* do some housekeeping every 1024 nodes */
	if ((nodes & 1023) == 0)
		checkup();

	pv_length[ply] = ply;

	/* if this isn't the root of the search tree (where we have
	   to pick a move and can't simply return 0) then check to
	   see if the position is a repeat. if so, we can assume that
	   this line is a draw and return 0. */
	if (ply && reps())
		return 0;

	/* are we too deep? */
	if (ply >= MAX_PLY - 1)
		return eval();
	if (hply >= HIST_STACK - 1)
		return eval();

	/* are we in check? if so, we want to search deeper */
	c = in_check(side);
	if (c)
		++depth;
#ifdef USE_LEARNING
	HtLearning *pLearn = getLearn(hash);
	if (ply > 0) {
		if (pLearn)
		{
			int pLearnEval = pLearn->score;
			UNSCALE_MATE_VALUE(pLearnEval);

			if (pLearn->depth >= depth)
			{
				return pLearnEval;
			}
		}
	}
#endif
#ifdef USE_HASH
	move transpositionMove;
	transpositionMove.u= 0;
	HtTyp *pTransp = getTT();
	if (pTransp)
	{
		transpositionMove = pTransp->move;
		int pTranspEval = pTransp->score;
		UNSCALE_MATE_VALUE(pTranspEval);

		if (!follow_pv && pTransp->depth >= depth)
		{
			if (pTransp->flag & FLAG_VALID)
			{
				pv_length[ply] = ply + 1;
				pv[ply][ply] = pTransp->move;
				return pTranspEval;
			}
			else
				if (pTransp->flag & FLAG_L_BOUND)
				{
					if (alpha < pTranspEval)
						alpha = pTranspEval;
				}
				else
					if (pTransp->flag & FLAG_U_BOUND)
					{
						if (beta > pTranspEval)
							beta = pTranspEval;
					}
			if (alpha >= beta)
			{
				pv_length[ply] = ply + 1;
				pv[ply][ply] = pTransp->move;
				return alpha;
			}
		}
	}
#endif
#if defined USE_BETA_FCUT || defined USE_FCUT
	int current_eval = eval();
#endif
#ifdef USE_BETA_FCUT
    // Manque le comptage des pi�ces car il vaut mieux beta fcut d�sactiver en fin de partie
	if (!follow_pv && !c && !is_mate(beta) && depth>0 && depth<=3 && current_eval > beta + depth * 220)
		return beta;
#endif
	gen();
	if (follow_pv)  /* are we following the PV? */
		sort_pv(pv[0][ply]);
#ifdef USE_HASH
#if 1
	// R�ordonnancement des coups: on joue le meilleur coup de la hash table
	else if (transpositionMove.u != 0)
	{
		// Hack pour que le coup de la hash table soit choisi en premier
		sort_pv(transpositionMove);
	}
#endif
#endif
	f = FALSE;
	int nbNonlostMoves = 0;
	move bestmove;
	bestmove.u = 0;          // Pas de meilleur coup pour le moment
	int searchAlpha = alpha; // borne alpha initiale

	/* loop through the moves */
	for (i = first_move[ply]; i < first_move[ply + 1]; ++i) {
		sort(i);
		if (!makemove(gen_dat[i].m.b))
			continue;
		f = TRUE;
#if defined USE_LMP || defined USE_LMR || defined USE_FCUT
		int givescheck = in_check(side);
#endif
#ifdef USE_FCUT
		if (!follow_pv && !c &&
			(gen_dat[i].m.b.bits & 37/* 100101 */) == 0 &&
			!givescheck && depth>= 1 && depth <= 4 && nbNonlostMoves >= 1 && current_eval < alpha - (depth-1) * 200-100)
		{
			takeback();
			continue;
	}
#endif
		int reduc = 0;
#ifdef USE_LMR
		if (!follow_pv && depth >= 3 && nbNonlostMoves >= 3 &&
			!c &&
			!givescheck &&
			(gen_dat[i].m.b.bits & 37/* 100101 */) == 0
			)
		{
			reduc = 1;
			//if ((gen_dat[i].m.b.bits & 63/* 111111 */) == 0) // Si l'on veut �tre plus prudent
			if (depth >= 6 && nbNonlostMoves >= 6)
				reduc = depth/3;
		}
#endif



#ifdef USE_LMR
		if (nbNonlostMoves >= 1)
		{
			x = -search(-alpha - 1, -alpha, depth - 1 - reduc,TRUE);
			if (x > alpha)
				x = -search(-beta, -alpha, depth - 1, TRUE);
		}
		else
			x = -search(-beta, -alpha, depth - 1, TRUE);
#else
		x = -search(-beta, -alpha, depth - 1, TRUE);
#endif
		takeback();
		if (x > MOINS_INFINI + MAX_PLY)
			nbNonlostMoves++;

		if (x > alpha) {
			bestmove = gen_dat[i].m;
#ifdef USE_HASH
			if (!stop_search) putTT(depth , x, bestmove, alpha, beta);
#endif
			/* this move caused a cutoff, so increase the history
			   value so it gets ordered high next time we can
			   search it */
			history[(int)gen_dat[i].m.b.from][(int)gen_dat[i].m.b.to] += depth;
			if (x >= beta)
			{
				history[(int)gen_dat[i].m.b.from][(int)gen_dat[i].m.b.to] += depth*depth;
				return beta;
			}
			alpha = x;

			/* update the PV */
			pv[ply][ply] = gen_dat[i].m;
			for (j = ply + 1; j < pv_length[ply + 1]; ++j)
				pv[ply][j] = pv[ply + 1][j];
			pv_length[ply] = pv_length[ply + 1];
		}
	}

	/* no legal moves? then we're in checkmate or stalemate */
	if (!f) {
		if (c)
		{
			return MOINS_INFINI + ply;
#ifdef USE_HASH
			if (!stop_search) putTT(depth, MOINS_INFINI + ply, bestmove, MOINS_INFINI, PLUS_INFINI);
#endif
		}
		else
			return 0;
	}

	/* fifty move draw rule */
	if (fifty >= 100)
	{
#ifdef USE_HASH
		if (!stop_search) putTT(depth, 0, bestmove, MOINS_INFINI, PLUS_INFINI);
#endif
		return 0;
	}

#ifdef USE_HASH
	if (!stop_search) putTT(depth, x, bestmove, searchAlpha, beta);
#endif
	return alpha;
}


/* quiesce() is a recursive minimax search function with
   alpha-beta cutoffs. In other words, negamax. It basically
   only searches capture sequences and allows the evaluation
   function to cut the search off (and set alpha). The idea
   is to find a position where there isn't a lot going on
   so the static evaluation function will work. */

int quiesce(int alpha,int beta)
{
	int i, j, x;

	++nodes;

	/* do some housekeeping every 1024 nodes */
	if ((nodes & 1023) == 0)
		checkup();

	pv_length[ply] = ply;

	/* are we too deep? */
	if (ply >= MAX_PLY - 1)
		return eval();
	if (hply >= HIST_STACK - 1)
		return eval();
#ifdef USE_LEARNING
	HtLearning *pLearn = getLearn(hash);
	if (ply > 0) {
		if (pLearn)
		{
			int pLearnEval = pLearn->score;
			UNSCALE_MATE_VALUE(pLearnEval);
			return pLearnEval;
		}
	}
#endif
#ifdef USE_HASH
	HtTyp *pTransp = getTT();
	if (pTransp)
	{
		int pTranspEval = pTransp->score;
		UNSCALE_MATE_VALUE(pTranspEval);

		if (pTransp->flag & FLAG_VALID)
		{
			pv_length[ply] = ply + 1;
			pv[ply][ply] = pTransp->move;
			return pTranspEval;
		}
		else
			if (pTransp->flag & FLAG_L_BOUND)
			{
				if (alpha < pTranspEval)
					alpha = pTranspEval;
			}
			else
				if (pTransp->flag & FLAG_U_BOUND)
				{
					if (beta > pTranspEval)
						beta = pTranspEval;
				}
		if (alpha >= beta)
		{
			pv_length[ply] = ply + 1;
			pv[ply][ply] = pTransp->move;
			return alpha;
		}
	}
#endif
	/* check with the evaluation function */
	x = eval();
	if (x >= beta)
		return beta;
	if (x > alpha)
		alpha = x;

	gen_caps();
	if (follow_pv)  /* are we following the PV? */
		sort_pv(pv[0][ply]);

	/* loop through the moves */
	for (i = first_move[ply]; i < first_move[ply + 1]; ++i) {
		sort(i);
		if (!makemove(gen_dat[i].m.b))
			continue;
		x = -quiesce(-beta, -alpha);
		takeback();
		if (x > alpha) {
			if (x >= beta)
				return beta;
			alpha = x;

			/* update the PV */
			pv[ply][ply] = gen_dat[i].m;
			for (j = ply + 1; j < pv_length[ply + 1]; ++j)
				pv[ply][j] = pv[ply + 1][j];
			pv_length[ply] = pv_length[ply + 1];
		}
	}
	return alpha;
}


/* reps() returns the number of times the current position
   has been repeated. It compares the current value of hash
   to previous values. */

int reps()
{
	int i;
	int r = 0;

	for (i = hply - fifty; i < hply; ++i)
		if (hist_dat[i].hash == hash)
			++r;
	return r;
}


/* sort_pv() is called when the search function is following
   the PV (Principal Variation). It looks through the current
   ply's move list to see if the PV move is there. If so,
   it adds 10,000,000 to the move's score so it's played first
   by the search function. If not, follow_pv remains FALSE and
   search() stops calling sort_pv(). */

void sort_pv(move m)
{
	int i;

	follow_pv = FALSE;
	for(i = first_move[ply]; i < first_move[ply + 1]; ++i)
		if (gen_dat[i].m.u == m.u) {
			follow_pv = TRUE;
			gen_dat[i].score += 10000000;
			return;
		}
}


/* sort() searches the current ply's move list from 'from'
   to the end to find the move with the highest score. Then it
   swaps that move and the 'from' move so the move with the
   highest score gets searched next, and hopefully produces
   a cutoff. */

void sort(int from)
{
	int i;
	int bs;  /* best score */
	int bi;  /* best i */
	gen_t g;

	bs = -1;
	bi = from;
	for (i = from; i < first_move[ply + 1]; ++i)
		if (gen_dat[i].score > bs) {
			bs = gen_dat[i].score;
			bi = i;
		}
	g = gen_dat[from];
	gen_dat[from] = gen_dat[bi];
	gen_dat[bi] = g;
}


/* checkup() is called once in a while during the search. */

void checkup()
{
	/* is the engine's time up? if so, longjmp back to the
	   beginning of think() */
	if (get_ms() >= stop_time) {
		stop_search = TRUE;
		longjmp(env, 0);
	}
}
