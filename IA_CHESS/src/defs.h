/*
 *	DEFS.H
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


#ifndef DEFS_H
#define DEFS_H

 // Liste des fonctionalites actives ---------------------------------------------------
	 // Piece list
		#define USE_PIECE_LIST
		#define USE_PIECE_LIST_GEN

	// Hash de Zobrist
		#define USE_HASH
		#define USE_FAST_HASH
		#define USE_LEARNING

	// Alpha pruning
		#define USE_FCUT
		#define USE_LMR
// ------------------------------------------------------------------------------------


#define BOOL			int
#define TRUE			1
#define FALSE			0

#define GEN_STACK		1120
#define MAX_PLY			128
#define HIST_STACK		600

#define LIGHT			0
#define DARK			1

#define PAWN			0
#define KNIGHT			1
#define BISHOP			2
#define ROOK			3
#define QUEEN			4
#define KING			5

#define EMPTY			6

/* useful squares */
enum square
{
	A8,	B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1
};

#define ROW(x)			(x >> 3)
#define COL(x)			(x & 7)


/* This is the basic description of a move. promote is what
   piece to promote the pawn to, if the move is a pawn
   promotion. bits is a bitfield that describes the move,
   with the following bits:

   1	capture
   2	castle
   4	en passant capture
   8	pushing a pawn 2 squares
   16	pawn move
   32	promote

   It's union'ed with an integer so two moves can easily
   be compared with each other. */

typedef struct {
	char from;
	char to;
	char promote;
	char bits;
} move_bytes;

typedef union {
	move_bytes b;
	int u;
} move;

/* an element of the move stack. it's just a move with a
   score, so it can be sorted by the search functions. */
typedef struct {
	move m;
	int score;
} gen_t;

typedef unsigned long long HashType; // Type pour les hash
/* an element of the history stack, with the information
   necessary to take a move back. */
typedef struct {
	move m;
	int captureBoard; // Capture r�guli�re
	int captureEp;    // Capture de la pi�ce en passant			
	int capture;
	int castle;
	int ep;
	int fifty;
	HashType hash;
} hist_t;

// Fonction d'ASSERT perso
#ifdef _DEBUG
#define ASSERT(a) if (!(a)) {int n=0; n/=n;};
#else
#define ASSERT(a) ;
#endif

#define PIECE_DEAD (-1)

#define HT_SIZE 10000000
#define HTLearning_SIZE 100000


#define PLUS_INFINI (10000)
#define MOINS_INFINI (-10000)

#define SCALE_MATE_VALUE(value) \
{\
	if (value > PLUS_INFINI-MAX_PLY) \
	{value += (ply);};\
	if (value < MOINS_INFINI+MAX_PLY) \
	{value -= (ply);};\
}

#define UNSCALE_MATE_VALUE(value) \
{\
	if (value > PLUS_INFINI-MAX_PLY) \
		value -= (ply);\
	if (value < MOINS_INFINI+MAX_PLY) \
		value += (ply);\
}

#define FLAG_VALID				1
#define FLAG_L_BOUND			2 // Eval is the lower bound, real eval should be higher
#define FLAG_U_BOUND			4


typedef struct
{
	HashType hash;
	move	move;     // est utilise pour le tri des coups (pas indispensable dans 1 premier temps)
	short   score;
	unsigned char flag;     // indique si le score est vrai, inferieur ou superieur
	unsigned char depth;    // le nombre de coup avant la quiescence.
} HtTyp;


// DM3 - learning
typedef struct
{
	HashType hash; // Hash de la position
	short score; // Score de la position
	 // (toujours exacte car prise � la racine
	unsigned char depth; // Profondeur de la recherche.
} HtLearning;
	 
#endif // DEFS_H