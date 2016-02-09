/* 
 * TODO: do you have to capture with the most number of jumps? Not clear from itsyourturn.com.
 * Definitely do have to in international checkers, but not sure for American checkers.
 * 
 * Right now I am enforcing that, but revisit this later, shouldn't be too hard to change, just will have
 * to update move_list_append_capture().
 */

/* #define _SVID_SOURCE */
/* #define _POSIX_C_SOURCE 200809L */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#define MAX_PATH 8
#define MAX_MOVES 32
#define COLUMNS 8
#define ROWS 8
#define SQUARES 32
#define TRUE 1
#define FALSE 0
/* 0 indexed */
#define MASK(square) ((square_t)1 << (square))
/* 1 indexed */
#define SQR(n) ((n) - 1)
#define SQUARE(square) MASK(SQR(square))
#define PLACE(pieces, square) do { ((pieces) |= MASK(square)); } while(0)
#define CLEAR(pieces, square) do { ((pieces) &= ~MASK(square)); } while(0)
#define OCCUPIED(pieces, square) ((pieces) & MASK(square))
#define MOVE(pieces, from, to) do { PLACE(pieces, to); CLEAR(pieces, from); } while(0)
#define ROW(square) (((square) / 4) - 1)
#define WHITE(state) ((state).white | (state).white_kings)
#define BLACK(state) ((state).black | (state).black_kings)
#define KINGS(state) ((state).black_kings | (state).white_kings)
#define BLACK_KING(state, square) ((OCCUPIED((state).black_kings, square)) != 0)
#define WHITE_KING(state, square) ((OCCUPIED((state).white_kings, square)) != 0)
#define PAWNS(state) ((state).white | (state).black)
#define FULLBOARD(state) (WHITE(state) | BLACK(state))
#define ODDROW(square) ((square / 4) & 1)
#define UP_LEFT(square) (ODDROW(square) ? (square) + 4 : (square) + 3)
#define UP_RIGHT(square) (ODDROW(square) ? (square) + 5 : (square) + 4)
#define DOWN_LEFT(square) (ODDROW(square) ? (square) - 4 : (square) - 5)
#define DOWN_RIGHT(square) (ODDROW(square) ? (square) - 3 : (square) - 4)
#define JUMP_UP_LEFT(square) ((square) + 7)
#define JUMP_UP_RIGHT(square) ((square) + 9)
#define JUMP_DOWN_LEFT(square) ((square) - 9)
#define JUMP_DOWN_RIGHT(square) ((square) - 7)
#define TOP(square) ((MASK(square) & (MASK(28) | MASK(29) | MASK(30) | MASK(31))) != 0)
#define LEFT(square) ((MASK(square) & (MASK(0) | MASK(8) | MASK(16) | MASK(24))) != 0)
#define RIGHT(square) ((MASK(square) & (MASK(7) | MASK(15) | MASK(23) | MASK(31))) != 0)
#define BOTTOM(square) ((MASK(square) & (MASK(0) | MASK(1) | MASK(2) | MASK(3))) != 0)
#define TOP2(square) ((MASK(square) & (MASK(24) | MASK(25) | MASK(26) | MASK(27))) != 0)
#define LEFT2(square) ((MASK(square) & (MASK(4) | MASK(12) | MASK(20) | MASK(28))) != 0)
#define RIGHT2(square) ((MASK(square) & (MASK(3) | MASK(11) | MASK(19) | MASK(27))) != 0)
#define BOTTOM2(square) ((MASK(square) & (MASK(4) | MASK(5) | MASK(6) | MASK(7))) != 0)

static const char * __unittest = 0;
/* static int __unittest_number = 0; */
#define ENTER_UNITTEST() do { __unittest = __func__; } while(0)
#define EXIT_UNITTEST() do { printf("Passed %s.\n", __unittest); } while(0)
#define __UNITTEST_FAIL(line) do {                                      \
        fprintf(stderr, "Unit Test [%s] failed on line: %d\n", __unittest, line); \
        exit(1);                                                        \
    } while(0)
#define UNITTEST_ASSERT(actual, expected) do {  \
        if ((actual) != (expected)) {           \
            __UNITTEST_FAIL(__LINE__);          \
        }                                       \
    } while(0)
#define UNITTEST_ASSERT_MSG(actual, expected, format) do {              \
        if ((actual) != (expected)) {                                   \
            fprintf(stderr, "Expected: " format ", Actual: " format "\n", (expected), (actual)); \
            __UNITTEST_FAIL(__LINE__);                                  \
        }                                                               \
    } while(0)
#define UNITTEST_ASSERT_NEQU(actual, expected) do { \
        if ((actual) == (expected)) {               \
            __UNITTEST_FAIL(__LINE__);              \
        }                                           \
    } while(0)
#define UNITTEST_ASSERT_MOVELIST(actual, expected) do {     \
        move_list_sort(&actual);                            \
        move_list_sort(&expected);                          \
        if (move_list_compare(&actual, &expected) != 0) {   \
            printf("Expected: ");                           \
            print_move_list(expected);                      \
            printf("\nActual: ");                           \
            print_move_list(actual);                        \
            printf("\n");                                   \
            __UNITTEST_FAIL(__LINE__);                      \
        }                                                   \
    } while(0)

/* --- types --- */
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

/* moves are 1-indexed so 0 can indicate that the path is empty */
struct move_t {
    uint8_t src; /* square started on */
    uint8_t dst; /* square ended on */
    uint8_t path[MAX_PATH]; /* path traveled if this was a multi-jump */
    uint8_t pathlen;
};
/* -- end types -- */

boolean is_noncapture(const struct move_t* move) {
    return (move->pathlen == 0)
        && (move->dst == UP_LEFT(move->src)
            || move->dst == UP_RIGHT(move->src)
            || move->dst == DOWN_LEFT(move->src)
            || move->dst == DOWN_RIGHT(move->src));
}

boolean is_capture(const struct move_t* move) {
    return !is_noncapture(move);
}

#define move_init(move) memset(move, 0, sizeof(*move))

/* move_compare: compare 2 moves for equality
 * 0  => equal
 * 1  => lhs > rhs
 * -1 => lhs < rhs
 */
int move_compare(struct move_t* lhs, struct move_t* rhs) {
    int ret;
    if (lhs->src != rhs->src) {
        ret = lhs->src - rhs->src;
    } else if (lhs->dst != rhs->dst) {
        ret = lhs->dst - rhs->dst;
    } else if (lhs->pathlen != rhs->pathlen) {
        ret = lhs->pathlen - rhs->pathlen;
    } else {
        ret = memcmp(&(lhs->path[0]), &(rhs->path[0]), sizeof(lhs->path[0]) * lhs->pathlen);
    }
    return ret;
}

struct move_list_t {
    struct move_t moves[MAX_MOVES];
    int njumps; /* number of jumps */    
    int nmoves; /* number of regular moves */
};
#define move_list_num_moves(list) ((list).njumps + (list).nmoves)

/* TODO: print captures with 'x' between intead of '-' */
void __print_move(FILE* file, struct move_t* move /*, boolean is_capture */) {
    int i;
    fprintf(file, "%d-", move->src + 1);
    for (i = 0; i < move->pathlen; ++i) {
        fprintf(file, "%d-", move->path[i] + 1);
    }
    fprintf(file, "%d", move->dst + 1);
}

