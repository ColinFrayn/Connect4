/***************
 * ColConnect4 
 * Colin Frayn 
 * colin@frayn.net
 * December 2002 - December 2006  
 ***************/

// connect.c
// Contains the main body of the code

#include "connect.h"

#include <stdlib.h>
#include <stdio.h>

// Program algorithm switches
#define HASH_TABLE_ON
//#define TIMER_ON    // Use the timer
#define GAME_TIME      (6000)   // Maximum time for a game in centiseconds
#define TRACK_CLOCK  // Keep track of the time left
//#define AUTO_COMP    // The computer automatically replies
//#define AUTO_OPEN    // Play the opening automatically

// These two should be set from the makefile
//#define SCORING_ON   // Allow approximate scoring
//#define ALLOW_PASS   // Allow a pass move

// Define this to compile the lean (no extra output, concise code) version
// This removes all the needless fprintfs to stderr
//#define LEAN_VERSION

// Feedback during thinking
#define FEEDBACK

// Main control structure
int main(void) {
  int side=0,drawcount=0;
  static int maxdepth=100;
  char ch;
  Board B;

  // Perform the initialisation
  SetupBoard(&B);

  // Main command loop
  do {
#ifdef AUTO_COMP
    if (side==1) Comp(&B,side,drawcount,maxdepth);
    // Get user input
    else
#endif // AUTO_COMP
    {
      ch = fgetc(stdin);
      if (ch == 'q' || ch == 'Q') return -1;
#ifndef LEAN_VERSION
      if (ch == 'b' || ch == 'B') {PrintBoard(&B);continue;}
      if (ch == 'c' || ch == 'C') {Comp(&B,side,drawcount,maxdepth);continue;}
      if (ch == 's' || ch == 'S') {fprintf(stderr,"Score = %d\n",ScoreBoard(&B));continue;}
      if (ch == 'd' || ch == 'D') {fprintf(stderr,"Depth = ");fscanf_s(stdin,"%d\n",&maxdepth);continue;}
#endif // LEAN_VERSION
      BestMove[0] = (int)(ch - '0');
    }
    if (ParseInput(&B,BestMove[0],side)) {
      if (CheckLine(&B,side)) break;
      side = 1 - side;
    }
    if (BestMove[0] == 0) drawcount++;
    else drawcount = 0;
    // Break out if we've passed twice each or if we've managed to get a line!
  } while (!CheckLine(&B,side) && drawcount<4 && (~(B.p1|B.p2) | MaskBoard));
#ifndef LEAN_VERSION
  if (CheckLine(&B,0))      fprintf(stderr,"Game Ended in a win for Player 1\n");
  else if (CheckLine(&B,1)) fprintf(stderr,"Game Ended in a win for Player 2\n");
  else                      fprintf(stderr,"Game Ended in a draw\n");
  fprintf(stderr,"Press any key\n");
  // Get a keypress
  while ((ch = fgetc(stdin)) == '\n');
#endif // LEAN_VERSION
  return 1;
}

// Setup the board 
void SetupBoard(Board *B) {
  int n;

  // Set up the board itself
  B->p1 = EMPTY;
  B->p2 = EMPTY;
  // Space left in each column
  for (n=0;n<7;n++) B->count[n] = 5;
  // Set board masks
  MaskRow = EMPTY;
  MaskColumn = EMPTY;
  // Mask for a full row (top row)
  for (n=0;n<7;n++) MaskRow |= (UNIT<<n);
  // Mask for a full column (left column)
  for (n=0;n<6;n++) MaskColumn |= (UNIT<<(n*7));
  // Masks for certain column spans on the board.
  Mask14 = Mask15 = Mask16 = EMPTY;
  Mask27 = Mask37 = Mask47 = EMPTY;
  for (n=0;n<7;n++) {
    if (n<4) Mask14 |= MaskColumn << n;
    if (n<5) Mask15 |= MaskColumn << n;
    if (n<6) Mask16 |= MaskColumn << n;
    if (n>0) Mask27 |= MaskColumn << n;
    if (n>1) Mask37 |= MaskColumn << n;
    if (n>2) Mask47 |= MaskColumn << n;
    MaskBoard |= MaskColumn << n;
  }
  OddRows  = (MaskRow<<7) | (MaskRow<<21) | (MaskRow<<35);
  EvenRows = (MaskRow<<0) | (MaskRow<<14) | (MaskRow<<28);
  TimeLeft = GAME_TIME;
  // Say hello!
#ifndef LEAN_VERSION
  fprintf(stderr,"Welcome to Col's Connect 4 Program\n");
  fprintf(stderr,"----------------------------------\n\n");
#endif // LEAN_VERSION
}

