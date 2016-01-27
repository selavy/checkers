/* #define _SVID_SOURCE */
/* #define _POSIX_C_SOURCE 200809L */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
/* #include <locale.h> */
/* #include <regex.h> */
/* #include <ctype.h> */

#define MAX_MOVES_PER_TURN 10 /* maximum number of captures in a turn is 9 => 10 moves */
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

/* could represent moves with 6 bits / per x 10 moves = 60 bits => uint64_t */
/* but this complicates the move generation for no significant gain.        */
struct move_t {
    uint8_t moves[MAX_MOVES_PER_TURN]; /* non-move are 0, indexed from 1 */
};

#define MASK(square) ((square_t)1 << (square))
#define PLACE(pieces, square) do { ((pieces) |= MASK(square)); } while(0)
#define CLEAR(pieces, square) do { ((pieces) &= ~MASK(square)); } while(0)
#define OCCUPY(pieces, square) (pieces & MASK(square))
#define MOVE(pieces, from, to) do { PLACE(pieces, to); CLEAR(pieces, from); } while(0)
#define ROW(square) (((square) / 4) - 1)

void __state_init(struct state_t* state) {
    memset(state, 0, sizeof(*state));
}
#define state_init(state) do { memset(state, 0, sizeof(*state)); } while(0)

int __black_move(struct state_t* state) {
    return (state->moves & 1) == 0; /* even moves => black to move */
}
#define black_move(state) ((state).moves & 1 == 0)

int __white_move(struct state_t* state) {
    return state->moves & 1;
}
#define white_move(state) (!black_move(state))

void __print_board(FILE* file, struct state_t* state) {
/*
    0-indexed square numbers
    ---------------------------------
    |   |28 |   |29 |   |30 |   |31 |
    ---------------------------------
    |24 |   |25 |   |26 |   |27 |   |
    ---------------------------------
    |   |20 |   |21 |   |22 |   |23 |
    ---------------------------------
    |16 |   |17 |   |18 |   |19 |   |
    ---------------------------------
    |   |12 |   |13 |   |14 |   |15 |
    ---------------------------------
    | 8 |   | 9 |   |10 |   |11 |   |
    ---------------------------------
    |   | 4 |   | 5 |   | 6 |   | 7 |
    ---------------------------------
    | 0 |   | 1 |   | 2 |   | 3 |   |
    ---------------------------------
*/    
    int8_t r;
    int8_t c;
    int8_t skip = 1;
    int square = 28; /* upper left square, 0-indexed */
    for (r = ROWS - 1; r >= 0; --r) {
        printf("\n    ---------------------------------\n    |");
        for (c = 0; c < COLUMNS; ++c) {
            if (skip) {
                fprintf(file, "   |");
            } else {
                /* fprintf(file, " %d |", square); */
                if (OCCUPY(state->black, square)) {
                    fprintf(file, " b |");
                } else if (OCCUPY(state->black_kings, square)) {
                    fprintf(file, " B |");
                } else if (OCCUPY(state->white, square)) {
                    fprintf(file, " w |");
                } else if (OCCUPY(state->white_kings, square)) {
                    fprintf(file, " W |");
                } else {
                    fprintf(file, "   |");
                }
                ++square;
            }
            skip ^= 1;
        }
        skip ^= 1;
        square -= 8;
    }
    printf("\n    ---------------------------------\n");       
}
#define print_board(state) __print_board(stdout, state);

void __move_init(struct move_t* move) {
    memset(move, 0, sizeof(*move));
}
#define move_init(move) do { memset(move, 0, sizeof(*move)); } while(0)

#define UP_LEFT(square) ((square) + 4)
#define UP_RIGHT(square) ((square) + 5)
#define DOWN_LEFT(square) ((square) - 4)
#define DOWN_RIGHT(square) ((square) - 3)
#define JUMP_UP_LEFT(square) (UP_LEFT(UP_LEFT(square)))
#define JUMP_UP_RIGHT(square) (UP_RIGHT(UP_RIGHT(square)))
#define JUMP_DOWN_LEFT(square) (DOWN_LEFT(DOWN_LEFT(square)))
#define JUMP_DOWN_RIGHT(square) (DOWN_RIGHT(DOWN_RIGHT(square)))

void __setup_start_position(struct state_t* state) {
    state->white = 4293918720;
    state->black = 4095;
    state->white_kings = 0;
    state->black_kings = 0;
    state->moves = 0;
}

int main(int argc, char **argv) {
    struct state_t state;
    struct move_t move;
    move_init(&move);
    state_init(&state);
    __setup_start_position(&state);
    print_board(&state);

    return 0;
}
