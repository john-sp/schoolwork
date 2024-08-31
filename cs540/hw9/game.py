import random, copy
import numpy as np


class TeekoPlayer:
    """ An object representation for an AI game player for the game Teeko.
    """
    board = [[' ' for j in range(5)] for i in range(5)]
    pieces = ['b', 'r']

    def __init__(self):
        """ Initializes a TeekoPlayer object by randomly selecting red or black as its
        piece color.
        """
        self.my_piece = random.choice(self.pieces)
        self.opp = self.pieces[0] if self.my_piece == self.pieces[1] else self.pieces[1]


    def make_move(self, state):
        """ Selects a (row, col) space for the next move. You may assume that whenever
        this function is called, it is this player's turn to move.
        Args:
            state (list of lists): should be the current state of the game as saved in
                this TeekoPlayer object. Note that this is NOT assumed to be a copy of
                the game state and should NOT be modified within this method (use
                place_piece() instead). Any modifications (e.g. to generate successors)
                should be done on a deep copy of the state.

                In the "drop phase", the state will contain less than 8 elements which
                are not ' ' (a single space character).

        Return:
            move (list): a list of move tuples such that its format is
                    [(row, col), (source_row, source_col)]
                where the (row, col) tuple is the location to place a piece and the
                optional (source_row, source_col) tuple contains the location of the
                piece the AI plans to relocate (for moves after the drop phase). In
                the drop phase, this list should contain ONLY THE FIRST tuple.

        Note that without drop phase behavior, the AI will just keep placing new markers
            and will eventually take over the board. This is not a valid strategy and
            will earn you no points.
        """

        drop_phase = True  # TODO: detect drop phase

        numB = sum((i.count('b') for i in state))
        numR = sum((i.count('r') for i in state))
        if numB >= 4 and numR >= 4:
            drop_phase = False

        if not drop_phase:
            # TODO: choose a piece to move and remove it from the board
            # (You may move this condition anywhere, just be sure to handle it)
            #
            # Until this part is implemented and the move list is updated
            # accordingly, the AI will not follow the rules after the drop phase!
            move = []
            value, bstate = self.max_value(state, 0)
            # print("desired state: ")
            # self.print_board_debug(bstate)
            arr1 = np.array(state) == np.array(bstate)
            arr2 = np.where(arr1 == False) # check difference between succ and curr state
            if state[arr2[0][0]][arr2[1][0]] == ' ': # find original to define move
                (origrow, origcol) = (arr2[0][1],arr2[1][1])
                (row,col) = (arr2[0][0], arr2[1][0])
            else:
                (origrow, origcol) = (arr2[0][0], arr2[1][0])
                (row, col) = (arr2[0][1], arr2[1][1])
            move.insert(0, (int(row), col))
            move.insert(1, (int(origrow), origcol))  # move for after drop phase
            # print("Value: ",value)
            return move

        # select an unoccupied space randomly
        move = []
        value, bstate = self.max_value(state,0)
        arr1 = np.array(state) == np.array(bstate)
        arr2 = np.where(arr1 == False) # check difference between succ and curr state
        (row,col) = (arr2[0][0], arr2[1][0])
        while not state[row][col] == ' ': # find original to define move
            (row, col) = (arr2[0][0], arr2[1][0])

        
        move.insert(0, (int(row),int(col)))  # move for drop phase
        
        # print("Drop Value: ",value)
        return move
   
    

    def opponent_move(self, move):
        """ Validates the opponent's next move against the internal board representation.
        You don't need to touch this code.

        Args:
            move (list): a list of move tuples such that its format is
                    [(row, col), (source_row, source_col)]
                where the (row, col) tuple is the location to place a piece and the
                optional (source_row, source_col) tuple contains the location of the
                piece the AI plans to relocate (for moves after the drop phase). In
                the drop phase, this list should contain ONLY THE FIRST tuple.
        """
        # validate input
        if len(move) > 1:
            source_row = move[1][0]
            source_col = move[1][1]
            if source_row != None and self.board[source_row][source_col] != self.opp:
                self.print_board()
                print(move)
                raise Exception("You don't have a piece there!")
            if abs(source_row - move[0][0]) > 1 or abs(source_col - move[0][1]) > 1:
                self.print_board()
                print(move)
                raise Exception('Illegal move: Can only move to an adjacent space')
        if self.board[move[0][0]][move[0][1]] != ' ':
            raise Exception("Illegal move detected")
        # make move
        self.place_piece(move, self.opp)

    def place_piece(self, move, piece):
        """ Modifies the board representation using the specified move and piece

        Args:
            move (list): a list of move tuples such that its format is
                    [(row, col), (source_row, source_col)]
                where the (row, col) tuple is the location to place a piece and the
                optional (source_row, source_col) tuple contains the location of the
                piece the AI plans to relocate (for moves after the drop phase). In
                the drop phase, this list should contain ONLY THE FIRST tuple.

                This argument is assumed to have been validated before this method
                is called.
            piece (str): the piece ('b' or 'r') to place on the board
        """
        if len(move) > 1:
            self.board[move[1][0]][move[1][1]] = ' '
        self.board[move[0][0]][move[0][1]] = piece

    def print_board(self):
        """ Formatted printing for the board """
        for row in range(len(self.board)):
            line = str(row)+": "
            for cell in self.board[row]:
                line += cell + " "
            print(line)
        print("   A B C D E")

    def print_board_debug(self, state):
        """ Formatted printing for the board """
        for row in range(len(state)):
            line = str(row)+": "
            for cell in state[row]:
                line += cell + " "
            print(line)
        print("   A B C D E")

    def game_value(self, state):
        """ Checks the current board status for a win condition

        Args:
        state (list of lists): either the current state of the game as saved in
            this TeekoPlayer object, or a generated successor state.

        Returns:
            int: 1 if this TeekoPlayer wins, -1 if the opponent wins, 0 if no winner

        """
        # check horizontal wins
        # print("current state ", state)
        for row in state:
            for i in range(2):
                # print(row)
                if row[i] != ' ' and row[i] == row[i+1] == row[i+2] == row[i+3]:
                    return 1 if row[i]==self.my_piece else -1

        # check vertical wins
        for col in range(5):
            for i in range(2):
                if state[i][col] != ' ' and state[i][col] == state[i+1][col] == state[i+2][col] == state[i+3][col]:
                    return 1 if state[i][col]==self.my_piece else -1
        # check \ diagonal wins
        for row in range(2):
            for i in range(2):
                if state[row][i] != ' ' and state[row][i] == state[row + 1][i + 1] == state[row + 2][i + 2] == \
                        state[row + 3][i + 3]:
                    return 1 if state[row][i] == self.my_piece else -1

        # check / diagonal wins
        for row in range(3, 5):
            for i in range(2):
                if state[row][i] != ' ' and state[row][i] == state[row - 1][i + 1] == state[row - 2][i + 2] == \
                        state[row - 3][i + 3]:
                    return 1 if state[row][i] == self.my_piece else -1
        # check box wins
        for i in range(0,4):
            for j in range(0,4):
                if state[i][j] != ' ' and state[i][j] == state[i][j+1] == state[i+1][j] == state[i+1][j+1]:
                    return 1 if state[i][j]==self.my_piece else -1

        return 0 # no winner yet

    def succ(self, state, piece):
        """ Generates a successor state for the specified state

        Args:
            state (list of lists): the current state of the game

        Returns:
            list of lists: a successor state of the game
        """
    
        drop_phase = sum(row.count('b') + row.count('r') for row in state) < 8
        if drop_phase:
            succ = []
            for row in range(len(self.board)):
                for col in range(len(self.board[row])):
                    if state[row][col] ==' ':
                        toapp = copy.deepcopy(state)
                        toapp[row][col] = piece
                        succ.append(toapp)
            # print("successors:", succ)
            return succ
                
        # TODO: Does not work! makes illegal moves
        succ = []
        for row in range(5):
            for col in range(5):
                if state[row][col] == piece:
                    for i in [-1, 0, 1]:
                        for j in [-1, 0, 1]:
                            if row+i >= 0 and row+i < 5 and col+j >= 0 and col+j < 5 and self.board[row+i][col+j] == ' ':
                                succ.append(self.move(state, [(row+i, col+j), (row, col)], piece))

        return succ

    def move(self, k, move, piece):
        state = copy.deepcopy(k)
        # test if new position is empty

        state[move[1][0]][move[1][1]] = ' '
        state[move[0][0]][move[0][1]] = piece
        return state
    
    def heuristic_game_value(self, state, piece):
        def count_connected(row, col, dr, dc):
            count = 0
            while 0 <= row < len(state) and 0 <= col < len(state):
                if state[row][col] == piece:
                    count += 1
                    row += dr
                    col += dc
                else:
                    break
            return count

        mine = piece
        oppo = 'r' if piece == 'b' else 'b'

        max_score = 0

        for row in range(len(state)):
            for col in range(len(state[row])):
                if state[row][col] == piece:
                    # Check horizontal, vertical, and diagonals
                    directions = [(0, 1), (1, 0), (1, 1), (1, -1)]
                    for dr, dc in directions:
                        count = 1 + count_connected(row + dr, col + dc, dr, dc)
                        count += count_connected(row - dr, col - dc, -dr, -dc)
                        max_score = max(max_score, count)

        # Check 2x2 patterns
        for row in range(len(state) - 1):
            for col in range(len(state[row]) - 1):
                count = sum(state[row + i][col + j] == piece for i in range(2) for j in range(2))
                max_score = max(max_score, count)

        return max_score / 6, state

 
    def max_value(self, state, depth, alpha = float('-Inf'), beta = float('Inf')):
        bstate = state
        if self.game_value(state) != 0:
            return self.game_value(state), state

        if depth >= 3:
            return self.heuristic_game_value(state, self.my_piece)

        a = float('-Inf')
        for s in self.succ(state, self.my_piece):
            val = self.min_value(s, depth + 1, alpha, beta)
            if val[0] > a:
                a = val[0]
                bstate = s
            if a >= beta:
                return a, bstate  # Prune the remaining branches

            alpha = max(alpha, a)

        return a, bstate

    def min_value(self, state, depth, alpha, beta):
        bstate = state
        if self.game_value(state) != 0:
            return self.game_value(state), state

        if depth >= 3:
            return self.heuristic_game_value(state, self.opp)

        b = float('Inf')
        for s in self.succ(state, self.opp):
            val = self.max_value(s, depth + 1, alpha, beta)
            if val[0] < b:
                b = val[0]
                bstate = s
            if b <= alpha:
                return b, bstate  # Prune the remaining branches

            beta = min(beta, b)

        return b, bstate



