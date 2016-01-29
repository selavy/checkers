#!/usr/bin/env python
import sys

# BOARD = """
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   | X |
# ---------------------------------
# """

BOARD = """
---------------------------------
|   | w |   | w |   | w |   | w |
---------------------------------
| w |   | w |   | w |   | w |   |
---------------------------------
|   | w |   | w |   | w |   | w |
---------------------------------
|   |   |   |   |   |   |   |   |
---------------------------------
|   |   |   |   |   |   |   |   |
---------------------------------
| b |   | b |   | b |   | b |   |
---------------------------------
|   | b |   | b |   | b |   | b |
---------------------------------
| b |   | b |   | b |   | b |   |
---------------------------------
"""

def foo(squares):
    ret = 0
    for s in squares:
        ret |= (1 << s)
    return hex(ret)

def generate_from_board():
    lines = BOARD.split('\n')[1:-1]

    board = []
    for i, line in enumerate(lines):
        if i % 2 == 0: # skip "----" lines
            continue
        cells = line.replace(' ', '').split('|')[1:-1]
        if i not in [1,5,9,13]:
            cells.pop(7)
            cells.pop(5)
            cells.pop(3)            
            cells.pop(1)
        else:
            cells.pop(6)
            cells.pop(4)            
            cells.pop(2)
            cells.pop(0)
        #board.extend(cells)
        cells[len(cells):] = board
        board = cells

    white = 0 # w|W = white
    black = 0 # b|B = black
    
    for i, cell in enumerate(board):
        if cell == 'w':
            white = white | (1 << i)
            print "W = ", i
        elif cell == 'b':
            black = black | (1 << i)
            print "B = ", i

    print white
    print black


# ---------------------------------
# |   | 1 |   | 3 |   | 5 |   | 7 |
# ---------------------------------
# | 8 |   |10 |   |12 |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------
# |   |   |   |   |   |   |   |   |
# ---------------------------------

LEFT = [8, 24, 40, 56]
RIGHT = [7, 23, 39, 55]
TOP = [1, 3, 5, 7]
BOTTOM = [56, 58, 60, 62]
def move_from_square(square):
    if ((1 << square) & 6172840429334713770) == 0:
        return '0', '0', '0'
    moves = []
    if square not in LEFT:
        if square + 7 < 64:
            moves.append(square + 7)
        if square - 9 > 0:
            moves.append(square - 9)
    if square not in RIGHT:
        if square + 9 < 64:
            moves.append(square + 9)
        if square - 7 > 0:
            moves.append(square - 7)
    ret = 0
    white = 0
    black = 0
    for move in moves:
        ret = ret | (1 << move)
        if move > square:
            black = black | (1 << move)
        elif move < square:
            white = white | (1 << move)
    return str(ret), str(white), str(black)

if __name__ == '__main__':
    generate_from_board()

    #print move_from_square(40)
    #sys.exit(0)

    # full = []
    # white = []
    # black = []
    # for i in range(64):
    #     ret = move_from_square(i)
    #     full.append(ret[0])
    #     white.append(ret[1])
    #     black.append(ret[2])
    #     #print ','.join(move_from_square(i))
    # print ','.join(full), '\n'
    # print ','.join(white), '\n'
    # print ','.join(black), '\n'

