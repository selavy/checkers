#define _SVID_SOURCE
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

struct state_t {
    board_t white; /* displayed as 'x' */
    board_t black; /* displayed as 'o' */
    board_t white_kings; /* -- displayed as 'X' -- */
    board_t black_kings; /* -- displayed as 'O' -- */
    move_t move;
};

struct move_t {
    char x1;
    char y1;
    char x2;
    char y2;
    uint8_t capture; // 255 if no capture, otherwise location of piece captured
};

board_t MOVES[] = { 0,1280,0,5120,0,20480,0,16384,131074,0,655370,0,2621480,0,10485920,0,0,83887360,0,335549440,0,1342197760,0,1073758208,8590065664,0,42950328320,0,171801313280,0,687205253120,0,0,5497642024960,0,21990568099840,0,87962272399360,0,70369817919488,562958543355904,0,2814792716779520,0,11259170867118080,0,45036683468472320,0,0,360293467747778560,0,1441173870991114240,0,5764695483964456960,0,4611756387171565568,562949953421312,0,2814749767106560,0,11258999068426240,0,45035996273704960,0 };

board_t WHITE_MOVES[] = { 0,0,0,0,0,0,0,0,2,0,10,0,40,0,160,0,0,1280,0,5120,0,20480,0,16384,131072,0,655360,0,2621440,0,10485760,0,0,83886080,0,335544320,0,1342177280,0,1073741824,8589934592,0,42949672960,0,171798691840,0,687194767360,0,0,5497558138880,0,21990232555520,0,87960930222080,0,70368744177664,562949953421312,0,2814749767106560,0,11258999068426240,0,45035996273704960,0 };

board_t BLACK_MOVES[] = { 0,1280,0,5120,0,20480,0,16384,131072,0,655360,0,2621440,0,10485760,0,0,83886080,0,335544320,0,1342177280,0,1073741824,8589934592,0,42949672960,0,171798691840,0,687194767360,0,0,5497558138880,0,21990232555520,0,87960930222080,0,70368744177664,562949953421312,0,2814749767106560,0,11258999068426240,0,45035996273704960,0,0,360287970189639680,0,1441151880758558720,0,5764607523034234880,0,4611686018427387904,0,0,0,0,0,0,0,0 };

#define CAPTURE(move) ((move).capture != 255)
#define MASK(square) ((board_t)1 << (square))
#define CAN_MOVE(to, from) (MOVES[(from)] & MASK(to))
#define WHITE_CAN_MOVE(to, from) (WHITE_MOVES[(from)] & MASK(to))
#define BLACK_CAN_MOVE(to, from) (BLACK_MOVES[(from)] & MASK(to))
#define BLACK_MOVE(state) ((state).move & 1)
#define WHITE_MOVE(state) (!BLACK_MOVE(state))
#define IS_SET(board, square) ((board) & MASK(square))
#define SET(board, square) (board |= MASK(square))
#define CLEAR(board, square) (board &= ~MASK(square))
#define DISPLAY(state, square) (IS_SET((state).white, (square)) ? 'x'       : \
                               (IS_SET((state).white_kings, (square)) ? 'X' : \
                               (IS_SET((state).black, (square)) ? 'o'       : \
                               (IS_SET((state).black_kings, (square)) ? 'O' : \
                               ' '))))
#define state_init(state)                       \
    (state).white = 6172839697753047040UL;      \
    (state).white_kings = 0UL;                  \
    (state).black = 11163050UL;                 \
    (state).black_kings = 0UL;                  \
    (state).move  = 0
#define move_init(move)                         \
    (move).x1 = 0;                              \
    (move).y1 = 0;                              \
    (move).x2 = 0;                              \
    (move).y2 = 0;                              \
    (move).capture = 255;
