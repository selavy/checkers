/* #define _SVID_SOURCE */
/* #define _POSIX_C_SOURCE 200809L */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
/* #include <locale.h> */
/* #include <regex.h> */
/* #include <ctype.h> */

#define MAX_PATH 8
#define MAX_MOVES 32
#define COLUMNS 8
#define ROWS 8
#define SQUARES 32
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

static char * unittest = 0;
#define ENTER_UNITTEST(name) do { unittest = name; } while(0)
#define UNITTEST_ASSERT_MSG(msg, line) do { fprintf(stderr, "Unit Test [%s] failed on line: %d\n", unittest, line); exit(1); } while(0)
#define UNITTEST_ASSERT(msg) UNITTEST_ASSERT_MSG(msg, __LINE__)

/* moves are 1-indexed so 0 can indicate that the path is empty */
struct move_t {
    uint8_t src; /* square started on */
    uint8_t dst; /* square ended on */
    uint8_t path[MAX_PATH]; /* path traveled if this was a multi-jump */
};
#define move_init(move) memset(&(move), 0, sizeof(move))

/* move_compare: compare 2 moves for equality
 * 0  => equal
 * 1  => lhs > rhs
 * -1 => lhs < rhs
 */
int move_compare(struct move_t* lhs, struct move_t* rhs) {
    int ret = 0;
    
    if (lhs->src != rhs->src) {
        ret = lhs->src - rhs->src;
    } else if (lhs->dst != rhs->dst) {
        ret = lhs->dst - rhs->dst;
    } else {
        ret = memcmp(&(lhs->path[0]), &(rhs->path[0]), sizeof(lhs->path[0]) * MAX_PATH);
    }
    return ret;
}

struct move_list_t {
    struct move_t moves[MAX_MOVES];
    int njumps; /* number of jumps */    
    int nmoves; /* number of regular moves */
};
#define move_list_num_moves(list) ((list).njumps + (list).nmoves)

/* move_list_compare: compare 2 move_lists for strict equality
 * 0  => equal
 * 1  => lhs > rhs
 * -1 => lhs < rhs
 */
int __move_list_compare(struct move_list_t* lhs, struct move_list_t* rhs) {
    int ret;
    
    if (lhs->njumps != rhs->njumps) {
        ret = lhs->njumps - rhs->njumps;
    } else if (lhs->nmoves != rhs->nmoves) {
        ret = lhs->nmoves - rhs->nmoves;
    } else if (move_list_num_moves(*lhs) != move_list_num_moves(*rhs)) {
        ret = move_list_num_moves(*lhs) - move_list_num_moves(*rhs);
    } else {
        ret = memcmp(&(lhs->moves[0]), &(rhs->moves[0]), sizeof(lhs->moves[0]) * MAX_MOVES);
    }
    return ret;
}
#define move_list_compare(lhs, rhs) __move_list_compare(&lhs, &rhs)

int __move_compare_generic(const void* lhs, const void* rhs) {
    return move_compare((struct move_t*)lhs, (struct move_t*)rhs);
}

void move_list_sort(struct move_list_t* list) {
    int nmoves = move_list_num_moves(*list);
    if (nmoves != 0) {
        qsort(&(list->moves[0]), nmoves, sizeof(list->moves), &__move_compare_generic);
    }
}

/* TODO: add is_capture parameter */
void __print_move(FILE* file, struct move_t* move /*, boolean is_capture */) {
    int i;
    /* if (is_capture) { */
    fprintf(file, "%d-", move->src);
    for (i = 0; i < MAX_PATH && move->path[i]; ++i) {
        fprintf(file, "%d-", move->path[i]);
    }
    fprintf(file, "%d", move->dst);
    /* } else { */
    /*     fprintf(file, "%d-%d", move->src, move->dst); */
    /* } */
}
#define print_move(move, is_capture) __print_move(stdout, &(move))

