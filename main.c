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
    uint8_t src;
    uint8_t dst[MAX_MOVES_PER_TURN - 1]; /* non-move are 0, indexed from 1 */
    uint8_t capture;
};

#define MASK(square) ((square_t)1 << (square))
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

void __state_init(struct state_t* state) {
    memset(state, 0, sizeof(*state));
}
#define state_init(state) do { memset(state, 0, sizeof(*state)); } while(0)

int __black_move(struct state_t* state) {
    return (state->moves & 1) == 0; /* even moves => black to move */
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

struct __move_list_node_t {
    struct move_t* move;
    struct __move_list_node_t* next;
};

struct move_list_t {
    struct __move_list_node_t* head;
    struct __move_list_node_t* tail;
};

int move_list_init(struct move_list_t* list) {
    list->head = 0;
    list->tail = 0;
    return 0;
}

/* move must be heap allocated, list assume ownership of memory */
int move_list_append(struct move_list_t* list, struct move_t* move) {
    struct __move_list_node_t* node = malloc(sizeof(*node));
    if (!node) return -1;
    node->move = move;
    node->next = 0;
    if (list->head == 0) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;
    return 0;
}

void __move_print(FILE* file, struct move_t* move) {
    int i;
    if (move->capture == 0) {
        fprintf(file, "%d-%d", (int)move->src, (int)move->dst[1]);
    }
    else {
        fprintf(file, "%dx%d", (int)move->src, (int)move->dst[1]);
    }
    for (i = 2; i < MAX_MOVES_PER_TURN && move->dst[i]; ++i) {
        fprintf(file, "x%d", (int)move->dst[i]);
    }
}
#define move_print(move) __move_print(stdout, move)

/* FIX ME: broke these when changed move definition */
/* moves should be 1-indexed */
#define move_append(move, square) do {          \
        int i = 0;                              \
        for (; i < MAX_MOVES_PER_TURN; ++i) {   \
            if ((move).moves[i] == 0) {         \
                (move).moves[i] = square;       \
                if (i > 1) {                    \
                    (move).capture = 1;         \
                }                               \
                break;                          \
            }                                   \
        }                                       \
    } while(0)
#define move_set(move, num, square) do {        \
        if (num > 1) {                          \
            (move).capture = 1;                 \
        }                                       \
        (move).moves[num] = (square);           \
    } while(0)
#define move_make(move, from, to) do {          \
        (move).moves[0] = from;                 \
        (move).moves[1] = to;                   \
    } while(0)
#define move_capture(move, from, to) do {       \
        (move).capture = 1;                     \
        (move).moves[0] = from;                 \
        (move).moves[1] = to;                   \
    } while(0)

/* int find_captures(struct state_t* state) { */
/*     square_t square; */
/*     struct move_t move; */
/*     if (black_move(*state)) { */
/*         for (square = 0; square < 32; ++square) { */
/*             if (OCCUPIED(BLACK(*state), square)) { */
/*                 if (OCCUPIED(WHITE(*state), UP_LEFT(square)) && !OCCUPIED(BLACK(*state), JUMP_UP_LEFT(square))) { */
/*                     move_init(&move); */
/*                 } */

                
/*             } */
/*         } */
/*     } */
/*     else { */
/*     } */
/*     return 0; */
/* } */

int generate_moves(struct state_t* state) {
    square_t square;
    if (black_move(*state)) {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(BLACK(*state), square)) {
                if (!OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                    printf("%d - %d\n", square+1, UP_LEFT(square)+1);
                }
                if (!OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                    printf("%d - %d\n", square+1, UP_RIGHT(square)+1);
                }

                if (OCCUPIED(state->black_kings, square)) {
                    /* check kings move */
                    if (!OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                        printf("%d - %d\n", square+1, DOWN_LEFT(square)+1);
                    }
                    if (!OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                        printf("%d - %d\n", square+1, DOWN_RIGHT(square)+1);
                    }
                }
            }
        }
    } else {
        for (square = 0; square < 32; ++square) {
            if (OCCUPIED(WHITE(*state), square)) {
                if (!OCCUPIED(FULLBOARD(*state), DOWN_LEFT(square))) {
                    printf("%d - %d\n", square+1, DOWN_LEFT(square)+1);
                }
                if (!OCCUPIED(FULLBOARD(*state), DOWN_RIGHT(square))) {
                    printf("%d - %d\n", square+1, DOWN_RIGHT(square)+1);
                }
                
                if (OCCUPIED(state->white_kings, square)) {
                    /* check kings move */
                    if (!OCCUPIED(FULLBOARD(*state), UP_LEFT(square))) {
                        printf("%d - %d\n", square+1, UP_LEFT(square)+1);
                    }
                    if (!OCCUPIED(FULLBOARD(*state), UP_RIGHT(square))) {
                        printf("%d - %d\n", square+1, UP_RIGHT(square)+1);
                    }
                }
            }
        }        
    }
    

    return 0;
}

int main(int argc, char **argv) {
    struct state_t state;
    /* struct move_t move; */
    /* move_init(&move); */
    /* state_init(&state); */
    /* __setup_start_position(&state); */
    /* print_board(&state); */
    /* state.black <<= 4; */
    /* print_board(&state); */

    state.black = MASK(14);
    state.black_kings = 0;    
    state.white = 0;
    state.white_kings = 0;
    state.moves = 0;

    print_board(&state);
    generate_moves(&state);

    /* move_capture(move, 1, 5); */
    /* move_append(move, 10); */
    /* move_print(&move); printf("\n"); */

    /* move_init(&move); */
    /* move_make(move, 1, 5); */
    /* move_print(&move); printf("\n"); */

    printf("\n\nBye.\n");
    return 0;
}
