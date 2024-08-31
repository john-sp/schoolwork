import numpy as np
import matplotlib.pyplot as plt
import csv, sys

# Example data from toy.csv

# year,days
# 1800,120
# 1801,155
# 1802,99

def question1(filepath):
    fig = plt.figure()
    
    file = open(filepath, 'r')
    reader = csv.reader(file)
    days = []
    year = []
    
    daysInt = 0
    yearInt = 1
    for row in reader:
        if (row[1] == "days"):
            daysInt = 1
            yearInt = 0
            continue
        days.append(int(row[daysInt]))
        year.append(row[yearInt])
    # Note days is not sorted, so plot will not have y axis in order right now
    plt.plot(year, days)

    plt.ylabel("Number of Days frozen")
    plt.xlabel("Year")

    return fig, year, days


def q3a(year):
    years = np.zeros((len(year), 2), dtype=np.int64)
    for i in range(len(year)):
        years[i, 0] = 1.0
        years[i, 1] = int(year[i])
    return years


def main():
    if len(sys.argv) == 2:
        arg1 = sys.argv[1]
    fig, year, days = question1(arg1)
    plt.savefig("plot.jpg")
    # plt.show()
    X = q3a(year)
    print("Q3a:")
    print(X)
    Y = np.array(days,dtype=np.int64)
    print("Q3b:")
    print(Y)
    Z = np.dot(X.T, X)
    print("Q3c:")
    print(Z)
    I = np.linalg.inv(Z)
    print("Q3d:")
    print(I)
    PI = np.dot(I, X.T)
    print("Q3e:")
    print(PI)
    fancyB_hat = np.dot(PI, Y)
    print("Q3f:")
    print(fancyB_hat)
    
    y_hat_test = np.add(fancyB_hat[0], np.multiply(fancyB_hat[1], 2022)) # not accurate to enough digits
    print("Q4:", y_hat_test)
    
    print("Q5a:", "=" if fancyB_hat[0] == 0 else "<" if 0 < fancyB_hat[0] else ">") # sorry for this inline monstrosity

    print("Q5b: If B_hat is zero, then this implies that the number of days frozen does not change with the year. If <, then it means that the number of days frozen decreases as the year increases. If >, then the number of days with ice is incresing as years increase") # TODO: ANSWER THIS QUESTION, what does it mean if fancyB_hat is 0, negative and positive

    x_star = - (fancyB_hat[0]/fancyB_hat[1])
    print("Q6a:", x_star) # 1812.8730158716883
    
    print("Q6b: After looking at the graph, there is a clear negative slope to the data, so it makes sense that 2463 there could be no snow") # TODO: ANSWER THIS QUESTION, Discuss whether this x_star makes sense given what we see in the data trends.


if __name__ == '__main__':
     main()
