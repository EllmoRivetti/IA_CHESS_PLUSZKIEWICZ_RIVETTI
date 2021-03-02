/*
 *	BOARD.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>				   
#include "defs.h"
#include "data.h"
#include "protos.h"

int checkBoard()
{ // Cette fonction peut être plus ou moins longue, dépendamment de vos besoins en matière de debug
	for (int i = 0; i < 64; ++i)
	{
		if (piece[i] != EMPTY && board[i] == 0)
			return 0;
		if (piece[i] == EMPTY && board[i])
			return 0;
		if (color[i] == LIGHT && (board[i] > 16||board[i]==0))
			return 0;
		if (color[i] == DARK && board[i] < 17)
			return 0;
		if (board[i] && pospiece[board[i]] != i)
			return 0;
	}
	for (int i = 1; i <= 32; ++i)
	{ 
		if (pospiece[i]!=PIECE_DEAD && board[pospiece[i]] != i)
			return 0;
		if (pospiece[i] != PIECE_DEAD && piece[pospiece[i]]== EMPTY)
			return 0;
		if (pospiece[i] != PIECE_DEAD && (color[pospiece[i]] == LIGHT && i>16|| color[pospiece[i]] == DARK && i < 17))
			return 0;
	}
	return 1;
}

void syncBoard()
{
	memset(board, 0, sizeof(board));
	memset(pospiece, PIECE_DEAD, sizeof(pospiece));

	int pieceIndexW = 2;  // 1 réservé au King blanc: accélère la fonction in_check()
	int pieceIndexB = 18; // 17 réservé au king noir: accélère la fonction in_check()
	for (int i = 0; i < 64; ++i) { // On scanne l'échiquier 
		if (color[i] != EMPTY)
		{
			if (color[i] == LIGHT) 
			{
				// On traite les pièces blanches
				if (piece[i] == KING)
				{
					pospiece[1] = i; // Index fixe pour le roi blanc
					board[i] = 1;
					continue; // On passe au suivant
				}
				ASSERT(pieceIndexW <= 16);
				pospiece[pieceIndexW] = i;
				board[i] = pieceIndexW++;
			}
			else
			{ // On traite les pièces noires
				if (piece[i] == KING) 
				{
					pospiece[17] = i;  // Index fixe pour le roi noir
					board[i] = 17;
					continue; // On passe au suivant
				}
				ASSERT(pieceIndexB <= 32);
				pospiece[pieceIndexB] = i;
				board[i] = pieceIndexB++;
			}
		}
	}
}

// Fonction permettant de dumper les valeurs 0/1 de l atable can_attack[][][]
void print_attacktables(char table[64])
{
	printf("\n8 ");
	for (int i = 0; i < 64; ++i) {
		if (table[i])
			printf("1 ");
		else
			printf("0 ");
		if ((i + 1) % 8 == 0 && i != 63)
			printf("\n%d ", 7 - ROW(i));
	}
	printf("\n\n   a b c d e f g h\n\n");
}


void init_attack_table()
{
	memset(can_attack,0,sizeof(can_attack));

	for (int piece = 1; piece < 6; ++piece) {
		for (int from = 0; from < 64; ++from) {
			for (int k = 0; k < offsets[piece]; ++k) {
				for (int to = from;;) {
					to = mailbox[mailbox64[to] + offset[piece][k]];
					if (to == -1)
						break;
					ASSERT(piece>=0 && piece<=6 && from>=0 && from<=63 && to>=0 && to<=63);
					can_attack[piece][from][to] = 1;
					if (!slide[piece])
						break;
				}
			}
		}
	}

	//print_attacktables(can_attack[5][10]);
}				

/* init_board() sets the board to the initial game state. */

void init_board()
{
	int i;

	for (i = 0; i < 64; ++i) {
		color[i] = init_color[i];
		piece[i] = init_piece[i];
	}
	side = LIGHT;
	xside = DARK;
	castle = 15;
	ep = -1;
	fifty = 0;
	ply = 0;
	hply = 0;
	set_hash();  /* init_hash() must be called before this function */
	first_move[0] = 0;
#ifdef USE_PIECE_LIST
	syncBoard();
	ASSERT(checkBoard());
#endif					 
}


/* init_hash() initializes the random numbers used by set_hash(). */

void init_hash()
{
	int i, j, k;

	srand(0);
	for (i = 0; i < 2; ++i)
		for (j = 0; j < 6; ++j)
			for (k = 0; k < 64; ++k)
				hash_piece[i][j][k] = hash_rand();
	hash_side = hash_rand();
	for (i = 0; i < 64; ++i)
		hash_ep[i] = hash_rand();
}