/* move_list_compare: compare 2 move_lists for strict equality
 * 0  => equal
 * 1  => lhs > rhs
 * -1 => lhs < rhs
 */
int move_list_compare(struct move_list_t* lhs, struct move_list_t* rhs) {
    int ret = 0;
    int i;
    const int lhs_moves = move_list_num_moves(*lhs);
    const int rhs_moves = move_list_num_moves(*rhs);

    if (lhs_moves != rhs_moves) {
        ret = lhs_moves - rhs_moves;
    } else {
        for (i = 0; i < lhs_moves; ++i) {
            ret = move_compare(&(lhs->moves[i]), &(rhs->moves[i]));
            if (ret != 0) {
                break;
            }
        }
    }
    return ret;
}

int __move_compare_generic(const void* lhs, const void* rhs) {
    return move_compare((struct move_t*)lhs, (struct move_t*)rhs);
}

void move_list_sort(struct move_list_t* list) {
    int nmoves = move_list_num_moves(*list);
    if (nmoves != 0) {
        qsort(&(list->moves[0]), nmoves, sizeof(list->moves[0]), &__move_compare_generic);
    }
}

int move_list_init(struct move_list_t* list) {
    list->njumps = 0;
    list->nmoves = 0;
    memset(&(list->moves[0]), 0, sizeof(list->moves[0]) * MAX_MOVES);
    return 0;
}

void __move_list_append_move(struct move_list_t* list, int src, int dst) {
    struct move_t* const move = &(list->moves[move_list_num_moves(*list)]);
    move->src = src;
    move->dst = dst;
    ++list->nmoves;
}
#define move_list_append_move(list, src, dst) __move_list_append_move(&list, src, dst)
void __move_list_append_capture(struct move_list_t* list, struct move_t* move) {
    struct move_t* const movep = &(list->moves[move_list_num_moves(*list)]);
    ++list->njumps;
    movep->src = move->src;
    movep->dst = move->dst;
    memcpy(&(movep->path[0]), &(move->path[0]), sizeof(move->path[0]) * sizeof(move->path));
    movep->pathlen = move->pathlen;
}
#define move_list_append_capture(list, move) __move_list_append_capture(&(list), &(move))

/* helper for unittests, don't use in real code */
void APPEND_CAPTURE(struct move_list_t* list, int src, int dst) {
    struct move_t move;
    move_init(&move);
    move.src = SQR(src);
    move.dst = SQR(dst);
    move_list_append_capture(*list, move);
}

void __print_move_list(FILE* file, struct move_list_t* list) {
    int i;
    const int moves = move_list_num_moves(*list);
    if (moves > 0) {
        __print_move(file, &(list->moves[0]));

        for (i = 1; i < moves; ++i) {
            fprintf(file, ", ");
        __print_move(file, &(list->moves[i]));
        }

        fprintf(file, ", moves: %d, jumps: %d", list->nmoves, list->njumps);
    }
}
#define print_move_list(list) __print_move_list(stdout, &(list));

void __state_init(struct state_t* state) {
    memset(state, 0, sizeof(*state));
}
#define state_init(state) do { memset(state, 0, sizeof(*state)); } while(0)

int __black_move(struct state_t* state) {
    return (state->moves & 1) == 0;
}
#define black_move(state) (((state).moves & 1) == 0)

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
    boolean skip = TRUE;
    int square = 28; /* upper left square, 0-indexed */
    for (r = ROWS - 1; r >= 0; --r) {
        fprintf(file, "\n    ---------------------------------\n    |");
        for (c = 0; c < COLUMNS; ++c) {
            if (skip) {
                fprintf(file, "   |");
            } else {
                if (OCCUPIED(state->black, square)) {
                    fprintf(file, " b |");
                } else if (OCCUPIED(state->black_kings, square)) {
                    fprintf(file, " B |");
                } else if (OCCUPIED(state->white, square)) {
                    fprintf(file, " w |");
                } else if (OCCUPIED(state->white_kings, square)) {
                    fprintf(file, " W |");
                } else {
                    if (square > 8) { // will use 2 characters
                        fprintf(file, "%d |", square + 1);
                    } else {
                        fprintf(file, " %d |", square + 1);
                    }
                }
                ++square;
            }
            skip ^= TRUE;
        }
        skip ^= TRUE;
        square -= 8;
    }
    fprintf(file, "\n    ---------------------------------\n");       
}
#define print_board(state) __print_board(stdout, &state);

void __setup_start_position(struct state_t* state) {
    state->white = 4293918720;
    state->black = 4095;
    state->white_kings = 0;
    state->black_kings = 0;
    state->moves = 0;
}
#define setup_start_position(state)             \
    (state).white = 4293918720;                 \
    (state).black = 4095;                       \
    (state).white_kings = 0;                    \
    (state).black_kings = 0;                    \
    (state).moves = 0;

void add_to_move_list(struct move_list_t* moves, int* path, int len) {
    int i;
    struct move_t move;
    boolean skip = FALSE;
    const int nmoves = move_list_num_moves(*moves);

    /* check that we aren't adding a capture that is shorter */
    for (i = 0; i < nmoves; ++i) {
        if (moves->moves[i].pathlen > len) {
            skip = TRUE;
            break;
        }
    }

    if (!skip) {
        /* TODO: don't use move_list_append_capture, just do the appending manually */
        move.src = path[0];
        for (i = 1; i < len; ++i) { /* TODO: switch to memcpy(&(move.path[0]), path[1], sizeof(path[1]) * len - 1); */
            move.path[i - 1] = path[i];
        }
        move.dst = path[len]; /* intentionally not len + 1 */
        move.pathlen = len - 1;
        move_list_append_capture(*moves, move);    
    }
}

