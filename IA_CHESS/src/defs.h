/*
 *	DEFS.H
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


#define BOOL			int
#define TRUE			1
#define FALSE			0

#define GEN_STACK		1120
#define MAX_PLY			32
#define HIST_STACK		400

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

/* an element of the history stack, with the information
   necessary to take a move back. */
typedef struct {
	move m;
	int captureBoard; // Capture régulière
	int captureEp;    // Capture de la pièce en passant												
	int capture;
	int castle;
	int ep;
	int fifty;
	int hash;
} hist_t;

// Fonction d'ASSERT perso
#ifdef _DEBUG
	#define ASSERT(a) if (!(a)) {int n=0; n/=n;};
#else
	#define ASSERT(a) ;
#endif

#define PIECE_DEAD (-1)
#define USE_PIECE_LIST // Macro permettant d'activer le rajout de l'optimisation basée sur les listes de pièces
#define USE_PIECE_LIST_GEN // Macro permettant d'activer le rajout de l'optimisation basée sur les listes de pièces pour les fonctions gen() et gen_caps(). 
                             // Cela modifie le nombre de noeuds. 