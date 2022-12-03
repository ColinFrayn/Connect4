// connect.h
// Include file for connect.c
// Connect4 project
// Colin Frayn, December 2002
// cmf29@cam.ac.uk

//  Boolean values, in case not already defined
#ifndef TRUE
# define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

/* Various scoring parameters */
#define WIN        (100)
#define DRAW_SCORE (0)
#define NO_MOVE    (-1)

/* Datatypes */
#ifdef _MSC_VER
#define longlong __int64
#else
#define longlong long long int
#endif
typedef unsigned longlong BITBOARD;
#ifndef BOOL
typedef int BOOL;
#endif

/* Miscellaneous Board defines */
#define UNIT   ((BITBOARD)1)
#define EMPTY  ((BITBOARD)0)

/* The board structure */
typedef struct {
  BITBOARD p1, p2;
  int count[7];
}Board;

#define MAX_MEM (1<<28)

/* Hashtable element */
typedef struct {
  BITBOARD p1,p2; // Actual board values
  char depth;     // Depth to which it was searched
  char type;      // The entry type (bits 1,2) and side to play (bit 3)
  char score;     // Recorded score
  char move;      // Best move
}Hash;

#define LOWER_BOUND  (1)
#define UPPER_BOUND  (2)
#define EXACT_SCORE  (3)

#define GameEnd(x)   (((x)==WIN) || ((x)==(-WIN)))
#define HashType(x)  ((x) & 3)
#define HashSide(x)  (((x) >> 2)&1)

// Function definitions
void SetupBoard(Board *);
BOOL ParseInput(Board *, int, int);
BITBOARD CheckLine(Board *, int);
int  ScoreBoard(Board *);
void Comp(Board *, int, int, int);
int  Search(Board *, int, int, int, int, int, int);
void PrintBoard(Board *);
void PrintBitboard(BITBOARD);
Hash *CheckHash(Board *, int);
void AddHash(Board *, int, int, int, int, int);
void SetupHash(void);
void SetStartTime(void);
int  GetElapsedTime(void);

// Global variables
BITBOARD Mask14, Mask47, Mask15, Mask37, Mask16, Mask27;
BITBOARD MaskColumn, MaskRow, MaskBoard, OddRows, EvenRows;
int BestMove[50];
long int Nodes,nHash,HashStores,HashProbes,HashFound;
int TimeLeft,nMoves;
Hash *Table;