/* hash_rand() XORs some shifted random numbers together to make sure
   we have good coverage of all 32 bits. (rand() returns 16-bit numbers
   on some systems.) */

int hash_rand()
{
	int i;
	int r = 0;

	for (i = 0; i < 32; ++i)
		r ^= rand() << i;
	return r;
}


/* set_hash() uses the Zobrist method of generating a unique number (hash)
   for the current chess position. Of course, there are many more chess
   positions than there are 32 bit numbers, so the numbers generated are
   not really unique, but they're unique enough for our purposes (to detect
   repetitions of the position). 
   The way it works is to XOR random numbers that correspond to features of
   the position, e.g., if there's a black knight on B8, hash is XORed with
   hash_piece[BLACK][KNIGHT][B8]. All of the pieces are XORed together,
   hash_side is XORed if it's black's move, and the en passant square is
   XORed if there is one. (A chess technicality is that one position can't
   be a repetition of another if the en passant state is different.) */

void set_hash()
{
	int i;

	hash = 0;	
	for (i = 0; i < 64; ++i)
		if (color[i] != EMPTY)
			hash ^= hash_piece[color[i]][piece[i]][i];
	if (side == DARK)
		hash ^= hash_side;
	if (ep != -1)
		hash ^= hash_ep[ep];
}


/* in_check() returns TRUE if side s is in check and FALSE
   otherwise. It just scans the board to find side s's king
   and calls attack() to see if it's being attacked. */

BOOL in_check(int s)
{
#ifdef USE_PIECE_LIST
	if (s == LIGHT)
		return attack(pospiece[1], DARK);
	else
		return attack(pospiece[17], LIGHT);
#else					 
	int i;

	for (i = 0; i < 64; ++i)
		if (piece[i] == KING && color[i] == s)
			return attack(i, s ^ 1);
	return TRUE;  /* shouldn't get here */
#endif	  
}


/* attack() returns TRUE if square sq is being attacked by side
   s and FALSE otherwise. */

BOOL attack(int sq, int s)
{
ASSERT(checkBoard());

	int i, j, n;

#ifdef USE_PIECE_LIST
	int start;
	int end;
	if (s == DARK) { start = 17; end = 33; }
	else { start = 1;  end = 17; };

	for (int k = start; k < end; ++k)
		if (pospiece[k] != PIECE_DEAD) // Les coordonnées ne sont valides que si la pièce n'est pas "morte"
		{
			i = pospiece[k];
#else
	for (i = 0; i < 64; ++i)
	{
#endif  
		if (color[i] == s) {
			if (piece[i] == PAWN) {
				if (s == LIGHT) {
					if (COL(i) != 0 && i - 9 == sq)
						return TRUE;
					if (COL(i) != 7 && i - 7 == sq)
						return TRUE;
				}
				else {
					if (COL(i) != 0 && i + 7 == sq)
						return TRUE;
					if (COL(i) != 7 && i + 9 == sq)
						return TRUE;
				}
			}
			else
				if (can_attack[piece[i]][i][sq])
				{
					//if (!slide[piece[i]]) return TRUE;
					for (j = 0; j < offsets[piece[i]]; ++j)
						for (n = i;;) {
							n = mailbox[mailbox64[n] + offset[piece[i]][j]];
							if (n == -1)
								break;
							if (n == sq)
								return TRUE;
							if (color[n] != EMPTY)
								break;
							if (!slide[piece[i]])
								break;
						}
				}
		}
	}
	return FALSE;

}


/* gen() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack." */

void gen()
{
	int i, j, n;

	ASSERT(checkBoard());

	/* so far, we have no moves for the current ply */
	first_move[ply + 1] = first_move[ply];

#ifdef USE_PIECE_LIST_GEN
	int start;
	int end;
	if (side == DARK) { start = 17; end = 33; }
	else { start = 1;  end = 17; };

	for (int k = start; k < end; ++k)
	if (pospiece[k] != PIECE_DEAD) // Les coordonnées ne sont valides que si la pièce n'est pas "morte"
	{
		i = pospiece[k];
#else
	for (i = 0; i < 64; ++i)
	{
#endif
		if (color[i] == side) {
			if (piece[i] == PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && color[i - 9] == DARK)
						gen_push(i, i - 9, 17);
					if (COL(i) != 7 && color[i - 7] == DARK)
						gen_push(i, i - 7, 17);
					if (color[i - 8] == EMPTY) {
						gen_push(i, i - 8, 16);
						if (i >= 48 && color[i - 16] == EMPTY)
							gen_push(i, i - 16, 24);
					}
				}
				else {
					if (COL(i) != 0 && color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17);
					if (COL(i) != 7 && color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17);
					if (color[i + 8] == EMPTY) {
						gen_push(i, i + 8, 16);
						if (i <= 15 && color[i + 16] == EMPTY)
							gen_push(i, i + 16, 24);
					}
				}
			}
			else
				for (j = 0; j < offsets[piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[piece[i]][j]];
						if (n == -1)
							break;
						if (color[n] != EMPTY) {
							if (color[n] == xside)
								gen_push(i, n, 1);
							break;
						}
						gen_push(i, n, 0);
						if (!slide[piece[i]])
							break;
					}
		}
	}

	/* generate castle moves */
	if (side == LIGHT) {
		if (castle & 1)
			gen_push(E1, G1, 2);
		if (castle & 2)
			gen_push(E1, C1, 2);
	}
	else {
		if (castle & 4)
			gen_push(E8, G8, 2);
		if (castle & 8)
			gen_push(E8, C8, 2);
	}
	
	/* generate en passant moves */
	if (ep != -1) {
		if (side == LIGHT) {
			if (COL(ep) != 0 && color[ep + 7] == LIGHT && piece[ep + 7] == PAWN)
				gen_push(ep + 7, ep, 21);
			if (COL(ep) != 7 && color[ep + 9] == LIGHT && piece[ep + 9] == PAWN)
				gen_push(ep + 9, ep, 21);
		}
		else {
			if (COL(ep) != 0 && color[ep - 9] == DARK && piece[ep - 9] == PAWN)
				gen_push(ep - 9, ep, 21);
			if (COL(ep) != 7 && color[ep - 7] == DARK && piece[ep - 7] == PAWN)
				gen_push(ep - 7, ep, 21);
		}
	}
}


