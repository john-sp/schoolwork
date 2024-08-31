from scipy.linalg import eigh
import numpy as np
import matplotlib.pyplot as plt

def load_and_center_dataset(filename):
    x = np.load(filename)
    x = x - np.mean(x, axis=0)
    return x

def get_covariance(dataset):
    covariance_matrix = np.cov(dataset, rowvar=False)
    return covariance_matrix

def get_eig(S, m):
    eigenvalues, eigenvectors = eigh(S, subset_by_index=(S.shape[0]-m, S.shape[0]-1))
    # Sort eigenvalues and eigenvectors in descending order
    eigenvalues = eigenvalues[::-1]
    eigenvectors = eigenvectors[:, ::-1]
    # Return top m eigenvalues and corresponding eigenvectors
    return np.diag(eigenvalues), eigenvectors

def get_eig_prop(S, prop):
    # total_variance = np.trace(S)
    eigenvalues, eigenvectors = eigh(S, subset_by_value=(0,np.inf))
    eigenvalues = eigenvalues[::-1]
    eigenvectors = eigenvectors[:, ::-1]
    totSum = eigenvalues.sum(axis=0)
    # print(eigenvalues.sum(axis=0))
    proportion = eigenvalues/totSum
    intsum= 0.0
    for i in range(len(proportion)):
        
        intsum += proportion[i]
        if (intsum >= prop*10):
    # Sort eigenvalues and eigenvectors in descending order
            # print(f"{intsum}, {proportion[:i+1]}")
            return np.diag(eigenvalues[:i+1]), eigenvectors[:, :i+1]
    raise SyntaxError("This should never happen")



def project_image(image, U):
    aij = np.dot(U.T, image)
    out = np.zeros(len(U))
    # print(f"aij: {aij}")
    for i in range(0, len(U)):
        for j in range(0,len(aij)):
            out[i] += aij[j] * U[i][j]
    return out
def display_image(orig, proj):
    orig = np.rot90(orig.reshape(32,32), k=-1)
    proj = np.rot90(proj.reshape(32,32), k=-1)
    fig, (ax1, ax2) = plt.subplots(figsize=(9,3), ncols=2)
    ax1.set_title('Original Image')
    ax2.set_title('Projected Image')

    tempa = ax1.imshow(orig, aspect='equal')
    tempb = ax2.imshow(proj, aspect='equal')

    fig.colorbar(tempa)
    fig.colorbar(tempb)

    return fig, ax1, ax2

# Tester function:

if __name__ == "__main__":
    x = load_and_center_dataset('YaleB_32x32.npy')
    cov_matrix = get_covariance(x)
    Lambda1, eigenvectors1 = get_eig(cov_matrix, 2)
    # print(Lambda1)
    # print(eigenvectors1)
    Lambda, eigenvectors = get_eig_prop(cov_matrix, 0.07)
    # print(Lambda)
    # print(eigenvectors)
    projected_image = project_image(x[0], eigenvectors)
    # print(len(eigenvectors))
    # print(projected_image)
    fig, ax1, ax2 = display_image(x[0], projected_image)
    fig.show()
    input()
