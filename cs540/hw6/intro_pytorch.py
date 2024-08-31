import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torchvision import datasets, transforms



def get_data_loader(training = True):
    """
    INPUT: 
        An optional boolean argument (default value is True for training dataset)

    RETURNS:
        Dataloader for the training set (if training = True) or the test set (if training = False)
    """
    transform=transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.1307,), (0.3081,))
        ])
    set = datasets.FashionMNIST('./data', train=training,download=True,transform=transform)
    return torch.utils.data.DataLoader(set, batch_size=64, shuffle=training)


def build_model():
    """
    INPUT: 
        None

    RETURNS:
        An untrained neural network model
    """
    # Layers in order
    # 1. A flatten layer to convert the 2d pixel array to a 1D array (just reformat the data)
    # 2. A dense Layer with 128 nodes and a ReLU Activation
    # 3. A dense layer with 64 nodes and a ReLU Activation
    # 4. A dense layer with 10 nodes

    return nn.Sequential(
        nn.Flatten(),
        nn.Linear(28*28, 128),
        nn.ReLU(),
        nn.Linear(128,64),
        nn.ReLU(),
        nn.Linear(64, 10)
    )

def train_model(model, train_loader, criterion, T):
    """
    INPUT: 
        model - the model produced by the previous function
        train_loader  - the train DataLoader produced by the first function
        criterion   - cross-entropy 
        T - number of epochs for training

    RETURNS:
        None
    """
    opt = optim.SGD(model.parameters(), lr=0.001, momentum=0.9)
    # Print training status after each epoch
    for epoch in range(T):  # loop over the dataset multiple times
        running_loss = 0.0
        correct = 0
        for i, data in enumerate(train_loader, 0):
            # get the inputs; data is a list of [inputs, labels]
            inputs, labels = data

            # zero the parameter gradients
            opt.zero_grad()

            # forward + backward + optimize
            outputs = model(inputs)
            _, predicted = torch.max(outputs.data, 1)  # Get the predicted class

            loss = criterion(outputs, labels)
            loss.backward()
            opt.step()
            
            # print statistics
            running_loss += loss.item()
            correct += (predicted == labels).sum().item()  # Increment count of correct predictions

        print(f"Train Epoch: {epoch} Accuracy: {correct}/60000({(correct/600):0.2f}%) Loss: {running_loss/len(train_loader):0.3f}")
                
def evaluate_model(model, test_loader, criterion, show_loss = True):
    """
    INPUT: 
        model - the the trained model produced by the previous function
        test_loader    - the test DataLoader
        criterion   - cropy-entropy 

    RETURNS:
        None
    """
    model.eval()  # Set the model to evaluation mode
    correct = 0  # Count of correct predictions
    total_loss = 0  # Total loss for the test set

    with torch.no_grad():  # Disable gradient computation
        for images, labels in test_loader:  # Iterate over the test dataset
            outputs = model(images)  # Make predictions
            _, predicted = torch.max(outputs.data, 1)  # Get the predicted class
            loss = criterion(outputs, labels)  # Calculate loss

            total_loss += loss.item()  # Accumulate loss
            correct += (predicted == labels).sum().item()  # Count correct predictions

    accuracy = correct / len(test_loader.dataset) * 100  # Calculate accuracy
    avg_loss = total_loss / len(test_loader)  # Calculate average loss
    if show_loss:
        print(f"Average loss: {avg_loss:.4f}")
    print(f"Accuracy: {accuracy:.2f}%")
    


def predict_label(model, test_images, index):
    """
    TODO: implement this function.

    INPUT: 
        model - the trained model
        test_images   -  a tensor. test image set of shape Nx1x28x28
        index   -  specific index  i of the image to be tested: 0 <= i <= N - 1


    RETURNS:
        None
    """

    
    # Make a prediction
    output = model(test_images[index])
    probabilities = F.softmax(output, dim=1)
    probs, predicted = probabilities.topk(3)

    # Print the predicted class and the probability of it

    for i in range(3):
        print(f"{get_name(predicted[0][i])}: {probs[0][i]*100:.2f}%")
    


def get_name(index):
    class_names = ['T-shirt/top','Trouser','Pullover','Dress','Coat','Sandal','Shirt','Sneaker','Bag','Ankle Boot']
    return class_names[index]


if __name__ == '__main__':
    '''
    Feel free to write your own test code here to exaime the correctness of your functions. 
    Note that this part will not be graded.
    '''
    train_loader = get_data_loader(True)
    criterion = nn.CrossEntropyLoss()
    model = build_model()
    train_model(model,train_loader,criterion,5)
    test_loader = get_data_loader(False)
    
    evaluate_model(model,test_loader,criterion)
    test_images, labels = next(iter(test_loader))
    predict_label(model, test_images, 1)