// Parse a column input
// Human player is player 1 always
BOOL ParseInput(Board *B, int column, int side) {
  // Play a pass move - leave this in even if passing is
  // not allowed because of the interface rules for setting
  // that the engine is to start with the first move
  if (column == 0) return TRUE;
  // Illegal column
  if (column < 1 || column > 7) return FALSE;
  // Full column
  if (B->count[column-1] < 0) return FALSE;
  // This move is OK - play it.
  if (side == 0) B->p1 |= ((UNIT<<(column-1)) << (7*B->count[column-1]));
  else B->p2 |= ((UNIT<<(column-1)) << (7*B->count[column-1]));
  B->count[column-1]--;
  return TRUE;
}

// Check to see if there is a line on the board
// for the specified player
BITBOARD CheckLine(Board *B, int side) {
  BITBOARD p;
  // Get the side that has just played a move
  if (side==0) p = B->p1;
  else         p = B->p2;

  // Just trust me that this works :)
  return (p&(p>>7)&(p>>14)&(p>>21))          |
         (p&(p>>1)&(p>>2)&((p&Mask47)>>3))   |
         ((p&Mask14)&(p>>8)&(p>>16)&(p>>24)) |
         ((p&Mask47)&(p>>6)&(p>>12)&(p>>18));
}

