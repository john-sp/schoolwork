import gym
import random
import numpy as np
import time
from collections import deque
import pickle
from collections import defaultdict

EPISODES =  20000
LEARNING_RATE = .8
DISCOUNT_FACTOR = .9
EPSILON = 1
EPSILON_DECAY = 1/(EPISODES)


def default_Q_value():
    return 0

if __name__ == "__main__":

    random.seed(1)
    np.random.seed(1)
    env = gym.envs.make("FrozenLake-v1")
    env.seed(1)
    env.action_space.np_random.seed(1)

    # You will need to update the Q_table in your iteration
    Q_table = defaultdict(default_Q_value) # starts with a pessimistic estimate of zero reward for each state.
    episode_reward_record = deque(maxlen=100)

    for i in range(EPISODES):
        episode_reward = 0
        done = False
        obs = env.reset()

        ##########################################################
        # YOU DO NOT NEED TO CHANGE ANYTHING ABOVE THIS LINE
        # TODO: Replace the following with Q-Learning
        while not done:
            # Choose action using epsilon-greedy strategy
            if np.random.rand() < EPSILON:
                action = env.action_space.sample()  # Explore: choose a random action
            else:
                # Exploit: choose the action with the highest Q-value for the current state
                action = max(range(env.action_space.n), key=lambda a: Q_table[(obs, a)])

            # Take the chosen action and observe the next state and reward
            next_obs, reward, done, info = env.step(action)

            # Q-learning update rule
            best_next_action = max(range(env.action_space.n), key=lambda a: Q_table[(next_obs, a)])
            current_key = (obs, action)
            next_key = (next_obs, best_next_action)

            current_q = Q_table[current_key]
            new_q = current_q + LEARNING_RATE * (reward + DISCOUNT_FACTOR * Q_table[next_key] - current_q)
            Q_table[current_key] = new_q

            obs = next_obs  # Move to the next state
            episode_reward += reward  # Update episode reward
        EPSILON = max(EPSILON - EPSILON_DECAY, 0)

        # END of TODO
        # YOU DO NOT NEED TO CHANGE ANYTHING BELOW THIS LINE
        ##########################################################

        # record the reward for this episode
        episode_reward_record.append(episode_reward) 
     
        if i % 1000 == 0 and i > 0:
            print("LAST 1000 EPISODE AVERAGE REWARD: " + str(sum(list(episode_reward_record))/1000))
            print("EPSILON: " + str(EPSILON) )
    
    
    #### DO NOT MODIFY ######
    model_file = open('Q_TABLE.pkl' ,'wb')
    pickle.dump([Q_table,EPSILON],model_file)
    model_file.close()
    #########################