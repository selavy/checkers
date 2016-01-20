#include <stdio.h>
#include <stdint.h>
#include <regex.h>
#include <stdlib.h>
#include <ctype.h>

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

struct move_t {
    char x1;
    char y1;
    char x2;
    char y2;
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

/* char toupper(char c) { */
/*     if (c >= 'a' && c <= 'z') { */
/*         c -= ('a' - 'A'); */
/*     } */
/*     return c; */
/* } */

int ask_for_move(struct state_t* state, struct move_t* move) {
    char* line = 0;
    size_t n = 0;
    int ret;
    regex_t preg;
    regmatch_t match[3];

    /* Move: A1 B2 */
    if ((ret = regcomp(&preg, "([a-hA-H][1-8]) ([a-hA-H][1-8])", REG_EXTENDED)) < 0) {
        printf("regcomp failed!\n");
        regerror(ret, &preg, line, n);
        printf("Regex failed, error: %s\n", line);
        free(line);
        exit(0);
    }    

    printf("Enter a move: ");
    if (getline(&line, &n, stdin) < 1) {
        printf("getline failed!\n");
        exit(0);
    }

    printf("You entered: %s\n", line);

    if ((ret = regexec(&preg, line, 3, &(match[0]), 0)) != 0) {
        printf("Invalid input!");
        return -1;
    }

    move->x1 = toupper(line[match[1].rm_so]) - 'A';
    move->y1 = line[match[1].rm_eo - 1] - '0';
    move->x2 = toupper(line[match[2].rm_so]) - 'A';
    move->y2 = line[match[2].rm_eo - 1] - '0';
    return 0;
}

int main(int argc, char **argv) {
    struct move_t move;
    struct state_t state;
    state_init(state);
    print_board(&state);
    printf("\nDone.\n");

    ask_for_move(&state, &move);
    printf("Move: (%d, %d) -> (%d, %d)\n", move.x1, move.y1, move.x2, move.y2);
    return 0;
}