// Score the board - look for threats that are (almost)
// guaranteed to win.  This is intensely painful.
int ScoreBoard(Board *B) {
  int score=0;
  BITBOARD p1 = B->p1,p2 = B->p2,threat,all = B->p1|B->p2,p,total;

  /* -- TEST FOR HORIZONTAL ROW THREATS -- */

  // Test for player 1
  p = p1 & OddRows;
  // Three in a row - just needs end play
  threat = (p&(p>>1)&(p>>2));
  total  = threat|(threat<<1)|(threat<<2)|(threat<<3)|(threat>>1);
  // Two and a one with a gap
  threat = (p|((p&Mask27)>>1)) & (p|((p&Mask16)<<1));
  threat = (threat&(threat>>1)&(threat>>2)&(threat>>3));
  total |= threat|(threat<<1)|(threat<<2)|(threat<<3);
  // Together
  total &= (~all & OddRows);
  if (total & ((~all & MaskBoard)>>7)) score+=10;

  // Repeat for player 2
  // Potential rows
  p = p2 & EvenRows;
  // Three in a row - just needs end play
  threat = (p&(p>>1)&(p>>2));
  total  = threat|(threat<<1)|(threat<<2)|(threat<<3)|(threat>>1);
  // Two and a one with a gap
  threat = (p|((p&Mask27)>>1)) & (p|((p&Mask16)<<1));
  threat = (threat&(threat>>1)&(threat>>2)&(threat>>3));
  total |= threat|(threat<<1)|(threat<<2)|(threat<<3);
  // Together
  total &= (~all & EvenRows);
  if (total & ((~all & MaskBoard)>>7)) score--;

  /* -- TEST FOR DIAGONAL THREATS A6 F1 SENSE -- */

  // Test for player 1
  // First test for 3-in-a-row diagonals with empty end squares
  threat  = (p1&(p1>>8)&(p1>>16)) & EvenRows & Mask15;
  total   = (threat<<8)|(threat<<16)|(threat<<24)|(threat>>8);
  // Two and a one with a gap
  threat = (p1|((p1&Mask27)>>8)) & (p1|((p1&Mask16)<<8));
  threat = (threat&(threat>>8)&(threat>>16)&(threat>>24));
  total |= threat|(threat<<8)|(threat<<16)|(threat<<24);
  // Together
  total  &= (~all & OddRows);
  if (total & ((~all & MaskBoard)>>7)) score+=10;

  // Repeat for player 2
  // First test for 3-in-a-row diagonals with empty end squares
  threat  = (p2&(p2>>8)&(p2>>16)) & OddRows & Mask15;
  total   = (threat<<8)|(threat<<16)|(threat<<24)|(threat>>8);
  // Two and a one with a gap
  threat = (p2|((p2&Mask27)>>8)) & (p2|((p2&Mask16)<<8));
  threat = (threat&(threat>>8)&(threat>>16)&(threat>>24));
  total |= threat|(threat<<8)|(threat<<16)|(threat<<24);
  // Together
  total  &= (~all & EvenRows);
  if (total & ((~all & MaskBoard)>>7)) score--;

  /* -- TEST FOR DIAGONAL THREATS A1 F6 SENSE -- */

  // Test for player 1
  // First test for 3-in-a-row diagonals with empty end squares
  threat  = (p1&(p1>>6)&(p1>>12)) & EvenRows & Mask37;
  total   = (threat<<6)|(threat<<12)|(threat<<18)|(threat>>6);
  // Two and a one with a gap
  threat = (p1|((p1&Mask16)>>6)) & (p1|((p1&Mask27)<<6));
  threat = (threat&(threat>>6)&(threat>>12)&(threat>>18));
  total |= threat|(threat<<6)|(threat<<12)|(threat<<18);
  // Together
  total  &= (~all & OddRows);
  if (total & ((~all & MaskBoard)>>7)) score+=10;

  // Repeat for player 2
  // First test for 3-in-a-row diagonals with empty end squares
  threat  = (p2&(p2>>6)&(p2>>12)) & OddRows & Mask37;
  total   = (threat<<6)|(threat<<12)|(threat<<18)|(threat>>6);
  // Two and a one with a gap
  threat = (p2|((p2&Mask16)>>6)) & (p2|((p2&Mask27)<<6));
  threat = (threat&(threat>>6)&(threat>>12)&(threat>>18));
  total |= threat|(threat<<6)|(threat<<12)|(threat<<18);
  // Together
  total  &= (~all & EvenRows);
  if (total & ((~all & MaskBoard)>>7)) score--;
  
  // Return the score
  return score;
}

