/* #define _SVID_SOURCE */
/* #define _POSIX_C_SOURCE 200809L */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
/* #include <locale.h> */
/* #include <regex.h> */
/* #include <ctype.h> */

#define COLUMNS 8
#define ROWS 8
#define SQUARES ((ROWS) * (COLUMNS))
#define TRUE 1
#define FALSE 0
typedef uint8_t boolean;
typedef uint32_t pieces_t;
typedef pieces_t square_t;

struct state_t {
    pieces_t black;
    pieces_t black_kings;    
    pieces_t white;
    pieces_t white_kings;
    uint8_t moves;
};

#define MASK(square) ((square_t)1 << (square))
#define PLACE(pieces, square) do { ((pieces) |= MASK(square)); } while(0)
#define CLEAR(pieces, square) do { ((pieces) &= ~MASK(square)); } while(0)
#define OCCUPY(pieces, square) (pieces & MASK(square))
#define MOVE(pieces, from, to) do { PLACE(pieces, to); CLEAR(pieces, from); } while(0)
#define ROW(square) (((square) / 4) - 1)

void state_init(struct state_t* state) {
    memset(state, 0, sizeof(*state));
}

int black_move(struct state_t* state) {
    return (state->moves & 1) == 0; // even moves black to move
}

void __print_board(FILE* file, struct state_t* state) {
    int8_t r;
    int8_t c;
    for (r = ROWS - 1; r >= 0; --r) {
        printf("\n    ---------------------------------\n    |");
        for (c = COLUMNS - 1; c >= 0; --c) {
            if (OCCUPY(state->black, r*4+c)) {
                fprintf(file, " b |");
            } else if (OCCUPY(state->black_kings, r*4+c)) {
                fprintf(file, " B |");
            } else if (OCCUPY(state->white, r*4+c)) {
                fprintf(file, " w |");
            } else if (OCCUPY(state->white_kings, r*4+c)) {
                fprintf(file, " W |");
            } else {
                fprintf(file, "   |");
            }
        }
    }
    printf("\n    ---------------------------------\n");       
}

#define print_board(state) __print_board(stdout, state);

int main(int argc, char **argv) {
    struct state_t state;
    state_init(&state);
    print_board(&state);
    return 0;
}
