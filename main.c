#define _SVID_SOURCE
//#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <locale.h>
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
typedef uint64_t square_t;

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

board_t MOVES[] = {
    0,
    1280,
    0,
    5120,
    0,
    20480,
    0,
    16384,
    131072,
    0,
    655370,
    0,
    2621480,
    0,
    10485920,
    0,
    0,
    83887360,
    0,
    335549440,
    0,
    1342197760,
    0,
    1073807360,
    8589967360,
    0,
    42950328320,
    0,
    171801313280,
    0,
    687205253120,
    0,
    0,
    5497642024960,
    0,
    21990568099840,
    0,
    87962272399360,
    0,
    70373039144960,
    562952100904960,
    0,
    2814792716779520,
    0,
    11259170867118080,
    0,
    45036683468472320,
    0,
    0,
    360293467747778560,
    0,
    1441173870991114240,
    0,
    5764695483964456960,
    0,
    4611967493404098560,
    140737488355328,
    0,
    2814749767106560,
    0,
    11258999068426240,
    0,
    45035996273704960,
    0
};

#define MASK(square) ((board_t)1 << (square))
#define CAN_MOVE(to, from) (MOVES[(from)] & MASK(to))
#define BLACK_MOVE(move) ((move) & 1)
#define WHITE_MOVE(move) (!BLACK_MOVE(move))
#define IS_SET(board, square) ((board) & MASK(square))
#define SET(board, square) (board |= MASK(square))
#define CLEAR(board, square) (board &= ~MASK(square))
#define DISPLAY(state, square)                    \
    IS_SET((state).white, (square)) ? 'x' :       \
    IS_SET((state).black, (square)) ? 'o' : ' '
#define state_init(state)                       \
    (state).white = 6172839697753047040UL;      \
    (state).black = 11163050UL;                 \
    (state).move  = 0
#define VALID_MOVES 6172840429334713770UL
#define VALID(square) (((square_t)(square) & VALID_MOVES))
#define SQUARE(x, y) ((y)*8 + (x))
#define FROM_SQUARE(move) (SQUARE((move).x1, (move).y1))
#define TO_SQUARE(move) (SQUARE((move).x2, (move).y2))
#define BOARD(board) ((board_t)((board).white | (board).black))
#define OCCUPIED(square, board) (MASK(square) & BOARD(board))

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

int rpmatch(const char* line) {
    regex_t preg;
    regmatch_t match[1];
    int ret;
    char* error = 0;

    if ((ret = regcomp(&preg, "^([yY]|yes|YES)", REG_EXTENDED)) < 0) {
        regerror(ret, &preg, error, 0);
        printf("Regex failed, error: %s\n", error);
        free(error);
        exit(0);
    }

    if (regexec(&preg, line, 1, &(match[0]), 0) != 0) {
        return -1;
    } else {
        return 1;
    }
}

int valid_move(struct state_t* state, struct move_t* move) {
    int ret;
    square_t square = TO_SQUARE(*move);
    if (!VALID(square) || OCCUPIED(square, *state)) {
        printf("failed not valid or occupied\n");
        ret = 0;
    }
    else if (!CAN_MOVE(FROM_SQUARE(*move), square)) {
        printf("failed can't move\n");
        ret = 0;
    }
    else if (WHITE_MOVE(state->move)) {
        ret = 1;
    }
    else {
        
        ret = 1;
    }
    return ret;
}

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
    move->y1 = 8 - (line[match[1].rm_eo - 1] - '0');
    move->x2 = toupper(line[match[2].rm_so]) - 'A';
    move->y2 = 8 - (line[match[2].rm_eo - 1] - '0');

    if (!valid_move(state, move)) {
        printf("Invalid move!\n");
        return -1;
    }
    
    return 0;
}

void get_move(struct state_t* state, struct move_t* move) {
    int i;
    char* line = 0;
    size_t n = 0;

    for (i = 0; ask_for_move(state, move) != 0 && i < 3; ++i) {
        printf("\nDo you want to quit? ");
        if (getline(&line, &n, stdin) < 1) {
            printf("getline failed!\n");
            exit(0);
        }
        if (rpmatch(line) == 1) {
            exit(0);
        }
    }
    if (i >= 3) {
        exit(0);
    }
}

void make_move(struct state_t* state, struct move_t* move) {
    if (WHITE_MOVE(state->move)) {
        CLEAR(state->white, SQUARE(move->x1, move->y1));
        SET(state->white, SQUARE(move->x2, move->y2));
    }
    else {
        CLEAR(state->black, SQUARE(move->x1, move->y1));
        SET(state->black, SQUARE(move->x2, move->y2));
    }
}

int main(int argc, char **argv) {
    struct move_t move;
    struct state_t state;

    state_init(state);
    print_board(&state);
    printf("\nDone.\n");

    get_move(&state, &move);
    printf("Move: (%d, %d) -> (%d, %d)\n", move.x1, move.y1, move.x2, move.y2);

    make_move(&state, &move);
    print_board(&state);

    return 0;
}