// Do the search
void Comp(Board *B, int side, int drawcount, int maxdepth) {
  int depth = 1,score = 0, column, plays=0, lastscore, TimeTaken = 0;
  BOOL Extend = FALSE;

  BestMove[0] = 0;
  Nodes = HashProbes = HashStores = HashFound = 0;

  // Count the squares left
  for (column=0; column<7; column++) plays += (5-B->count[column]);

  // Open in the middle of the board
#ifdef AUTO_OPEN
  if (plays==0) {
    BestMove[0] = 4;
    fprintf(stdout,"%d\n",BestMove[0]);
    return;
  }
#endif // AUTO_OPEN

  // Setup the hash table
#ifdef HASH_TABLE_ON
  SetupHash();
#endif // HASH_TABLE_ON
  // Start the clock
  SetStartTime();

  // Iterative deepening loop
  do {
    depth++;
    nMoves=0;
    lastscore = score;
    // Do the search
    score = Search(B,0,depth,-WIN,WIN,side,drawcount);
    // Extend if we're in trouble
    if (score < 0 && score < lastscore) Extend = TRUE;
    else Extend = FALSE;
    // Quit out only if we've found a win/lose situation
    // or if we're out of time for this move or if we've searched
    // as far as we possibly can or we only have one good move
#ifdef TIMER_ON
    TimeTaken = GetElapsedTime();
#endif // TIMER_ON
  } while (abs(score)<WIN && (Extend||TimeTaken<(TimeLeft/25)) && (depth+plays < 42) && nMoves>1 && depth<maxdepth);

  free(Table);
#ifndef LEAN_VERSION
  fprintf(stderr,"Nodes = %d\n",Nodes);
  fprintf(stderr,"Hash Stores = %d (Total %d)\n",HashStores,nHash);
  fprintf(stderr,"Hash Probes = %d (Found %d)\n",HashProbes,HashFound);
  // Keep track of the clock
#ifdef TRACK_CLOCK
  TimeTaken = GetElapsedTime();
  fprintf(stderr,"Time Taken  = %.2f seconds\n",(float)TimeTaken / 100.0f);
#endif // TRACK_CLOCK
#ifdef TIMER_ON
  fprintf(stderr,"Time Left   = %.2f seconds\n",(float)TimeLeft / 100.0f);
  TimeLeft -= TimeTaken;
#endif // TIMER_ON
#endif // LEAN_VERSION
  // Print out the move we've chosen
  BestMove[0]++;
  fprintf(stdout,"%d\n",BestMove[0]);
}

