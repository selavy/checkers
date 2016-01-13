#include <stdio.h>
#include <stdint.h>

#define BOARDSZ 64
#define TRUE 1
#define FALSE 0
typedef uint8_t boolean;
typedef uint64_t board_t;
typedef uint8_t move_t;

struct state_t {
    board_t white; /* displayed as 'x' */
    board_t black; /* displayed as 'o' */
    move_t move;
};

/* 
---------------------------------
|   | X |   | X |   | X |   | X |
---------------------------------
| X |   | X |   | X |   | X |   |
---------------------------------
|   | X |   | X |   | X |   | X |
---------------------------------
| X |   | X |   | X |   | X |   |
---------------------------------
|   | X |   | X |   | X |   | X |
---------------------------------
| X |   | X |   | X |   | X |   |
---------------------------------
|   | X |   | X |   | X |   | X |
---------------------------------
| X |   | X |   | X |   | X |   |
---------------------------------
*/

#define IS_BLACK_MOVE(move) ((move) & 1)
#define IS_WHITE_MOVE(move) (!IS_BLACK_MOVE(move))
#define IS_SET(board, square) ((board) & ((uint64_t)1 << (square)))
#define SET(board, square) (board |= ((uint64_t)1 << (square)))
#define DISPLAY(state, square)                    \
    IS_SET((state).white, (square)) ? 'x' :       \
    IS_SET((state).black, (square)) ? 'o' : ' '
#define state_init(state)                       \
    (state).white = 6172839697753047040UL;      \
    (state).black = 11163050UL;                 \
    (state).move  = 0

void print_board(struct state_t* state) {
    int i;
    printf("---------------------------------\n|");
    for (i = 0; i < BOARDSZ; ++i) {
        if (i != 0 && i % 8 == 0) {
            printf("\n---------------------------------\n|");
        }
        printf(" %c |", DISPLAY(*state, i));
    }
    printf("\n---------------------------------\n");
}

int main(int argc, char **argv) {
    struct state_t state;
    state_init(state);
    print_board(&state);
    printf("\nDone.\n");
    return 0;
}