int __move_list_init(struct move_list_t* list) {
    list->njumps = 0;
    list->nmoves = 0;
    memset(&(list->moves[0]), 0, sizeof(list->moves[0]) * MAX_MOVES);
    return 0;
}
#define move_list_init(list) __move_list_init(&(list))

void __move_list_append_move(struct move_list_t* list, int src, int dst) {
    struct move_t* const move = &(list->moves[move_list_num_moves(*list)]);
    move->src = src;
    move->dst = dst;
    ++list->nmoves;
}
#define move_list_append_move(list, src, dst) __move_list_append_move(&list, src, dst)
/* #define move_list_append_move(list, src, dst) do {              \ */
/*         (list).moves[move_list_num_moves(list)].src = (src);    \ */
/*         (list).moves[move_list_num_moves(list)].dst = (dst);    \ */
/*         ++(list).nmoves;                                        \ */
/*     } while(0) */
void __move_list_append_capture(struct move_list_t* list, struct move_t* move) {
    struct move_t* const movep = &(list->moves[move_list_num_moves(*list)]);
    ++list->njumps;
    movep->src = move->src;
    movep->dst = move->dst;
    memcpy(&(movep->path[0]), &(move->path[0]), sizeof(move->path[0]) * sizeof(move->path));
}
#define move_list_append_capture(list, move) __move_list_append_capture(&(list), &(move))

void __print_move_list(FILE* file, struct move_list_t* list) {
    int i;
    const int moves = move_list_num_moves(*list);
    if (moves > 0) {
        __print_move(file, &(list->moves[0]));

        for (i = 1; i < moves; ++i) {
            printf(", ");
        __print_move(file, &(list->moves[i]));
        }
    }
}
#define print_move_list(list) __print_move_list(stdout, &(list));

/* 0 indexed */
#define MASK(square) ((square_t)1 << (square))
/* 1 indexed */
#define SQUARE(square) MASK(square-1)
#define PLACE(pieces, square) do { ((pieces) |= MASK(square)); } while(0)
#define CLEAR(pieces, square) do { ((pieces) &= ~MASK(square)); } while(0)
#define OCCUPIED(pieces, square) (pieces & MASK(square))
#define MOVE(pieces, from, to) do { PLACE(pieces, to); CLEAR(pieces, from); } while(0)
#define ROW(square) (((square) / 4) - 1)
#define WHITE(state) ((state).white | (state).white_kings)
#define BLACK(state) ((state).black | (state).black_kings)
#define KINGS(state) ((state).black_kings | (state).white_kings)
#define PAWNS(state) ((state).white | (state).black)
#define FULLBOARD(state) (WHITE(state) | BLACK(state))
#define ODDROW(square) ((square / 4) & 1)
#define UP_LEFT(square) (ODDROW(square) ? (square) + 4 : (square) + 3)
#define UP_RIGHT(square) (ODDROW(square) ? (square) + 5 : (square) + 4)
#define DOWN_LEFT(square) (ODDROW(square) ? (square) - 4 : (square) - 5)
#define DOWN_RIGHT(square) (ODDROW(square) ? (square) - 3 : (square) - 4)
#define JUMP_UP_LEFT(square) (UP_LEFT(UP_LEFT(square)))
#define JUMP_UP_RIGHT(square) (UP_RIGHT(UP_RIGHT(square)))
#define JUMP_DOWN_LEFT(square) (DOWN_LEFT(DOWN_LEFT(square)))
#define JUMP_DOWN_RIGHT(square) (DOWN_RIGHT(DOWN_RIGHT(square)))
#define TOP(square) (MASK(square) & (MASK(28) | MASK(29) | MASK(30) | MASK(31)))
#define LEFT(square) (MASK(square) & (MASK(0) | MASK(8) | MASK(16) | MASK(24)))
#define RIGHT(square) (MASK(square) & (MASK(7) | MASK(15) | MASK(23) | MASK(31)))
#define BOTTOM(square) (MASK(square) & (MASK(0) | MASK(1) | MASK(2) | MASK(3)))
#define TOP2(square) 1
#define LEFT2(square) 1
#define RIGHT2(square) 1
#define BOTTOM2(square) 1