// Do the search iteration
int Search(Board *B, int ply, int depth, int alpha, int beta, int side, int draw) {
  int column,score,bestscore=-WIN-1,firstcol=0,loop,bestcolumn=NO_MOVE;
  int cols[8] = {0,1,2,3,4,5,6,NO_MOVE};
#ifdef ALLOW_PASS
  int NumCols = 8;
#else 
  int NumCols = 7;
#endif // ALLOW_PASS
  BITBOARD p,pp,move = EMPTY;
  BOOL IsPV = FALSE;
#ifdef HASH_TABLE_ON
  Hash *H;
#endif // HASH_TABLE_ON

  // Draw by passing four times?
#ifdef ALLOW_PASS
  if (draw>=4) return DRAW_SCORE;
#endif // ALLOW_PASS

  // Board is full - return draw score
  // (We've already tested to see if this is a win)
  if ((B->p1|B->p2) == MaskBoard) return DRAW_SCORE;

  // Check that there isn't an immediate win from this position.
  // This allows us potentially to prune out vast chunks of game tree.
  // (Instead of checking them in turn in the loop below, after
  //  needlessly searching the entire game trees of the preceeding moves)
  if (side) p = B->p2;
  else      p = B->p1;

  // Loop through the seven columns
  for (column=0; column<7; column++) {
    // Check if this column is full
    if (B->count[column]<0) continue;
    // Temporarily play this move
    pp = p | ((UNIT<<column) << (7*B->count[column]));
    // Did we make a line? Just trust me that this works!
    if ( (pp&(pp>>7)&(pp>>14)&(pp>>21))          |
         (pp&(pp>>1)&(pp>>2)&((pp&Mask47)>>3))   |
         ((pp&Mask14)&(pp>>8)&(pp>>16)&(pp>>24)) |
         ((pp&Mask47)&(pp>>6)&(pp>>12)&(pp>>18))) {
      // Sort out the node count
      Nodes += column+1;
      BestMove[ply] = column;
      // Return a winning score
      return WIN;
    }
  }

  // Bottom depth - return a score as we will not search further
  if (depth == 0) {
#ifdef SCORING_ON
    if (side) return -ScoreBoard(B);
    return ScoreBoard(B);
#else
    return DRAW_SCORE;
#endif // SCORING_ON
  }

  // Check the hash table 
#ifdef HASH_TABLE_ON
  if (H = CheckHash(B,side)) {
    HashFound++;
    // Use this move if the hash element is deep enough *or* if it's an exact win/loss
    if (H->depth >= depth || abs(H->score)==WIN) {
      switch(HashType(H->type)) {
       case LOWER_BOUND: if (H->score >= beta)  return  H->score;
                         if (H->score > alpha)  alpha = H->score;
                         break;
       case UPPER_BOUND: if (H->score <= alpha) return H->score;
                         if (H->score <  beta)  beta = H->score;
                         break;
       case EXACT_SCORE: return H->score; break;
      }
    }
    // Set that we're going to use the hash move first
    if (H->move!=NO_MOVE) {
      cols[0] = H->move;
      cols[H->move] = 0;
    }  
  }
#endif // HASH_TABLE_ON

  // Loop through columns 
  for (loop = 0; loop < NumCols; loop++) {
    column = cols[loop];

    // Full column?
#ifdef ALLOW_PASS
    if (column != NO_MOVE && B->count[column]<0) continue;
#else 
    if (B->count[column]<0) continue;
#endif // ALLOW_PASS

    // Keep a node tally
    Nodes++;

    // Do the move
#ifdef ALLOW_PASS
    if (column != NO_MOVE) {
#endif // ALLOW_PASS
    move = ((UNIT<<column) << (7*B->count[column]));
    if (side) B->p2 |= move;
    else      B->p1 |= move;
    B->count[column]--;
#ifdef ALLOW_PASS
    }
#endif // ALLOW_PASS

#ifdef ALLOW_PASS
    if (column == NO_MOVE) score = -Search(B,ply+1,depth-1,-beta,-alpha,1-side,draw+1);
    else score = -Search(B,ply+1,depth-1,-beta,-alpha,1-side,0);
#else 
    score = -Search(B,ply+1,depth-1,-beta,-alpha,1-side,0);
#endif // ALLOW_PASS

#ifdef ALLOW_PASS
    if (column != NO_MOVE) {
#endif // ALLOW_PASS
    // Undo the move
    B->count[column]++;
    if (side) B->p2 ^= move;
    else      B->p1 ^= move;
#ifdef ALLOW_PASS
    }
#endif // ALLOW_PASS

    // Keep track of the number of legal, nonlosing moves on
    // ply=0.  We can quit immediately if there is only one.
    if (ply==0 && score != -WIN) nMoves++;

    // Update if we've beaten the best score
    if (score > bestscore) {
      // Record the best move
      BestMove[ply] = column;
      // Top ply - print off that we've improved the best move
      if (ply==0) {
#ifndef LEAN_VERSION
        if (depth<10) fprintf(stderr,"[%d]  Column %d  (%d)\n",depth,column+1,score);
        else fprintf(stderr,"[%d] Column %d  (%d)\n",depth,column+1,score);
#endif // LEAN_VERSION
      }
      // Test for beta (fail-high) cuts
      if (score >= beta) {
#ifdef HASH_TABLE_ON
        // Store a hash entry for this node
        if (score == WIN) AddHash(B, EXACT_SCORE, side, depth, score, column);
        else              AddHash(B, LOWER_BOUND, side, depth, score, column);
#endif // HASH_TABLE_ON
        return score;
      }
      // Test for improving alpha.  See if we're in the 
      // principal variation - used for calculating hash type (below).
      bestscore = score;
      bestcolumn = column;
      if (score > alpha) {
        alpha = score; 
        IsPV = TRUE;
      }
    }
  }
#ifdef HASH_TABLE_ON
  // Store a hash entry for this node
  if (IsPV || bestscore==-WIN) AddHash(B, EXACT_SCORE, side, depth, bestscore, bestcolumn);
  else                         AddHash(B, UPPER_BOUND, side, depth, bestscore, bestcolumn);
#endif // HASH_TABLE_ON
  // Return the best score from this node
  return bestscore;
}