#define VALID_MOVES 6172840429334713770UL
#define VALID(square) (((square_t)(square) & VALID_MOVES))
#define SQUARE(x, y) ((y)*8 + (x))
#define FROM_SQUARE(move) (SQUARE((move).x1, (move).y1))
#define TO_SQUARE(move) (SQUARE((move).x2, (move).y2))
#define WHITE(board) ((board_t)((board).white | (board).white_kings))
#define BLACK(board) ((board_t)((board).black | (board).black_kings))
#define FULLBOARD(board) (WHITE(board) | BLACK(board))
#define OCCUPIED(square, board) (MASK(square) & FULLBOARD(board))
#define DOWN_LEFT(square) ((square) + 9)
#define DOWN_RIGHT(square) ((square) + 7)
#define UP_LEFT(square) ((square) - 7)
#define UP_RIGHT(square) ((square) - 9)

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

    return regexec(&preg, line, 1, &(match[0]), 0);
}

int legal_move(struct state_t* state, struct move_t* move) {
    int ret;
    square_t to = TO_SQUARE(*move);
    square_t from = FROM_SQUARE(*move);

    if (!VALID(to) || OCCUPIED(to, *state)) {
        ret = 0;
    }
    else if (WHITE_MOVE(*state)) {
        printf("Checking white move\n");
        /* TODO: check jumps, double jumps, kings */
        printf("to = %lu, from = %lu, mask = %lu\n", to, from, WHITE_MOVES[from]);
        printf("up_left(up_left(from)) = %lu\n", UP_LEFT(UP_LEFT(from)));
        printf("up_right(up_right(from)) = %lu\n", UP_RIGHT(UP_RIGHT(from)));
        if (WHITE_CAN_MOVE(to, from)) {
            ret = 1;
        } else if (to == UP_LEFT(UP_LEFT(from)) && IS_SET(BLACK(*state), UP_LEFT(from))) {
            move->capture = UP_LEFT(from);
            ret = 1;
        } else if (to == UP_RIGHT(UP_RIGHT(from)) && IS_SET(BLACK(*state), UP_RIGHT(from))) {
            move->capture = UP_RIGHT(from);
            ret = 1;
        } else {
            /* check king moves */
            ret = 0;
        }
    }
    else { /* black move */
        printf("Checking black move\n");
        /* TODO: check jumps, double jumps, kings */
        printf("to = %lu, from = %lu, mask = %lu\n", to, from, BLACK_MOVES[from]);
        if (BLACK_CAN_MOVE(to, from)) {
            ret = 1;
        } else if (to == DOWN_LEFT(DOWN_LEFT(from)) && IS_SET(WHITE(*state), DOWN_LEFT(from))) {
            move->capture = DOWN_LEFT(from);
            ret = 1;
        } else if (to == DOWN_RIGHT(DOWN_RIGHT(from)) && IS_SET(WHITE(*state), DOWN_RIGHT(from))) {
            move->capture = DOWN_RIGHT(from);
            ret = 1;
        } else {
            ret = 0;
        }
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

    if (!legal_move(state, move)) {
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
        if (rpmatch(line) == 0) {
            exit(0);
        }
    }
    if (i >= 3) {
        exit(0);
    }
}

void make_move(struct state_t* state, struct move_t* move) {
    if (WHITE_MOVE(*state)) {
        CLEAR(state->white, SQUARE(move->x1, move->y1));
        SET(state->white, SQUARE(move->x2, move->y2));
        if (CAPTURE(*move)) {
            CLEAR(state->black, move->capture);
        }
    } else {
        CLEAR(state->black, SQUARE(move->x1, move->y1));
        SET(state->black, SQUARE(move->x2, move->y2));
        if (CAPTURE(*move)) {
            CLEAR(state->white, move->capture);
        }
    }
    state->move += 1;
}

int main(int argc, char **argv) {
    struct move_t move;
    struct state_t state;
    int i;

    state_init(state);
    print_board(&state);

    /* TODO: how to handle multi-capture moves? */
    for (i = 0; i < 10; ++i) {
        printf("Move #%d\n", state.move);
        if (WHITE_MOVE(state)) {
            printf("white to move\n");
        }
        if (BLACK_MOVE(state)) {
            printf("black to move\n");
        }
        move_init(move);
        get_move(&state, &move);
        make_move(&state, &move);
        print_board(&state);
    }

    return 0;
}