void __state_init(struct state_t* state) {
    memset(state, 0, sizeof(*state));
}
#define state_init(state) do { memset(&(state), 0, sizeof(state)); } while(0)

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
        printf("\n    ---------------------------------\n    |");
        for (c = 0; c < COLUMNS; ++c) {
            if (skip) {
                fprintf(file, "   |");
            } else {
                /* fprintf(file, " %d |", square); */
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
    printf("\n    ---------------------------------\n");       
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
     (state).black = 4095;                      \
     (state).white_kings = 0;                   \
     (state).black_kings = 0;                   \
     (state).moves = 0;

/* TODO: generate bitmaps to test for sides of board squares */
int generate_captures(struct state_t* state, struct move_list_t* moves) {
    square_t square;
    struct move_t move;
    move_init(move);
    if (black_move(*state)) {
        for (square = 0; square < SQUARES; ++square) {
            if (OCCUPIED(BLACK(*state), square)) {
                if (OCCUPIED(WHITE(*state), UP_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_LEFT(square))) {
                    move.src = square+1;
                    move.dst = JUMP_UP_LEFT(square);
                    move_list_append_capture(*moves, move);
                }
                if (OCCUPIED(WHITE(*state), UP_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_RIGHT(square))) {
                    move.src = square+1;
                    move.dst = JUMP_UP_RIGHT(square);
                    move_list_append_capture(*moves, move);                    
                }

                if (OCCUPIED(state->black_kings, square)) {
                    /* check king moves */
                    if (OCCUPIED(WHITE(*state), DOWN_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_LEFT(square))) {
                        move.src = square + 1;
                        move.dst = JUMP_DOWN_LEFT(square);
                        move_list_append_capture(*moves, move);                        
                    }
                    if (OCCUPIED(WHITE(*state), DOWN_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_RIGHT(square))) {
                        move.src = square + 1;
                        move.dst = JUMP_DOWN_RIGHT(square);
                        move_list_append_capture(*moves, move);                        
                    }
                }
            }
        }
    } else {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(WHITE(*state), square)) {
                if (OCCUPIED(BLACK(*state), DOWN_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_LEFT(square))) {
                    move.src = square+1;
                    move.dst = JUMP_DOWN_LEFT(square);
                    move_list_append_capture(*moves, move);                    
                }
                if (OCCUPIED(BLACK(*state), DOWN_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_DOWN_RIGHT(square))) {
                    move.src = square+1;
                    move.dst = JUMP_DOWN_RIGHT(square);
                    move_list_append_capture(*moves, move);                    
                }

                if (OCCUPIED(state->white_kings, square)) {
                    /* check king moves */
                    if (OCCUPIED(BLACK(*state), UP_LEFT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_LEFT(square))) {
                        move.src = square + 1;
                        move.dst = JUMP_UP_LEFT(square);
                        move_list_append_capture(*moves, move);
                    }
                    if (OCCUPIED(BLACK(*state), UP_RIGHT(square)) && !OCCUPIED(FULLBOARD(*state), JUMP_UP_RIGHT(square))) {
                        move.src = square + 1;
                        move.dst = JUMP_UP_RIGHT(square);
                        move_list_append_capture(*moves, move);
                    }
                }
            }
        }        
    }
    return 0;
}

