#ifndef __TERM_CONTROL_H__
#define __TERM_CONTROL_H__

#include <stdint.h>

/*
 * display constants and ANSI control sequences
 */

// Terminal control Sequences

#define CLEAR       "\033[2J"   // Clear the screen
#define CURS_START  "\033[H"	// Move the cursor to the upper-left corner of the screen
#define CURS_HIDE   "\033[?25l" // Hide the cursor
#define CURS_SHOW   "\033[?25h" // Show the cursor
#define DEL_LINE    "\033[K"    // Delete everything from the cursor to the end of the line.
#define CURS_SAVE   "\033[s"    // save cursor pos'n
#define CURS_UNSAVE "\033[u"    // unsave cursor pos'n

#define CURS_BWD    "\033[D"    // move cursor backwards

#define CURS_MOV    "\033[%u;%uH" // printf str format


// Code Effects
#define COL_RST     "\033[0m"   // Reset special formatting
#define COL_BLK     "\033[30m"
#define COL_RED     "\033[31m"
#define COL_GRN     "\033[32m"
#define COL_YEL     "\033[33m"
#define COL_BLU     "\033[34m"
#define COL_MAG     "\033[35m"
#define COL_CYN     "\033[36m"
#define COL_WHT     "\033[37m"

#define BLD         "\033[1m"

#endif