/* gen_caps() is basically a copy of gen() that's modified to
   only generate capture and promote moves. It's used by the
   quiescence search. */

void gen_caps()
{
	int i, j, n;

	first_move[ply + 1] = first_move[ply];
#ifdef USE_PIECE_LIST_GEN
	int start;
	int end;
	if (side == DARK) { start = 17; end = 33; }
	else { start = 1;  end = 17; };

	for (int k = start; k < end; ++k)
	if (pospiece[k] != PIECE_DEAD) // Les coordonnées ne sont valides que si la pièce n'est pas "morte"
	{
		i = pospiece[k];
#else
	for (i = 0; i < 64; ++i)
	{
#endif
		if (color[i] == side) {
			if (piece[i] == PAWN) {
				if (side == LIGHT) {
					if (COL(i) != 0 && color[i - 9] == DARK)
						gen_push(i, i - 9, 17);
					if (COL(i) != 7 && color[i - 7] == DARK)
						gen_push(i, i - 7, 17);
					if (i <= 15 && color[i - 8] == EMPTY)
						gen_push(i, i - 8, 16);
				}
				if (side == DARK) {
					if (COL(i) != 0 && color[i + 7] == LIGHT)
						gen_push(i, i + 7, 17);
					if (COL(i) != 7 && color[i + 9] == LIGHT)
						gen_push(i, i + 9, 17);
					if (i >= 48 && color[i + 8] == EMPTY)
						gen_push(i, i + 8, 16);
				}
			}
			else
				for (j = 0; j < offsets[piece[i]]; ++j)
					for (n = i;;) {
						n = mailbox[mailbox64[n] + offset[piece[i]][j]];
						if (n == -1)
							break;
						if (color[n] != EMPTY) {
							if (color[n] == xside)
								gen_push(i, n, 1);
							break;
						}
						if (!slide[piece[i]])
							break;
					}
		}
	}
	if (ep != -1) {
		if (side == LIGHT) {
			if (COL(ep) != 0 && color[ep + 7] == LIGHT && piece[ep + 7] == PAWN)
				gen_push(ep + 7, ep, 21);
			if (COL(ep) != 7 && color[ep + 9] == LIGHT && piece[ep + 9] == PAWN)
				gen_push(ep + 9, ep, 21);
		}
		else {
			if (COL(ep) != 0 && color[ep - 9] == DARK && piece[ep - 9] == PAWN)
				gen_push(ep - 9, ep, 21);
			if (COL(ep) != 7 && color[ep - 7] == DARK && piece[ep - 7] == PAWN)
				gen_push(ep - 7, ep, 21);
		}
	}
}


/* gen_push() puts a move on the move stack, unless it's a
   pawn promotion that needs to be handled by gen_promote().
   It also assigns a score to the move for alpha-beta move
   ordering. If the move is a capture, it uses MVV/LVA
   (Most Valuable Victim/Least Valuable Attacker). Otherwise,
   it uses the move's history heuristic value. Note that
   1,000,000 is added to a capture move's score, so it
   always gets ordered above a "normal" move. */

void gen_push(int from, int to, int bits)
{
	gen_t *g;
	
	if (bits & 16) {
		if (side == LIGHT) {
			if (to <= H8) {
				gen_promote(from, to, bits);
				return;
			}
		}
		else {
			if (to >= A1) {
				gen_promote(from, to, bits);
				return;
			}
		}
	}
	g = &gen_dat[first_move[ply + 1]++];
	g->m.b.from = (char)from;
	g->m.b.to = (char)to;
	g->m.b.promote = 0;
	g->m.b.bits = (char)bits;
	if (color[to] != EMPTY)
		g->score = 1000000 + (piece[to] * 10) - piece[from];
	else
		g->score = history[from][to];
}


/* gen_promote() is just like gen_push(), only it puts 4 moves
   on the move stack, one for each possible promotion piece */

void gen_promote(int from, int to, int bits)
{
	int i;
	gen_t *g;
	
	for (i = KNIGHT; i <= QUEEN; ++i) {
		g = &gen_dat[first_move[ply + 1]++];
		g->m.b.from = (char)from;
		g->m.b.to = (char)to;
		g->m.b.promote = (char)i;
		g->m.b.bits = (char)(bits | 32);
		g->score = 1000000 + (i * 10);
	}
}


/* makemove() makes a move. If the move is illegal, it
   undoes whatever it did and returns FALSE. Otherwise, it
   returns TRUE. */

BOOL makemove(move_bytes m)
{
	
	/* test to see if a castle move is legal and move the rook
	   (the king is moved with the usual move code later) */
	if (m.bits & 2) {
		int from, to;

		if (in_check(side))
			return FALSE;
		switch (m.to) {
			case 62:
				if (color[F1] != EMPTY || color[G1] != EMPTY ||
						attack(F1, xside) || attack(G1, xside))
					return FALSE;
				from = H1;
				to = F1;
				break;
			case 58:
				if (color[B1] != EMPTY || color[C1] != EMPTY || color[D1] != EMPTY ||
						attack(C1, xside) || attack(D1, xside))
					return FALSE;
				from = A1;
				to = D1;
				break;
			case 6:
				if (color[F8] != EMPTY || color[G8] != EMPTY ||
						attack(F8, xside) || attack(G8, xside))
					return FALSE;
				from = H8;
				to = F8;
				break;
			case 2:
				if (color[B8] != EMPTY || color[C8] != EMPTY || color[D8] != EMPTY ||
						attack(C8, xside) || attack(D8, xside))
					return FALSE;
				from = A8;
				to = D8;
				break;
			default:  /* shouldn't get here */
				from = -1;
				to = -1;
				break;
		}
#ifdef USE_PIECE_LIST
		// On bouge la tour
		ASSERT(board[from]>0 && board[from]<=32);
		ASSERT(from>=0 && from<64 && to>=0 && to<64)
		pospiece[board[from]]=to;
		board[to] = board[from];
		board[from] = 0;
#endif  
		color[to] = color[from];
		piece[to] = piece[from];
		color[from] = EMPTY;
		piece[from] = EMPTY;
	}

	/* back up information so we can take the move back later. */
	hist_dat[hply].m.b = m;
	hist_dat[hply].capture = piece[(int)m.to];
	hist_dat[hply].castle = castle;
	hist_dat[hply].ep = ep;
	hist_dat[hply].fifty = fifty;
	hist_dat[hply].hash = hash;
#ifdef USE_PIECE_LIST
	hist_dat[hply].captureBoard = board[m.to];
#endif
	++ply;
	++hply;

	/* update the castle, en passant, and
	   fifty-move-draw variables */
	castle &= castle_mask[(int)m.from] & castle_mask[(int)m.to];
	if (m.bits & 8) {
		if (side == LIGHT)
			ep = m.to + 8;
		else
			ep = m.to - 8;
	}
	else
		ep = -1;
	if (m.bits & 17)
		fifty = 0;
	else
		++fifty;

	/* move the piece */
	color[(int)m.to] = side;
	if (m.bits & 32)
		piece[(int)m.to] = m.promote;
	else
		piece[(int)m.to] = piece[(int)m.from];
	color[(int)m.from] = EMPTY;
	piece[(int)m.from] = EMPTY;
#ifdef USE_PIECE_LIST
	ASSERT(board[m.from] > 0 && board[m.from] <= 32);
	pospiece[board[m.from]]=m.to;
	if (board[m.to]) pospiece[board[m.to]] = PIECE_DEAD;
	board[m.to] = board[m.from];
	board[m.from] = 0;
#endif  

	/* erase the pawn if this is an en passant move */
	if (m.bits & 4) {
		if (side == LIGHT) {
			color[m.to + 8] = EMPTY;
			piece[m.to + 8] = EMPTY;
#ifdef USE_PIECE_LIST
			ASSERT(board[m.to + 8]);
			ASSERT(hply>=1);
			hist_dat[hply - 1].captureEp = board[m.to + 8];
			pospiece[board[m.to+8]] = PIECE_DEAD;
			board[m.to+8] = 0;
#endif  
		}
		else {
			color[m.to - 8] = EMPTY;
			piece[m.to - 8] = EMPTY;
#ifdef USE_PIECE_LIST
			ASSERT(board[m.to - 8]);
			ASSERT(hply >= 1);
			hist_dat[hply - 1].captureEp = board[m.to - 8];
			pospiece[board[m.to - 8]] = PIECE_DEAD;
			board[m.to - 8] = 0;
#endif
		}
	}

	/* switch sides and test for legality (if we can capture
	   the other guy's king, it's an illegal position and
	   we need to take the move back) */
	side ^= 1;
	xside ^= 1;
	if (in_check(xside)) {
		takeback();
		return FALSE;
	}
	set_hash();
	return TRUE;
}


/* takeback() is very similar to makemove(), only backwards :)  */

void takeback()
{
	move_bytes m;

	side ^= 1;
	xside ^= 1;
	--ply;
	--hply;
	m = hist_dat[hply].m.b;
	castle = hist_dat[hply].castle;
	ep = hist_dat[hply].ep;
	fifty = hist_dat[hply].fifty;
	hash = hist_dat[hply].hash;
	color[(int)m.from] = side;
	if (m.bits & 32)
		piece[(int)m.from] = PAWN;
	else
		piece[(int)m.from] = piece[(int)m.to];
#ifdef USE_PIECE_LIST
	ASSERT(board[m.to]>=1 && board[m.to]<=32);
	pospiece[board[m.to]]=m.from;
	board[m.from] = board[m.to];
	board[m.to] = hist_dat[hply].captureBoard;
	ASSERT(board[m.to] >= 0 && board[m.to] <= 32);
	if (board[m.to]) pospiece[board[m.to]]=m.to;
#endif
	if (hist_dat[hply].capture == EMPTY) {
		color[(int)m.to] = EMPTY;
		piece[(int)m.to] = EMPTY;
	}
	else {
		color[(int)m.to] = xside;
		piece[(int)m.to] = hist_dat[hply].capture;
	}
	if (m.bits & 2) {
		int from, to;

		switch(m.to) {
			case 62:
				from = F1;
				to = H1;
				break;
			case 58:
				from = D1;
				to = A1;
				break;
			case 6:
				from = F8;
				to = H8;
				break;
			case 2:
				from = D8;
				to = A8;
				break;
			default:  /* shouldn't get here */
				from = -1;
				to = -1;
				break;
		}
#ifdef USE_PIECE_LIST
		// On bouge la tour
		pospiece[board[from]] = to;
		board[to] = board[from];
		board[from] = 0;
#endif
		color[to] = side;
		piece[to] = ROOK;
		color[from] = EMPTY;
		piece[from] = EMPTY;
	}
	if (m.bits & 4) {
		if (side == LIGHT) {
			color[m.to + 8] = xside;
			piece[m.to + 8] = PAWN;
#ifdef USE_PIECE_LIST
			// On restaure le pion "en passant"
			ASSERT(hist_dat[hply].capture > -1 && hist_dat[hply].capture <= 32);
			board[m.to + 8] = hist_dat[hply].captureEp;
			pospiece[board[m.to + 8]] = m.to + 8;
#endif
		}
		else {
			color[m.to - 8] = xside;
			piece[m.to - 8] = PAWN;
#ifdef USE_PIECE_LIST
			// On restaure le pion "en passant"
			ASSERT(hist_dat[hply].capture > -1 && hist_dat[hply].capture <= 32);
			board[m.to - 8] = hist_dat[hply].captureEp;
			pospiece[board[m.to - 8]] = m.to - 8;
#endif
		}
	}
}