int multicapture_black(int32_t white, int32_t black, struct move_list_t* moves, int* path, int len, boolean is_king) {
    int32_t nwhite;
    int32_t nblack;
    int ret = 0;
    int square = path[len - 1];

    if (!TOP(square) && !TOP2(square)) {
        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(white, UP_LEFT(square)) && !OCCUPIED(white | black, JUMP_UP_LEFT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nwhite, UP_LEFT(square));
            PLACE(nblack, UP_LEFT(square));
            CLEAR(nblack, square);
            PLACE(nblack, JUMP_UP_LEFT(square));
            ret = 1;
            path[len] = JUMP_UP_LEFT(square);

            if (!multicapture_black(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(white, UP_RIGHT(square)) && !OCCUPIED(white | black, JUMP_UP_RIGHT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nwhite, UP_RIGHT(square));
            PLACE(nblack, UP_RIGHT(square));
            CLEAR(nblack, square);
            PLACE(nblack, JUMP_UP_RIGHT(square));
            ret = 1;
            path[len] = JUMP_UP_RIGHT(square);
        
            if (!multicapture_black(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
    }

    if (is_king && !BOTTOM(square) && !BOTTOM2(square)) {
        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(white, DOWN_LEFT(square)) && !OCCUPIED(white | black, JUMP_DOWN_LEFT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nwhite, DOWN_LEFT(square));
            PLACE(nblack, DOWN_LEFT(square));
            CLEAR(nblack, square);
            PLACE(nblack, JUMP_DOWN_LEFT(square));
            ret = 1;
            path[len] = JUMP_DOWN_LEFT(square);
            if (!multicapture_black(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(white, DOWN_RIGHT(square)) && !OCCUPIED(white | black, JUMP_DOWN_RIGHT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nwhite, DOWN_RIGHT(square));
            PLACE(nblack, DOWN_RIGHT(square));
            CLEAR(nblack, square);
            PLACE(nblack, JUMP_DOWN_RIGHT(square));
            ret = 1;
            path[len] = JUMP_DOWN_RIGHT(square);
            if (!multicapture_black(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }            
        }
    }
    
    return ret;
}

int multicapture_white(int32_t white, int32_t black, struct move_list_t* moves, int* path, int len, boolean is_king) {
    int32_t nwhite;
    int32_t nblack;
    int ret = 0;
    int square = path[len - 1];

    if (!BOTTOM(square) && !BOTTOM2(square)) {
        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(black, DOWN_LEFT(square)) && !OCCUPIED(white | black, JUMP_DOWN_LEFT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nblack, DOWN_LEFT(square));
            PLACE(nwhite, DOWN_LEFT(square));
            CLEAR(nwhite, square);
            PLACE(nwhite, JUMP_DOWN_LEFT(square));
            ret = 1;
            path[len] = JUMP_DOWN_LEFT(square);
            if (!multicapture_white(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(black, DOWN_RIGHT(square)) && !OCCUPIED(white | black, JUMP_DOWN_RIGHT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nblack, DOWN_RIGHT(square));
            PLACE(nwhite, DOWN_RIGHT(square));
            CLEAR(nwhite, square);
            PLACE(nwhite, JUMP_DOWN_RIGHT(square));
            ret = 1;
            path[len] = JUMP_DOWN_RIGHT(square);
            if (!multicapture_white(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
    }

    if (is_king && !TOP(square) && !TOP2(square)) {
        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(black, UP_LEFT(square)) && !OCCUPIED(white | black, JUMP_UP_LEFT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nblack, UP_LEFT(square));
            PLACE(nwhite, UP_LEFT(square));
            CLEAR(nwhite, square);
            PLACE(nwhite, JUMP_UP_LEFT(square));
            ret = 1;
            path[len] = JUMP_UP_LEFT(square);
            if (!multicapture_white(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }
        }
        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(black, UP_RIGHT(square)) && !OCCUPIED(white | black, JUMP_UP_RIGHT(square))) {
            nwhite = white;
            nblack = black;
            CLEAR(nblack, UP_RIGHT(square));
            PLACE(nwhite, UP_RIGHT(square));
            CLEAR(nwhite, square);
            PLACE(nwhite, JUMP_UP_RIGHT(square));
            ret = 1;
            path[len] = JUMP_UP_RIGHT(square);
            if (!multicapture_white(nwhite, nblack, moves, path, len + 1, is_king)) {
                add_to_move_list(moves, path, len);
            }            
        }
    }

    return ret;
}

int generate_captures(const struct state_t* state, struct move_list_t* moves) {
    square_t square;
    struct move_t move;
    move_init(&move);
    int path[10];
    int i;
    int maxlength;
    int nmoves;
    struct move_list_t tmp;

    if (black_move(*state)) {
        for (square = 0; square < SQUARES; ++square) {
            if (OCCUPIED(BLACK(*state), square)) {
                if (!TOP(square) && !TOP2(square)) {                
                    if (!LEFT(square) && !LEFT2(square) && OCCUPIED(WHITE(*state), UP_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_LEFT(square))) {
                        path[0] = square;
                        path[1] = JUMP_UP_LEFT(square);
                        if (!multicapture_black(WHITE(*state) & ~MASK(UP_LEFT(square)), (BLACK(*state) | MASK(JUMP_UP_LEFT(square))), moves, &(path[0]), 2, BLACK_KING(*state, square))) {
                            move.src = square;
                            move.dst = JUMP_UP_LEFT(square);                        
                            move_list_append_capture(*moves, move);
                        }
                    }
                    if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(WHITE(*state), UP_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_RIGHT(square))) {
                        path[0] = square;
                        path[1] = JUMP_UP_RIGHT(square);
                        if (!multicapture_black(WHITE(*state) & ~MASK(UP_RIGHT(square)), (BLACK(*state) | MASK(JUMP_UP_LEFT(square))), moves, &(path[0]), 2, BLACK_KING(*state, square))) {
                            move.src = square;
                            move.dst = JUMP_UP_RIGHT(square);                        
                            move_list_append_capture(*moves, move);
                        }
                    }
                }

                if (BLACK_KING(*state, square)) {
                    if (!BOTTOM(square) && !BOTTOM2(square)) {                        
                        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(WHITE(*state), DOWN_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_LEFT(square))) {
                            path[0] = square;
                            path[1] = JUMP_DOWN_LEFT(square);
                            if (!multicapture_black(WHITE(*state) & ~MASK(DOWN_LEFT(square)), (BLACK(*state) | MASK(JUMP_DOWN_LEFT(square))), moves, &(path[0]), 2, TRUE)) {
                                move.src = square;
                                move.dst = JUMP_DOWN_LEFT(square);                        
                                move_list_append_capture(*moves, move);
                            }
                        }
                        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(WHITE(*state), DOWN_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_RIGHT(square))) {
                            path[0] = square;
                            path[1] = JUMP_DOWN_RIGHT(square);
                            if (!multicapture_black(WHITE(*state) & ~MASK(DOWN_RIGHT(square)), (BLACK(*state) | MASK(JUMP_DOWN_RIGHT(square))), moves, &(path[0]), 2, TRUE)) {
                                move.src = square;
                                move.dst = JUMP_DOWN_RIGHT(square);                        
                                move_list_append_capture(*moves, move);
                            }                            
                        }
                    }
                }
            }
        }
    } else {
        for (square = 0; square < SQUARES; ++square) {
            if (OCCUPIED(WHITE(*state), square)) {
                if (!BOTTOM(square) && !BOTTOM2(square)) {
                    if (!LEFT(square) && !LEFT2(square) && OCCUPIED(BLACK(*state), DOWN_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_LEFT(square))) {
                        path[0] = square;
                        path[1] = JUMP_DOWN_LEFT(square);
                        if (!multicapture_white((WHITE(*state) | MASK(JUMP_DOWN_LEFT(square))), BLACK(*state) & ~MASK(DOWN_LEFT(square)), moves, &(path[0]), 2, WHITE_KING(*state, square))) {
                            move.src = square;
                            move.dst = JUMP_DOWN_LEFT(square);                        
                            move_list_append_capture(*moves, move);
                        }                    
                    }
                    if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(BLACK(*state), DOWN_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_RIGHT(square))) {
                        path[0] = square;
                        path[1] = JUMP_DOWN_RIGHT(square);
                        if (!multicapture_white((WHITE(*state) | MASK(JUMP_DOWN_RIGHT(square))), BLACK(*state) & ~MASK(DOWN_RIGHT(square)), moves, &(path[0]), 2, WHITE_KING(*state, square))) {
                            move.src = square;
                            move.dst = JUMP_DOWN_RIGHT(square);                        
                            move_list_append_capture(*moves, move);
                        }                    
                    }
                }
                if (WHITE_KING(*state, square)) {
                    if (!TOP(square) && !TOP2(square)) {
                        if (!LEFT(square) && !LEFT2(square) && OCCUPIED(BLACK(*state), UP_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_LEFT(square))) {
                            path[0] = square;
                            path[1] = JUMP_UP_LEFT(square);
                            if (!multicapture_white((WHITE(*state) | MASK(JUMP_UP_LEFT(square))), BLACK(*state) & ~MASK(UP_LEFT(square)), moves, &(path[0]), 2, TRUE)) {
                                move.src = square;
                                move.dst = JUMP_UP_LEFT(square);
                                move_list_append_capture(*moves, move);
                            }
                        }
                        if (!RIGHT(square) && !RIGHT2(square) && OCCUPIED(BLACK(*state), UP_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_RIGHT(square))) {
                            path[0] = square;
                            path[1] = JUMP_UP_RIGHT(square);
                            if (!multicapture_white((WHITE(*state) | MASK(JUMP_UP_RIGHT(square))), BLACK(*state) & ~MASK(UP_RIGHT(square)), moves, &(path[0]), 2, TRUE)) {
                                move.src = square;
                                move.dst = JUMP_UP_RIGHT(square);
                                move_list_append_capture(*moves, move);
                            }

                        }
                    }
                }
            }
        }        
    }

    nmoves = move_list_num_moves(*moves);
    if (nmoves > 1) {
        maxlength = 0;
        for (i = 0; i < nmoves; ++i) {
            if (moves->moves[i].pathlen > maxlength) {
                maxlength = moves->moves[i].pathlen;
            }
        }
        move_list_init(&tmp);
        for (i = 0; i < nmoves; ++i) {
            if (moves->moves[i].pathlen == maxlength) {
                move_list_append_capture(tmp, moves->moves[i]);
            }
        }
        move_list_init(moves);
        memcpy(moves, &tmp, sizeof(*moves));
    }
    
    return 0;
}


int generate_moves(const struct state_t* state, struct move_list_t* moves) {
    square_t square;
    if (black_move(*state)) {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(BLACK(*state), square)) {
                if (!TOP(square)) {
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                        move_list_append_move(*moves, square, UP_LEFT(square));
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                        move_list_append_move(*moves, square, UP_RIGHT(square));
                    }
                }

                if (OCCUPIED(state->black_kings, square) && !BOTTOM(square)) {
                    /* check king moves */
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                        move_list_append_move(*moves, square, DOWN_LEFT(square));
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                        move_list_append_move(*moves, square, DOWN_RIGHT(square));
                    }
                }
            }
        }
    } else {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(WHITE(*state), square)) {
                if (!BOTTOM(square)) {
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                        move_list_append_move(*moves, square, DOWN_LEFT(square));
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                        move_list_append_move(*moves, square, DOWN_RIGHT(square));
                    }
                }
                
                if (OCCUPIED(state->white_kings, square) && !TOP(square)) {
                    /* check king moves */
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                        move_list_append_move(*moves, square, UP_LEFT(square));
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                        move_list_append_move(*moves, square, UP_RIGHT(square));
                    }
                }
            }
        }        
    }
    return 0;
}