int generate_moves(struct state_t* state, struct move_list_t* moves) {
    square_t square;
    if (black_move(*state)) {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(BLACK(*state), square)) {
                if (!TOP(square)) {
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                        move_list_append_move(*moves, square+1, UP_LEFT(square)+1);
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                        move_list_append_move(*moves, square+1, UP_RIGHT(square)+1);
                    }
                }

                if (OCCUPIED(state->black_kings, square) && !BOTTOM(square)) {
                    /* check king moves */
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                        move_list_append_move(*moves, square+1, DOWN_LEFT(square)+1);
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                        move_list_append_move(*moves, square+1, DOWN_RIGHT(square)+1);
                    }
                }
            }
        }
    } else {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(WHITE(*state), square)) {
                if (!BOTTOM(square)) {
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                        move_list_append_move(*moves, square+1, DOWN_LEFT(square)+1);
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                        move_list_append_move(*moves, square+1, DOWN_RIGHT(square)+1);
                    }
                }
                
                if (OCCUPIED(state->white_kings, square) && !TOP(square)) {
                    /* check king moves */
                    if (!LEFT(square) && !OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                        move_list_append_move(*moves, square+1, UP_LEFT(square)+1);
                    }
                    if (!RIGHT(square) && !OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                        move_list_append_move(*moves, square+1, UP_RIGHT(square)+1);
                    }
                }
            }
        }        
    }
    return 0;
}

void unittest_move_list_compare() {
    struct move_list_t movelist;
    struct move_list_t rhs;
    struct move_t move;

    ENTER_UNITTEST("move_list_compare");
    
    move_list_init(movelist);
    move_list_init(rhs);

    assert(move_list_compare(movelist, rhs) == 0);
    move_list_append_move(movelist, 1, 5);
    move_list_append_move(rhs, 1, 5);
    assert(move_list_compare(movelist, rhs) == 0);
    move_list_append_move(movelist, 14, 19);
    move_list_append_move(rhs, 14, 19);
    assert(move_list_compare(movelist, rhs) == 0);
    move.src = 18;
    move.dst = JUMP_UP_RIGHT(move.src);
    move_list_append_capture(movelist, move);
    assert(move_list_compare(movelist, rhs) != 0);
    move_list_append_capture(rhs, move);
    assert(move_list_compare(movelist, rhs) == 0);
    move.src = 9;
    move.dst = JUMP_UP_RIGHT(move.src);

    printf("Passed move_list_compare.\n");
}

void unittest_move_list_sort() {
    struct move_list_t lhs;
    struct move_list_t rhs;
    struct move_t movea;
    struct move_t moveb;

    ENTER_UNITTEST("move_list_sort");

    move_list_init(lhs);
    move_list_init(rhs);

    movea.src = 14;
    movea.dst = JUMP_UP_LEFT(movea.src);
    moveb.src = 15;
    moveb.dst = JUMP_DOWN_LEFT(moveb.src);

    /* lhs has movea then moveb */
    move_list_append_capture(lhs, movea);
    move_list_append_capture(lhs, moveb);

    /* rhs has moveb then movea */
    move_list_append_capture(rhs, moveb);
    move_list_append_capture(rhs, movea);

    assert(move_list_compare(lhs, rhs) != 0);

    move_list_sort(&lhs);
    move_list_sort(&rhs);

    UNITTEST_ASSERT(move_list_compare(lhs, rhs) == 0);

    printf("Passed move_list_sort.\n");
}

int main(int argc, char **argv) {
    struct state_t state;
    struct move_list_t movelist;
    
    state_init(state);
    move_list_init(movelist);

    unittest_move_list_compare();
    unittest_move_list_sort();
    
    /* -- to show starting position -- */
    /* state_init(state); */
    /* setup_start_position(state); */
    /* print_board(state); */

    /* state.black = 0; */
    /* state.black_kings = SQUARE(10);     */
    /* state.white = 0; */
    /* state.white_kings = 0; */
    /* state.moves = 0; */

    /* print_board(state); */
    /* generate_captures(&state, &movelist); */
    /* printf("Captures: "); */
    /* print_move_list(movelist); */
    /* printf("\n"); */
    /* generate_moves(&state, &movelist); */

    /* printf("Move list: "); */
    /* print_move_list(movelist); */

    printf("Bye.\n");
    return 0;
}
