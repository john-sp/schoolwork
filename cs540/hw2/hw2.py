import sys
import math


def get_parameter_vectors():
    '''
    This function parses e.txt and s.txt to get the  26-dimensional multinomial
    parameter vector (characters probabilities of English and Spanish) as
    descibed in section 1.2 of the writeup

    Returns: tuple of vectors e and s
    '''
    #Implementing vectors e,s as lists (arrays) of length 26
    #with p[0] being the probability of 'A' and so on
    e=[0]*26
    s=[0]*26

    with open('e.txt',encoding='utf-8') as f:
        for line in f:
            #strip: removes the newline character
            #split: split the string on space character
            char,prob=line.strip().split(" ")
            #ord('E') gives the ASCII (integer) value of character 'E'
            #we then subtract it from 'A' to give array index
            #This way 'A' gets index 0 and 'Z' gets index 25.
            e[ord(char)-ord('A')]=float(prob)
    f.close()

    with open('s.txt',encoding='utf-8') as f:
        for line in f:
            char,prob=line.strip().split(" ")
            s[ord(char)-ord('A')]=float(prob)
    f.close()

    return (e,s)

def shred(filename):
    #Using a dictionary here. You may change this to any data structure of
    #your choice such as lists (X=[]) etc. for the assignment
    letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    X=dict()

    for letter in letters:
        X[letter] = 0

    with open (filename, encoding='utf-8') as file:
        for line in file:
            for char in line:
                #print(char)
                if char.isalpha:
                  char = char.upper()
                  #Verify char is in letters before adding to X
                  if char in letters:
                      X[char] += 1
    file.close()


    return X


def compute_F(parameter_vector, letter_count):
    F = 0.0
    
    for letter, count in letter_count.items():
        F += count * math.log(parameter_vector[ord(letter) - ord('A')]) # get position inside provided vector and take log of it
        # print(f"{letter}: {count} {F:0.4f}")
    return F

def main():
    # Get the parameter vectors
    e, s = get_parameter_vectors()
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"



    # Get the letter count list
    letter_count = shred("letter.txt")
    print("Q1")
    for letter in alphabet:
        print(f"{letter} {letter_count[letter]}")

    print("Q2")

    e1 = e[0]
    s1 = s[0]
        
    X1 = letter_count["A"]
    # Calculate X1 log e1 and X1 log s1
    x1_log_e1 = X1 * math.log(e1) 
    x1_log_s1 = X1 * math.log(s1) 
    
    # Print the values up to 4 decimal places
    print(f"{x1_log_e1:.4f}")
    print(f"{x1_log_s1:.4f}")

    print("Q3")
    
    F_english = compute_F(e, letter_count) - math.log(1.66666666666) #magic number offsets

    # Print the values up to 4 decimal places
    print(f"{F_english:.4f}")

    F_spanish = compute_F(s, letter_count) - math.log(2.5) #magic number offsets
    print(f"{F_spanish:.4f}")

    print("Q4")
    if F_spanish - F_english >= 100:
        print("0.0000")
    elif F_spanish - F_english <= -100:
        print("1.0000")
    else:
        probability = 1 / (1 + math.exp(F_spanish - F_english))
        print(f"{probability:.4f}")



if __name__ == "__main__":
    main()