int get_moves(const struct state_t* state, struct move_list_t* moves) {
    generate_captures(state, moves);
    if (moves->njumps == 0) {
        generate_moves(state, moves);
    }
    return 0;
}


/* returns -1 on error */
int jumped_square(int src, int dst) {
    if (dst == JUMP_UP_LEFT(src)) {
        return UP_LEFT(src);
    } else if (dst == JUMP_UP_RIGHT(src)) {
        return UP_RIGHT(src);
    } else if (dst == JUMP_DOWN_LEFT(src)) {
        return DOWN_LEFT(src);
    } else if (dst == JUMP_DOWN_RIGHT(src)) {
        return DOWN_RIGHT(src);
    } else {
        return -1;
    }
}

void make_move(struct state_t* state, const struct move_t* move) {
    int jumped;
    int i;
    uint8_t prev;
    if (black_move(*state)) {
        if (BLACK_KING(*state, move->src)) {
            CLEAR(state->black_kings, move->src);
            PLACE(state->black_kings, move->dst);
            if (is_capture(move)) {
                if (move->pathlen == 0) {
                    jumped = jumped_square(move->src, move->dst);
                    CLEAR(state->white_kings, jumped);
                    CLEAR(state->white, jumped);                    
                } else {
                    prev = move->src;
                    for (i = 0; i < move->pathlen; ++i) {
                        jumped = jumped_square(prev, move->path[i]);
                        CLEAR(state->white, jumped);
                        CLEAR(state->white_kings, jumped);
                        prev = move->path[i];
                    }
                    jumped = jumped_square(move->path[move->pathlen - 1], move->dst);
                    CLEAR(state->white, jumped);
                    CLEAR(state->white_kings, jumped);
                }
            }
        } else {
            CLEAR(state->black, move->src);
            if (TOP(move->dst)) {
                printf("promotion for black -> %d\n", move->dst);
                PLACE(state->black_kings, move->dst);
            } else {
                PLACE(state->black, move->dst);
            }
            if (is_capture(move)) {
                if (move->pathlen == 0) {
                    jumped = jumped_square(move->src, move->dst);
                    CLEAR(state->white_kings, jumped);
                    CLEAR(state->white, jumped);
                } else {
                    prev = move->src;
                    for (i = 0; i < move->pathlen; ++i) {
                        jumped = jumped_square(prev, move->path[i]);
                        CLEAR(state->white, jumped);
                        CLEAR(state->white_kings, jumped);
                        prev = move->path[i];
                        jumped = jumped_square(move->path[move->pathlen - 1], move->dst);
                        CLEAR(state->white, jumped);
                        CLEAR(state->white_kings, jumped);
                    }
                }
            }
        }
    } else { /* white_move(*state) */
        if (WHITE_KING(*state, move->src)) {
            CLEAR(state->white_kings, move->src);
            PLACE(state->white_kings, move->dst);
            if (is_capture(move)) {
                if (move->pathlen == 0) {
                    jumped = jumped_square(move->src, move->dst);
                    CLEAR(state->black_kings, jumped);
                    CLEAR(state->black, jumped);                    
                } else {
                    prev = move->src;
                    for (i = 0; i < move->pathlen; ++i) {
                        jumped = jumped_square(prev, move->path[i]);
                        CLEAR(state->black, jumped);
                        CLEAR(state->black_kings, jumped);
                        prev = move->path[i];
                    }
                    jumped = jumped_square(move->path[move->pathlen - 1], move->dst);
                    CLEAR(state->black, jumped);
                    CLEAR(state->black_kings, jumped);
                }
            }
        } else {
            CLEAR(state->white, move->src);
            if (BOTTOM(move->dst)) {
                printf("promotion for white -> %d\n", move->dst);                
                PLACE(state->white_kings, move->dst);
            } else {
                PLACE(state->white, move->dst);
            }
            PLACE(state->white, move->dst);
            if (is_capture(move)) {
                if (move->pathlen == 0) {
                    jumped = jumped_square(move->src, move->dst);
                    CLEAR(state->black_kings, jumped);
                    CLEAR(state->black, jumped);
                } else {
                    prev = move->src;
                    for (i = 0; i < move->pathlen; ++i) {
                        jumped = jumped_square(prev, move->path[i]);
                        CLEAR(state->black, jumped);
                        CLEAR(state->black_kings, jumped);
                        prev = move->path[i];
                        jumped = jumped_square(move->path[move->pathlen - 1], move->dst);
                        CLEAR(state->black, jumped);
                        CLEAR(state->black_kings, jumped);
                    }
                }
            }
        }        
    }
}