#ifndef LEAN_VERSION
/* Print out the board */
void PrintBoard(Board *B) {
  int r,c;
  BITBOARD p;

  // Loop through rows
  for (r = 0 ; r < 6 ; r++) {
    // Loop through columns
    for (c = 0 ; c < 7 ; c++) {
      p = UNIT<<(r*7 + c);
      if (B->p1&p) fprintf(stderr,"O ");
      else if (B->p2&p) fprintf(stderr,"X ");
      else  fprintf(stderr,". ");
    }
    fprintf(stderr,"\n");
  }
  fprintf(stderr,"\n");
}

/* Print out a bitboard */
void PrintBitboard(BITBOARD B) {
  int r,c;
  BITBOARD p;

  // Loop through rows
  for (r = 0 ; r < 6 ; r++) {
    // Loop through columns
    for (c = 0 ; c < 7 ; c++) {
      p = UNIT<<(r*7 + c);
      if (B&p) fprintf(stderr,"X");
      else fprintf(stderr,".");
    }
    fprintf(stderr,"\n");
  }
  fprintf(stderr,"\n");
}
#endif // LEAN_VERSION

#ifdef HASH_TABLE_ON
/* Check for a hash table entry */
Hash *CheckHash(Board *B, int side) {
  Hash *H;
  BITBOARD Key;
  int iKey;

  Key  = B->p1 | (B->p2 << 1);
  // Get a hash key.  Note that this is a many-to-one function,
  // so we might yet get clashes.  In order to resolve these,
  // we also store the exact values of the two players bitboards.
  iKey = (int)(Key % nHash);
  H = &Table[iKey];
  HashProbes++;
  // See if this entry has the correct bitboards in it
  if (H->p1 == B->p1 && H->p2 == B->p2 && HashSide(H->type) == side) return H;
  return NULL;
}

/* Add a hash table entry */
void AddHash(Board *B, int type, int side, int depth, int score, int move) {
  Hash *H;
  BITBOARD Key;
  int iKey;

  // Get the key for this position
  Key  = B->p1 | (B->p2 << 1);
  iKey = (int)(Key % nHash);
  H = &Table[iKey];
  // Don't replace entries from deeper searches
  if ((int)H->depth > depth) return;
  // Don't replace exact entries with inexact ones.
  if ((int)H->depth == depth && HashType(H->type) == EXACT_SCORE && type != EXACT_SCORE) return;
  // Fine - replace the existing entry
  H->p1 = B->p1;
  H->p2 = B->p2;
  H->depth = (char)depth;
  H->score = (char)score;
  H->type  = (char)(type + (side << 2));
  H->move  = (char)move;
  HashStores++;
}

/* Allocate and reset the hash table memory at the beginning of the search */
void SetupHash(void) {
  nHash = MAX_MEM / sizeof(Hash);
  Table = (Hash *)calloc(sizeof(Hash), nHash);
}
#endif //HASH_TABLE_ON

/* Stuff needed for the timing procedure.  Specialised for MSVC / other OS. */

#ifdef _MSC_VER

#include <sys/timeb.h>
struct timeb start;

/* Set the time for the start of the computation loop */
void SetStartTime(void) {
  (void)ftime(&start);
}

/* Get the elapsed time in centiseconds so far */
int GetElapsedTime(void) {
  struct timeb end;
  int TimeTaken;

  (void)ftime(&end);
  TimeTaken  = (int)(end.time - start.time)*100;
  TimeTaken += (int)((double)(end.millitm - start.millitm) / 10.0);

  return TimeTaken;
}

#else  // Not MSVC

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

struct timeval tvalStart;

/* Set the time for the start of the computation loop */
void SetStartTime(void) {
  gettimeofday(&tvalStart, 0);
}

/* Get the elapsed time in centiseconds so far */
int GetElapsedTime(void) {
  struct timeval tvalStop;
  int TimeTaken;
  gettimeofday(&tvalStop, 0);
  TimeTaken = (tvalStop.tv_sec - tvalStart.tv_sec)*1000000;
  TimeTaken += (tvalStop.tv_usec - tvalStart.tv_usec);
  TimeTaken /= 10000;
  return TimeTaken;
}

#endif // _MSC_VER
