import numpy as np
import scipy
import matplotlib.pyplot as plt
import csv, time


# Sources used:
# https://en.wikipedia.org/wiki/Complete-linkage_clustering
# https://github.com/scipy/scipy/blob/main/scipy/cluster/_hierarchy.pyx#L675 <- Scipy source code

def load_data(filepath):
    file = open(filepath, 'r')
    reader = csv.DictReader(file)
    return list(reader)

def calc_features(row):
    toReturn = np.zeros(shape=(6,), dtype= np.float64)
    toReturn[0] = row["Population"]
    toReturn[1] = row["Net migration"]
    toReturn[2] = row["GDP ($ per capita)"]
    toReturn[3] = row["Literacy (%)"]
    toReturn[4] = row["Phones (per 1000)"]
    toReturn[5] = row["Infant mortality (per 1000 births)"]
    return toReturn

# This is worth 50 points...
def hac(features):
    # Convert input features to a numpy array
    features = np.array(features)
    n = len(features)
    
    # Initialize distance and cluster matrices
    distance_matrix = np.zeros(shape=(n, n), dtype=np.float64)
    cluster_matrix = np.zeros(shape=(n, 2), dtype=np.intc)
    min_i = 0
    min_j = 0

    # Calculate pairwise distances between features
    for i in range(n):
        cluster_matrix[i, 0] = i
        cluster_matrix[i, 1] = 1
        for j in range(i+1,n):
            distance_matrix[i, j] = np.linalg.norm(features[i] - features[j])
            distance_matrix[j, i] = distance_matrix[i, j]
    
    # Initialize linkage matrix
    Z = np.zeros(shape=(n-1, 4), dtype=np.float64)

    # Z[i, 0] and Z[i, 1] are indicies of two clusters merged on the ith iteration of clustering algo
    # Z[i, 2] is complete linkage distance between Z[i, 0] and Z[i, 1]
    # Z[i, 3] is size of new cluster formed by the merge

    # Goal is to make a dendrogram
    # Z[i, 0] and Z[i, 1] are indicies of two clusters merged on the ith iteration of clustering algo
    # Z[i, 2] is complete linkage distance between Z[i, 0] and Z[i, 1]
    # Z[i, 3] is size of new cluster formed by the merge
   
    # Perform hierarchical agglomerative clustering
    for k in range(0, (n-1)):
        curr_min = np.inf

        # Find the minimum distance in the distance matrix
        for i in range(distance_matrix.shape[0]):
            for j in range(distance_matrix.shape[1]):
                if (i != j and (distance_matrix[i, j] < curr_min)):
                    min_i = i
                    min_j = j
                    curr_min = distance_matrix[i, j]
                    
        # Update linkage matrix
        Z[k, 0] = min(cluster_matrix[min_i, 0], cluster_matrix[min_j, 0])
        Z[k, 1] = max(cluster_matrix[min_i, 0], cluster_matrix[min_j, 0])
        cluster_matrix[min_i, 1] = cluster_matrix[min_i, 1] + cluster_matrix[min_j, 1]
        cluster_matrix[min_i, 0] = (n) + k
        cluster_matrix = np.delete(cluster_matrix, min_j, 0)
        Z[k, 2] = curr_min
        Z[k, 3] = cluster_matrix[min_i, 1]
        
        # Update distance matrix
        for m in range(distance_matrix.shape[0]):
            if (m == min_i):
                distance_matrix[m, min_i] = 0
            elif (m != min_j):
                distance_matrix[m, min_i] = np.maximum(distance_matrix[min_i, m], distance_matrix[min_j, m])
                distance_matrix[min_i, m] = distance_matrix[m, min_i]
        distance_matrix = np.delete(distance_matrix, min_j, 0)
        distance_matrix = np.delete(distance_matrix, min_j, 1)
        
        # Update linkage matrix with cluster distance and size
    
    # Return the linkage matrix
    return Z


def fig_hac(Z, names):
    fig = plt.figure()
    fig.tight_layout()
    scipy.cluster.hierarchy.dendrogram(Z, labels=names, leaf_rotation = 90)
    return fig

# For each value, x is orig, u is mean and sd is sd for column. x_new = (x-u)/sd
# features [np(6,1), np(6,1)]
def normalize_features(features):
    features = np.array(features)
    mean = np.mean(features, axis=0)
    sd = np.std(features, axis=0)
    normalized_features = (features - mean) / sd
    return normalized_features.tolist()


if __name__ == "__main__":
    start = time.time()
    data = load_data("countries.csv")
    country_names = [row["Country"] for row in data]
    features = [calc_features(row) for row in data]
    features_normalized = normalize_features(features)
    n = 203
    Z_raw = hac(features[:n])
    Z_normalized = hac(features_normalized[:n])
    print(Z_normalized)
    fig = fig_hac(Z_normalized, country_names[:n])
    # fig.savefig("hac.png")
    end= time.time()
    print(end-start)
    plt.show()