int _CC[] = {
    4,
    3,
    2,
    1,
    8,
    7,
    6,
    5,
    12,
    11,
    10,
    9,
    16,
    15,
    14,
    13,
    20,
    19,
    18,
    17,
    24,
    23,
    22,
    21,
    28,
    27,
    26,
    25,
    32,
    31,
    30,
    29
};

#define CONV(sqr) _CC[(sqr)]
/* #define CONV(sqr) (sqr) */

uint64_t __perft_helper(int depth, const struct state_t* in_state) {
    struct state_t state;
    struct move_list_t movelist;
    int i;
    int nmoves;
    int64_t nodes = 0;
    const struct move_t* move; /* TODO: remove */
    /* int j; */               /* TODO: remove */
    
    if (depth == 0) return 1;
    move_list_init(&movelist);
    get_moves(in_state, &movelist);
    nmoves = move_list_num_moves(movelist);
    if (depth == 1) {
        for (i = 0; i < nmoves; ++i) {
            move = &(movelist.moves[i]);
            printf("%d -> %d\n", CONV(move->src), CONV(move->dst));
            /* printf("%d", CONV(move->src)); */
            /* for (j = 0; j < move->pathlen; ++j) { */
            /*     printf(" -> %d", CONV(move->path[j])); */
            /* } */
            /* printf(" -> %d\n", CONV(move->dst)); */
        }
        return nmoves;
    }
    for (i = 0; i < nmoves; ++i) {
        memcpy(&state, in_state, sizeof(state));
        make_move(&state, &(movelist.moves[i]));
        ++state.moves;
        nodes += __perft_helper(depth - 1, &state);
    }
    return nodes;
}

uint64_t perft(int depth) {
    struct state_t state;
    state_init(&state);
    setup_start_position(state);
    return __perft_helper(depth, &state);
}

/* --- Begin Unit Tests --- */

void unittest_move_list_compare() {
    struct move_list_t movelist;
    struct move_list_t rhs;
    struct move_t move;

    ENTER_UNITTEST();
    
    move_list_init(&movelist);
    move_list_init(&rhs);
    move_init(&move);

    UNITTEST_ASSERT_MOVELIST(movelist, rhs);
    move_list_append_move(movelist, 1, 5);
    move_list_append_move(rhs, 1, 5);
    UNITTEST_ASSERT_MOVELIST(movelist, rhs);
    move_list_append_move(movelist, 14, 19);
    move_list_append_move(rhs, 14, 19);
    UNITTEST_ASSERT_MOVELIST(movelist, rhs);
    move.src = 18;
    move.dst = JUMP_UP_RIGHT(move.src);
    move_list_append_capture(movelist, move);
    UNITTEST_ASSERT_NEQU(move_list_compare(&movelist, &rhs), 0);
    move_list_append_capture(rhs, move);
    UNITTEST_ASSERT_MOVELIST(movelist, rhs);
    move.src = 9;
    move.dst = JUMP_UP_RIGHT(move.src);

    EXIT_UNITTEST();
}

void unittest_move_list_sort() {
    struct move_list_t lhs;
    struct move_list_t rhs;
    struct move_t movea;
    struct move_t moveb;

    ENTER_UNITTEST();

    move_list_init(&lhs);
    move_list_init(&rhs);
    move_init(&movea);
    move_init(&moveb);

    movea.src = 14;
    movea.dst = JUMP_UP_LEFT(movea.src);
    moveb.src = 15;
    moveb.dst = JUMP_DOWN_LEFT(moveb.src);

    /* lhs has movea then moveb */
    move_list_append_capture(lhs, movea);
    move_list_append_capture(lhs, moveb);
    move_list_append_move(lhs, 21, UP_LEFT(21));
    move_list_append_move(lhs, 3, UP_RIGHT(3));

    /* rhs has moveb then movea */
    move_list_append_capture(rhs, moveb);
    move_list_append_move(rhs, 21, UP_LEFT(21));
    move_list_append_capture(rhs, movea);
    move_list_append_move(rhs, 3, UP_RIGHT(3));    

    UNITTEST_ASSERT_NEQU(move_list_compare(&lhs, &rhs), 0);
    
    move_list_sort(&lhs);
    move_list_sort(&rhs);

    UNITTEST_ASSERT_MOVELIST(lhs, rhs);

    move_list_append_move(lhs, 2, UP_LEFT(2));
    move_list_append_move(lhs, 18, UP_LEFT(18));
    move_list_append_move(lhs, 2, UP_RIGHT(2));
    
    move_list_append_move(rhs, 18, UP_LEFT(18));
    move_list_append_move(rhs, 2, UP_LEFT(2));
    move_list_append_move(rhs, 2, UP_RIGHT(2));

    EXIT_UNITTEST();
}

void unittest_generate_moves() {
    struct state_t state;
    struct move_list_t movelist;
    struct move_list_t expected;
    ENTER_UNITTEST();

    /* black on 14 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(18));
    move_list_append_move(expected, SQR(14), SQR(19));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 14 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(14);
    state.moves = 1; /* make it white to move */
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(10));
    move_list_append_move(expected, SQR(14), SQR(11));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black king on 14 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black_kings = SQUARE(14);
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(18));
    move_list_append_move(expected, SQR(14), SQR(19));
    move_list_append_move(expected, SQR(14), SQR(10));
    move_list_append_move(expected, SQR(14), SQR(11));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white king on 14 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white_kings = SQUARE(14);
    state.moves = 1; /* make it white to move */
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(18));
    move_list_append_move(expected, SQR(14), SQR(19));
    move_list_append_move(expected, SQR(14), SQR(10));
    move_list_append_move(expected, SQR(14), SQR(11));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 14, white on 18 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    state.white = SQUARE(18);
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(19));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 14, black on 10 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(14);
    state.black = SQUARE(10);
    state.moves = 1; /* make it white to move */
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(14), SQR(11));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 32 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(32);
    state.black = 0;
    state.moves = 1; /* make it white to move */
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(32), SQR(28));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 29 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(29);
    state.moves = 1; /* make it white to move */
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(29), SQR(25));
    move_list_append_move(expected, SQR(29), SQR(26));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 1 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(1);
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(1), SQR(5));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 4 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(4);
    generate_moves(&state, &movelist);
    move_list_append_move(expected, SQR(4), SQR(7));
    move_list_append_move(expected, SQR(4), SQR(8));
    UNITTEST_ASSERT_MOVELIST(movelist, expected);    
    
    EXIT_UNITTEST();
}