############################################################################
#
# THE FOLLOWING CODE IS FOR SAMPLE GAMEPLAY ONLY
#
############################################################################

def normal_main():
    print('Hello, this is Samaritan')
    ai = TeekoPlayer()
    piece_count = 0
    turn = 0

    # drop phase
    while piece_count < 8 and ai.game_value(ai.board) == 0:

        # get the player or AI's move
        if ai.my_piece == ai.pieces[turn]:
            ai.print_board()
            move = ai.make_move(ai.board)
            ai.place_piece(move, ai.my_piece)
            print(ai.my_piece+" moved at "+chr(move[0][1]+ord("A"))+str(move[0][0]))
        else:
            move_made = False
            ai.print_board()
            print(ai.opp+"'s turn")
            while not move_made:
                player_move = input("Move (e.g. B3): ")
                if(player_move == "succ"):
                    for i in ai.succ(ai.board, ai.opp):
                        ai.print_board_debug(i)
                while player_move[0] not in "ABCDE" or player_move[1] not in "01234":
                    player_move = input("Move (e.g. B3): ")
                    if(player_move == "succ"):
                        for i in ai.succ(ai.board, ai.opp):
                            ai.print_board_debug(i)
                try:
                    ai.opponent_move([(int(player_move[1]), ord(player_move[0])-ord("A"))])
                    move_made = True
                except Exception as e:
                    print(e)

        # update the game variables
        piece_count += 1
        turn += 1
        turn %= 2

    # move phase - can't have a winner until all 8 pieces are on the board
    while ai.game_value(ai.board) == 0:

        # get the player or AI's move
        if ai.my_piece == ai.pieces[turn]:
            ai.print_board()
            move = ai.make_move(ai.board)
            ai.place_piece(move, ai.my_piece)
            print(ai.my_piece+" moved from "+chr(move[1][1]+ord("A"))+str(move[1][0]))
            print("  to "+chr(move[0][1]+ord("A"))+str(move[0][0]))
        else:
            move_made = False
            ai.print_board()
            print(ai.opp+"'s turn")
            while not move_made:
                move_from = input("Move from (e.g. B3): ")
                while move_from[0] not in "ABCDE" or move_from[1] not in "01234":
                    move_from = input("Move from (e.g. B3): ")
                move_to = input("Move to (e.g. B3): ")
                while move_to[0] not in "ABCDE" or move_to[1] not in "01234":
                    move_to = input("Move to (e.g. B3): ")
                try:
                    ai.opponent_move([(int(move_to[1]), ord(move_to[0])-ord("A")),
                                    (int(move_from[1]), ord(move_from[0])-ord("A"))])
                    move_made = True
                except Exception as e:
                    print(e)

        # update the game variables
        turn += 1
        turn %= 2

    ai.print_board()
    if ai.game_value(ai.board) == 1:
        print("AI wins! Game over.")
    else:
        print("You win! Game over.")

