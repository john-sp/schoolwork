import heapq

import numpy as np



def get_manhattan_distance(from_state, to_state=[1, 2, 3, 4, 5, 6, 7, 0, 0]):

    """

    TODO: implement this function. This function will not be tested directly by the grader. 



    INPUT: 

        Two states (if second state is omitted then it is assumed that it is the goal state)



    RETURNS:

        A scalar that is the sum of Manhattan distances for all tiles.

    """

    distance = 0

    for i in range(1, 8):

        from_pos = from_state.index(i)

        to_pos = to_state.index(i)

        

        distance += abs(from_pos // 3 - to_pos // 3) + abs(from_pos % 3 - to_pos % 3)

    return distance



def print_succ(state):

    """

    TODO: This is based on get_succ function below, so should implement that function.



    INPUT: 

        A state (list of length 9)



    WHAT IT DOES:

        Prints the list of all the valid successors in the puzzle. 

    """

    succ_states = get_succ(state)



    for succ_state in succ_states:

        print(succ_state, "h={}".format(get_manhattan_distance(succ_state)))









def get_succ(state):

    """

    TODO: implement this function.



    INPUT: 

        A state (list of length 9)



    RETURNS:

        A list of all the valid successors in the puzzle (don't forget to sort the result as done below). 

    """

    succ_states = []

    empty_indices = [i for i, x in enumerate(state) if x == 0]



    moves = [(0, 1), (0, -1), (1, 0), (-1, 0)]



    for empty_index in empty_indices:

        for move in moves:

            new_row = empty_index // 3 + move[0]

            new_col = empty_index % 3 + move[1]

            new_index = new_row * 3 + new_col



            # Check if the new index is within the bounds of the puzzle

            if 0 <= new_row < 3 and 0 <= new_col < 3:

                new_state = state.copy()

                new_state[empty_index], new_state[new_index] = new_state[new_index], new_state[empty_index]

                succ_states.append(new_state)

    

    # Remove duplicates

    dedupe = []

    for elem in succ_states:

        if elem not in dedupe:

            dedupe.append(elem)

    succ_states = dedupe



    # Remove the original state from the list of successors

    if state in succ_states:

        succ_states.remove(state)



    return sorted(succ_states)



def solve(state, goal_state=[1, 2, 3, 4, 5, 6, 7, 0, 0]):

    """

    TODO: Implement the A* algorithm here.



    INPUT: 

        An initial state (list of length 9)



    WHAT IT SHOULD DO:

        Prints a path of configurations from initial state to goal state along  h values, number of moves, and max queue number in the format specified in the pdf.

    """

    state_info_list = []

    max_length = 0

    visited_states = set()

    predecessors = {}  # Dictionary to store predecessors

    g_values = {tuple(state): 0}  # Dictionary to store g values



    # Priority queue to store states with their heuristic values

    priority_queue = [(0, state, 0)]

    heapq.heapify(priority_queue)



    while priority_queue:

        current_h, current_state, moves = heapq.heappop(priority_queue)



        # Check if the current state is the goal state

        if current_state == goal_state:

            state_info_list.append((current_state, current_h, moves))

            break



        # Generate successors and add them to the priority queue if not visited

        succ_states = get_succ(current_state)

        for succ_state in succ_states:

            g_current = g_values[tuple(current_state)] + 1

            if tuple(succ_state) not in visited_states:

                visited_states.add(tuple(succ_state))

                predecessors[tuple(succ_state)] = (current_state, moves + 1)



                # Check if succ_state is not on OPEN or CLOSED

                if tuple(succ_state) not in g_values:

                    g_values[tuple(succ_state)] = g_current

                    succ_h = get_manhattan_distance(succ_state, goal_state)

                    heapq.heappush(priority_queue, (g_current + succ_h, succ_state, moves + 1))

                else:

                    # If succ_state is on OPEN or CLOSED, check if g value is lower

                    if g_current < g_values[tuple(succ_state)]:

                        # Redirect pointers backward from succ_state along the path yielding lower g

                        predecessors[tuple(succ_state)] = (current_state, moves + 1)

                        g_values[tuple(succ_state)] = g_current

                        heapq.heappush(priority_queue, (g_current + get_manhattan_distance(succ_state, goal_state), succ_state, moves + 1))

            else:

                # If succ_state is visited, check if g value is lower

                if g_current < g_values[tuple(succ_state)]:

                    # Redirect pointers backward from succ_state along the path yielding lower g

                    predecessors[tuple(succ_state)] = (current_state, moves + 1)

                    g_values[tuple(succ_state)] = g_current

                    heapq.heappush(priority_queue, (g_current + get_manhattan_distance(succ_state, goal_state), succ_state, moves + 1))

                    



        # Update max queue length within the loop

        max_length = max(max_length, len(priority_queue))



    # Print the shortest path

    current_state = goal_state

    path = []

    while current_state != state:

        path.append((current_state, predecessors[tuple(current_state)][1]))

        current_state = predecessors[tuple(current_state)][0]



    # Print the states in reverse order to get the correct path

    print(state, "h={}".format(get_manhattan_distance(state, goal_state)), "moves: 0")

    for state_info in reversed(path):

        state = state_info[0]

        h = get_manhattan_distance(state, goal_state)

        move = state_info[1]

        print(state, f"h={h} moves: {move}")



    # Update max queue length outside the loop

    print(f"Max queue length: {max_length}")