void unittest_generate_captures() {
    struct state_t state;
    struct move_list_t movelist;
    struct move_list_t expected;
    ENTER_UNITTEST();

    /* black on 14 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    generate_captures(&state, &movelist);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 14, white on 19 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    state.white = SQUARE(19);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 14, 23);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 14, black on 10 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(10);
    state.white = SQUARE(14);
    state.moves = 1; /* make it white to move */
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 14, 5);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 14, white on 19 and 18*/
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    state.white = SQUARE(19) | SQUARE(18);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 14, 23);
    APPEND_CAPTURE(&expected, 14, 21);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    
    /* black on 14, white on 18, 19, 10, 11 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(14);
    state.white = SQUARE(19) | SQUARE(18) | SQUARE(10) | SQUARE(11);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 14, 23);
    APPEND_CAPTURE(&expected, 14, 21);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black king on 14, white on 18, 19, 10, 11 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black_kings = SQUARE(14);
    state.white = SQUARE(19) | SQUARE(18) | SQUARE(10) | SQUARE(11);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 14, 5);
    APPEND_CAPTURE(&expected, 14, 7);
    APPEND_CAPTURE(&expected, 14, 21); 
    APPEND_CAPTURE(&expected, 14, 23);   
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 9, white on 13 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(9);
    state.white = SQUARE(13);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 9, 18);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 17, white on 21, 13 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(17);
    state.white = SQUARE(21) | SQUARE(13);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 17, 26);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 24, white on 28, 20 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(24);
    state.white = SQUARE(28) | SQUARE(20);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 24, 31);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 32, black on 28 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(28);
    state.white = SQUARE(32);
    state.moves = 1;
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 32, 23);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* black on 1, white on 5 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(1);
    state.white = SQUARE(5);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 1, 10);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);    

    /* black on 4, white on 7, 8 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(4);
    state.white = SQUARE(7) | SQUARE(8);
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 4, 11);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 29, black on 25, 26 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(29);    
    state.black = SQUARE(25) | SQUARE(26);
    state.moves = 1;
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 29, 22);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);

    /* white on 32, black on 28 */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.white = SQUARE(32);    
    state.black = SQUARE(28);
    state.moves = 1;
    generate_captures(&state, &movelist);
    APPEND_CAPTURE(&expected, 32, 23);
    UNITTEST_ASSERT_MOVELIST(movelist, expected);        
        
    EXIT_UNITTEST();
}

/*
---------------------------------
|   |29 |   |30 |   |31 |   |32 |
---------------------------------
|25 |   |26 |   |27 |   |28 |   |
---------------------------------
|   |21 |   |22 |   |23 |   |24 |
---------------------------------
|17 |   |18 |   |19 |   |20 |   |
---------------------------------
|   |13 |   |14 |   |15 |   |16 |
---------------------------------
| 9 |   |10 |   |11 |   |12 |   |
---------------------------------
|   | 5 |   | 6 |   | 7 |   | 8 |
---------------------------------
| 1 |   | 2 |   | 3 |   | 4 |   |
---------------------------------
*/
void unittest_generate_multicaptures() {
    struct state_t state;
    struct move_list_t movelist;
    struct move_list_t expected;
    struct move_t move;
    ENTER_UNITTEST();

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   | w |   | w |   | w |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   | w |   | w |   |15 |   |16 |
    ---------------------------------
    | 9 |   |10 |   |11 |   |12 |   |
    ---------------------------------
    |   | w |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | b |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);    
    state.black = SQUARE(1);
    state.black_kings = 0;    
    state.white = SQUARE(5) | SQUARE(14) | SQUARE(22) | SQUARE(23) | SQUARE(13) | SQUARE(21);
    state.white_kings = 0;
    move_init(&move);
    move.src = SQR(1);
    move.path[0] = SQR(10);
    move.path[1] = SQR(17);
    move.dst = SQR(26);
    move.pathlen = 2;
    move_list_append_capture(expected, move);
    move.path[0] = SQR(10);
    move.path[1] = SQR(19);
    move.pathlen = 2;
    move_list_append_capture(expected, move);
    move.dst = SQR(28);
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);    
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 1-10-17-26, 1-10-19-26, 1-10-19-28 */
    
    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   | w |
    ---------------------------------
    |25 |   |26 |   |27 |   | b |   |
    ---------------------------------
    |   |21 |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   |18 |   | b |   | b |   |
    ---------------------------------
    |   |13 |   |14 |   |15 |   |16 |
    ---------------------------------
    | 9 |   | b |   | b |   | b |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);        
    state.moves = 1;
    state.black = SQUARE(28) | SQUARE(19) | SQUARE(10) | SQUARE(11) | SQUARE(20) | SQUARE(12);
    state.black_kings = 0;
    state.white = SQUARE(32);
    state.white_kings = 0;
    move_init(&move);
    move.src = SQR(32);
    move.path[0] = SQR(23);
    move.path[1] = SQR(14);
    move.pathlen = 2;
    move.dst = SQR(5);
    move_list_append_capture(expected, move);
    move.path[1] = SQR(14);
    move.dst = SQR(7);
    move_list_append_capture(expected, move);
    move.path[1] = SQR(16);
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);    
    /* Move list: 32-23-14-5, 32-23-14-7, 32-23-16-7 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   |21 |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   | W |   | W |   |20 |   |
    ---------------------------------
    |   |13 |   | B |   |15 |   |16 |
    ---------------------------------
    | 9 |   | w |   | w |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);        
    state.black = 0;
    state.black_kings = SQUARE(14);
    state.white = SQUARE(10) | SQUARE(11);
    state.white_kings = SQUARE(18) | SQUARE(19);
    move_init(&move);
    move.pathlen = 0;    
    move.src = SQR(14);
    move.dst = SQR(5);
    move_list_append_capture(expected, move);
    move.dst = SQR(7);
    move_list_append_capture(expected, move);
    move.dst = SQR(21);
    move_list_append_capture(expected, move);    
    move.dst = SQR(23);
    move_list_append_capture(expected, move);    
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 14-5, 14-7, 14-21, 14-23 */

        /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   |21 |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   | B |   | B |   |20 |   |
    ---------------------------------
    |   |13 |   | W |   |15 |   |16 |
    ---------------------------------
    | 9 |   | b |   | b |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.moves = 1;
    state.white = 0;
    state.white_kings = SQUARE(14);
    state.black = SQUARE(10) | SQUARE(11);
    state.black_kings = SQUARE(18) | SQUARE(19);
    move_init(&move);
    move.pathlen = 0;    
    move.src = SQR(14);
    move.dst = SQR(5);
    move_list_append_capture(expected, move);
    move.dst = SQR(7);
    move_list_append_capture(expected, move);
    move.dst = SQR(21);
    move_list_append_capture(expected, move);    
    move.dst = SQR(23);
    move_list_append_capture(expected, move);    
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 14-5, 14-7, 14-21, 14-23 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   |21 |   |22 |   | W |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | w |   |15 |   |16 |
    ---------------------------------
    | 9 |   | B |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(10);        
    state.white = SQUARE(14);
    state.white_kings = SQUARE(23);
    move_init(&move);
    move.src = SQR(10);
    move.path[0] = SQR(19);
    move.dst = SQR(28);
    move.pathlen = 1;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);    
    /* Move list: 10-19-28 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   | B |   |
    ---------------------------------
    |   |21 |   |22 |   | W |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | w |   |15 |   |16 |
    ---------------------------------
    | 9 |   |10 |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(28);        
    state.white = SQUARE(14);
    state.white_kings = SQUARE(23);
    move_init(&move);
    move.src = SQR(28);
    move.path[0] = SQR(19);
    move.dst = SQR(10);
    move.pathlen = 1;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 28-19-10 */

        /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    |25 |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   |21 |   |22 |   | B |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | b |   |15 |   |16 |
    ---------------------------------
    | 9 |   | W |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.moves = 1;
    state.black = SQUARE(14);
    state.black_kings = SQUARE(23);        
    state.white = 0;
    state.white_kings = SQUARE(10);
    move_init(&move);
    move.src = SQR(10);
    move.path[0] = SQR(19);
    move.dst = SQR(28);
    move.pathlen = 1;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 10-19-28 */    

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    | B |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   | W |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | w |   |15 |   |16 |
    ---------------------------------
    | 9 |   |10 |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(25);        
    state.white = SQUARE(14);
    state.white_kings = SQUARE(21);
    move_init(&move);
    move.src = SQR(25);
    move.path[0] = SQR(18);
    move.dst = SQR(11);
    move.pathlen = 1;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 25-18-11 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    | B |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   | W |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | w |   | W |   |16 |
    ---------------------------------
    | 9 |   |10 |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(25);        
    state.white = SQUARE(14);
    state.white_kings = SQUARE(21) | SQUARE(15);
    move_init(&move);
    move.src = SQR(25);
    move.path[0] = SQR(18);
    move.path[1] = SQR(11);
    move.dst = SQR(20);
    move.pathlen = 2;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 25-18-11-20 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   |32 |
    ---------------------------------
    | B |   |26 |   |27 |   |28 |   |
    ---------------------------------
    |   | W |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   |18 |   |19 |   |20 |   |
    ---------------------------------
    |   |13 |   | w |   | W |   |16 |
    ---------------------------------
    | 9 |   |10 |   |11 |   |12 |   |
    ---------------------------------
    |   | 5 |   | W |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | w |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(25);        
    state.white = SQUARE(14) | SQUARE(2);
    state.white_kings = SQUARE(21) | SQUARE(15) | SQUARE(6);
    move_init(&move);
    move.src = SQR(25);
    move.path[0] = SQR(18);
    move.path[1] = SQR(11);
    move.dst = SQR(20);
    move.pathlen = 2;
    move_list_append_capture(expected, move);
    generate_captures(&state, &movelist);        
    UNITTEST_ASSERT_MOVELIST(movelist, expected);
    /* Move list: 25-18-11-20 */

    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   | B |
    ---------------------------------
    |25 |   | w |   | W |   | w |   |
    ---------------------------------
    |   |21 |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   | W |   | W |   | W |   |
    ---------------------------------
    |   |13 |   |14 |   |15 |   |16 |
    ---------------------------------
    | 9 |   | w |   | w |   | W |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */
    state_init(&state);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = 0;
    state.black_kings = SQUARE(32);        
    state.white = SQUARE(10) | SQUARE(11) | SQUARE(26) | SQUARE(28);
    state.white_kings = SQUARE(12) | SQUARE(18) | SQUARE(19) | SQUARE(20) | SQUARE(27);
    /* move_init(&move); */
    /* move.src = SQR(25); */
    /* move.path[0] = SQR(18); */
    /* move.path[1] = SQR(11); */
    /* move.dst = SQR(20); */
    /* move.pathlen = 2; */
    /* move_list_append_capture(expected, move); */
    generate_captures(&state, &movelist);        
    /* UNITTEST_ASSERT_MOVELIST(movelist, expected); */
    /* print_move_list(movelist); */
    /* Move list: 32-23-30-21-14-23-16-7-14-5, 32-23-30-21-14-7-16-23-14-5,
                  32-23-14-21-30-23-16-7-14-5, 32-23-14-7-16-23-30-21-14-5,
                  32-23-16-7-14-21-30-23-14-5, 32-23-16-7-14-23-30-21-14-5 */



    /*
    ---------------------------------
    |   |29 |   |30 |   |31 |   | W |
    ---------------------------------
    |25 |   | b |   | b |   | b |   |
    ---------------------------------
    |   |21 |   |22 |   |23 |   |24 |
    ---------------------------------
    |17 |   | b |   | b |   | b |   |
    ---------------------------------
    |   |13 |   |14 |   |15 |   |16 |
    ---------------------------------
    | 9 |   | b |   | b |   | b |   |
    ---------------------------------
    |   | 5 |   | 6 |   | 7 |   | 8 |
    ---------------------------------
    | 1 |   | 2 |   | 3 |   | 4 |   |
    ---------------------------------
    */    
    state_init(&state);
    state.moves = 1;
    move_init(&move);
    move_list_init(&movelist);
    move_list_init(&expected);
    state.black = SQUARE(10) | SQUARE(11) | SQUARE(26) | SQUARE(28) | SQUARE(12) | SQUARE(18) | SQUARE(19) | SQUARE(20) | SQUARE(27);
    state.black_kings = 0;
    state.white = 0;
    state.white_kings = SQUARE(32);
    generate_captures(&state, &movelist);
    /* print_move_list(movelist); printf("\n"); */

    /* Move list: 32-23-14-7-16-23-30-21-14-5, 32-23-14-21-30-23-16-7-14-5,
                  32-23-16-7-14-21-30-23-14-5, 32-23-16-7-14-23-30-21-14-5,
                  32-23-30-21-14-7-16-23-14-5, 32-23-30-21-14-23-16-7-14-5 */

    EXIT_UNITTEST();
}