def random_main():
    print('Hello, this is Samaritan')
    ai = TeekoPlayer()
    piece_count = 0
    turn = 0

    # drop phase
    while piece_count < 8 and ai.game_value(ai.board) == 0:
       
        # get the player or AI's move
        if ai.my_piece == ai.pieces[turn]:
            ai.print_board()
            move = ai.make_move(ai.board)
            ai.place_piece(move, ai.my_piece)
            print(ai.my_piece+" moved at "+chr(move[0][1]+ord("A"))+str(move[0][0]))
            
        else:
            move_made = False
            ai.print_board()
            print(ai.opp+"'s turn")
            while not move_made:
                poss = ai.succ(ai.board,ai.opp)
                player_move = random.choice(poss)
                arr1 = np.array(ai.board) == np.array(player_move)
                arr2 = np.where(arr1 == False) # check difference between succ and curr state
                (row,col) = (arr2[0][0], arr2[1][0])
                while not ai.board[row][col] == ' ': # find original to define move
                    (row, col) = (arr2[0][0], arr2[1][0])
                print(ai.opp+"'s move: "+chr(col+ord("A"))+str(row))
                
               
                try:
                    ai.opponent_move([(row, col)])
                    move_made = True
                except Exception as e:
                    print(e)
                    exit(1)

        # update the game variables
        piece_count += 1
        turn += 1
        turn %= 2

    # move phase - can't have a winner until all 8 pieces are on the board
    while ai.game_value(ai.board) == 0:
        
      

        # get the player or AI's move
        if ai.my_piece == ai.pieces[turn]:
            ai.print_board()
            move = ai.make_move(ai.board)
            ai.place_piece(move, ai.my_piece)
            print(ai.my_piece+" moved from "+chr(move[1][1]+ord("A"))+str(move[1][0]))
            print("  to "+chr(move[0][1]+ord("A"))+str(move[0][0]))
        else:
            move_made = False
            ai.print_board()
            print(ai.opp+"'s turn")
            while not move_made:
                poss = ai.succ(ai.board, ai.opp)
                
                player_move = random.choice(poss)
                arr1 = np.array(ai.board) == np.array(player_move)
                arr2 = np.where(arr1 == False) # check difference between succ and curr state
                if ai.board[arr2[0][0]][arr2[1][0]] == ' ': # find original to define move
                    (origrow, origcol) = (arr2[0][1],arr2[1][1])
                    (row,col) = (arr2[0][0], arr2[1][0])
                else:
                    (origrow, origcol) = (arr2[0][0], arr2[1][0])
                    (row, col) = (arr2[0][1], arr2[1][1])


               
                try:
                    ai.opponent_move([(origrow,origcol),
                                    (row,col)])
                    move_made = True
                except Exception as e:
                    print(e)

        # update the game variables
        turn += 1
        turn %= 2

    ai.print_board()
    if ai.game_value(ai.board) == 1:
        print("AI wins! Game over.")
    else:
        print("You win! Game over.")
def main():
    normal_main()
   

if __name__ == "__main__":
    main()