void unittest_make_move() {
    struct state_t state;
    struct move_t move;
    struct move_list_t movelist;    
    ENTER_UNITTEST();

    /* -- black pawn  -- */
    
    /* black move */
    state_init(&state);
    state.black = SQUARE(11);
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(!!is_noncapture(&move), TRUE);
    UNITTEST_ASSERT(!!is_capture(&move), FALSE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(15)), TRUE);

    /* white move */
    state_init(&state);
    state.white = SQUARE(11);
    state.moves = 1;
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(!!is_noncapture(&move), TRUE);
    UNITTEST_ASSERT(!!is_capture(&move), FALSE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.black | state.black_kings | state.white_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(15)), TRUE);

    /* black king move */
    state_init(&state);
    state.black_kings = SQUARE(11);
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(!!is_noncapture(&move), TRUE);
    UNITTEST_ASSERT(!!is_capture(&move), FALSE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(15)), TRUE);

    /* white king move */
    state_init(&state);
    state.white_kings = SQUARE(11);
    state.moves = 1;
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(!!is_noncapture(&move), TRUE);
    UNITTEST_ASSERT(!!is_capture(&move), FALSE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.black_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(15)), TRUE);

    /* black capture white pawn */
    state_init(&state);
    state.black = SQUARE(11);
    state.white = SQUARE(15);
    move_init(&move);    
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(20)), TRUE);

    /* white capture black pawn */
    state_init(&state);
    state.black = SQUARE(15);
    state.white = SQUARE(20);
    state.moves = 1;
    move_init(&move);    
    move.src = SQR(20);
    move.dst = SQR(11);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.black | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(20)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(11)), TRUE);

    /* black capture white king */
    state_init(&state);
    state.black = SQUARE(11);
    state.white_kings = SQUARE(15);
    move_init(&move);        
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(20)), TRUE);

    /* white capture black king */
    state_init(&state);
    state.black_kings = SQUARE(15);
    state.white = SQUARE(20);
    state.moves = 1;
    move_init(&move);    
    move.src = SQR(20);
    move.dst = SQR(11);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.black | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(20)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(11)), TRUE);

    /* -- black king -- */
    state_init(&state);
    state.black_kings = SQUARE(11);
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(is_noncapture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(15)), TRUE);

    /* -- white king -- */
    state_init(&state);
    state.white_kings = SQUARE(11);
    state.moves = 1;
    move_init(&move);
    move.src = SQR(11);
    move.dst = SQR(15);
    UNITTEST_ASSERT(!!is_noncapture(&move), TRUE);
    UNITTEST_ASSERT(!!is_capture(&move), FALSE);    
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.black_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(15)), TRUE);

    /* black king captures white pawn */
    state_init(&state);
    state.black_kings = SQUARE(11);
    state.white = SQUARE(15);
    move_init(&move);        
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(20)), TRUE);

    /* white king captures black pawn */
    state_init(&state);
    state.white_kings = SQUARE(11);
    state.black = SQUARE(15);
    state.moves = 1;
    move_init(&move);        
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.black_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(20)), TRUE);

    /* black king capture white king */
    state_init(&state);
    state.black_kings = SQUARE(11);
    state.white_kings = SQUARE(15);
    move_init(&move);        
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(20)), TRUE);

    /* white king capture black king */
    state_init(&state);
    state.white_kings = SQUARE(11);
    state.black_kings = SQUARE(15);
    state.moves = 1;
    move_init(&move);        
    move.src = SQR(11);
    move.dst = SQR(20);
    UNITTEST_ASSERT(!!is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(!!is_capture(&move), TRUE);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.black_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(11)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(20)), TRUE);

    /* black multi-jump */
    /* 1 - (x5) 10 - (x13) - 17 - (x21) - 26 */
    state_init(&state);
    state.black = SQUARE(1);
    state.white = SQUARE(5) | SQUARE(13) | SQUARE(21);
    move_init(&move);
    move.src = SQR(1);
    move.dst = SQR(26);
    move.pathlen = 2;
    move.path[0] = SQR(10);
    move.path[1] = SQR(17);
    UNITTEST_ASSERT(is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(is_capture(&move), TRUE);
    move_list_init(&movelist);
    get_moves(&state, &movelist);
    UNITTEST_ASSERT(move_list_num_moves(movelist), 1);
    UNITTEST_ASSERT(move_compare(&move, &(movelist.moves[0])), 0);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black_kings, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(1)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black, SQR(26)), TRUE);

    /* white multi-jump */
    /* 26 - (x21) - 17 - (x13) - 10 - (x5) - 1 */
    state_init(&state);
    state.white = SQUARE(26);
    state.black = SQUARE(5) | SQUARE(13) | SQUARE(21);
    state.moves = 1;
    move_init(&move);
    move.src = SQR(26);
    move.dst = SQR(1);
    move.pathlen = 2;
    move.path[0] = SQR(17);
    move.path[1] = SQR(10);
    UNITTEST_ASSERT(is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(is_capture(&move), TRUE);
    move_list_init(&movelist);
    get_moves(&state, &movelist);
    UNITTEST_ASSERT(move_list_num_moves(movelist), 1);
    UNITTEST_ASSERT(move_compare(&move, &(movelist.moves[0])), 0);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.black | state.white | state.black_kings, 0); /* white pawn was promoted */
    UNITTEST_ASSERT(!!OCCUPIED(state.white, SQR(26)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(1)), TRUE);

    /* black king multi-jump */
    /* 1 - (x5) 10 - (x13) - 17 - (x21) - 26 */
    state_init(&state);
    state.black_kings = SQUARE(1);
    state.white = SQUARE(5) | SQUARE(13) | SQUARE(21);
    move_init(&move);
    move.src = SQR(1);
    move.dst = SQR(26);
    move.pathlen = 2;
    move.path[0] = SQR(10);
    move.path[1] = SQR(17);
    UNITTEST_ASSERT(is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(is_capture(&move), TRUE);
    move_list_init(&movelist);
    get_moves(&state, &movelist);
    UNITTEST_ASSERT(move_list_num_moves(movelist), 1);
    UNITTEST_ASSERT(move_compare(&move, &(movelist.moves[0])), 0);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.white_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(1)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.black_kings, SQR(26)), TRUE);

    /* white king multi-jump */
    /* 1 - (x5) 10 - (x13) - 17 - (x21) - 26 */
    state_init(&state);
    state.white_kings = SQUARE(1);
    state.black = SQUARE(5) | SQUARE(13) | SQUARE(21);
    state.moves = 1;
    move_init(&move);
    move.src = SQR(1);
    move.dst = SQR(26);
    move.pathlen = 2;
    move.path[0] = SQR(10);
    move.path[1] = SQR(17);
    UNITTEST_ASSERT(is_noncapture(&move), FALSE);
    UNITTEST_ASSERT(is_capture(&move), TRUE);
    move_list_init(&movelist);
    get_moves(&state, &movelist);
    UNITTEST_ASSERT(move_list_num_moves(movelist), 1);
    UNITTEST_ASSERT(move_compare(&move, &(movelist.moves[0])), 0);
    make_move(&state, &move);
    UNITTEST_ASSERT(state.white | state.black_kings | state.black, 0);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(1)), FALSE);
    UNITTEST_ASSERT(!!OCCUPIED(state.white_kings, SQR(26)), TRUE);

    EXIT_UNITTEST();
}



void unittest_perft() {
    int depth;
    // from www.aartbik.com/MISC/checkers.html
    const uint64_t expected[] = {
         1
        ,7
        ,49
        ,302
        ,1469
        ,7361
        ,36768
        ,179740
        /* ,845931 */
        /* ,3963680 */
        /* ,18391564 */
        /* ,85242128 */
        /* ,388623673 */
        /* ,1766623630 */
        /* ,7978439499 */
        /* ,36263167175 */
        /* ,165629569428 */
        /* ,758818810990 */
        /* ,3493881706141 */
        /* ,16114043592799 */
        /* ,74545030871553 */
        /* ,345100524480819 */
        /* ,1602372721738102 */
        /* ,7437536860666213 */
        /* ,34651381875296000 */
        /* ,161067479882075800 */
        /* ,752172458688067137 */
        /* ,3499844183628002605 */
        /* ,16377718018836900735 */
    };
    uint64_t nodes;
    struct timeval start;
    struct timeval end;
    ENTER_UNITTEST();

    for (depth = 0; depth < (sizeof(expected) / sizeof(expected[0])); ++depth) {
        gettimeofday(&start, 0);
        nodes = perft(depth);
        gettimeofday(&end, 0);
        
        printf("perft(%d) = %lu in %lu ms.\n", depth, nodes, ((end.tv_sec * 1000000UL + end.tv_usec) - (start.tv_sec * 1000000UL + start.tv_usec)) / 1000UL);
        /* UNITTEST_ASSERT_MSG(nodes, expected[depth], "%lu"); */
    }

    EXIT_UNITTEST();
}

/* --- End Unit Tests --- */

int main(int argc, char **argv) {
    /* struct state_t state; */
    /* state_init(&state); */
    /* print_board(state); */
    perft(6);    
    /* struct state_t state; */
    /* struct move_list_t moves; */
    /* int depth; */
    
    /* state_init(&state); */
    /* move_list_init(&moves); */

    /* unit tests */
    /* unittest_move_list_compare(); */
    /* unittest_move_list_sort(); */
    /* unittest_generate_moves(); */
    /* unittest_generate_captures(); */
    /* unittest_generate_multicaptures(); */
    /* unittest_make_move(); */
    /* unittest_perft(); */
    
    /* -- to show starting position -- */
    /* state_init(&state); */
    /* setup_start_position(state); */
    /* print_board(state); */

    /* for (depth = 0; depth < 12; ++depth) { */
    /*     printf("moves at depth %d = %lu\n", depth, perft(depth)); */
    /* } */

    /* printf("Bye.\n"); */
    return 0;
}
