# python imports
import os
from tqdm import tqdm

# torch imports
import torch
import torch.nn as nn
import torch.optim as optim

# helper functions for computer vision
import torchvision
import torchvision.transforms as transforms


class LeNet(nn.Module):
    def __init__(self, input_shape=(32, 32), num_classes=100):
        super(LeNet, self).__init__()
         # 1. First convolutional layer with 6 output channels, kernel size 5, stride 1, followed by relu and max pooling
        self.conv1 = nn.Conv2d(in_channels=3, out_channels=6, kernel_size=5, stride=1)
        self.relu1 = nn.ReLU()
        self.pool1 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        # 2. Second convolutional layer with 16 output channels, kernel size 5, stride 1, followed by relu and max pooling
        self.conv2 = nn.Conv2d(in_channels=6, out_channels=16, kernel_size=5, stride=1)
        self.relu2 = nn.ReLU()
        self.pool2 = nn.MaxPool2d(kernel_size=2, stride=2)
        
        # 3. Flatten layer to convert 3D tensor to 1D tensor
        self.flatten = nn.Flatten()
        
        # 4. First fully connected layer with 256 output dimensions, followed by relu activation
        self.fc1 = nn.Linear(in_features=16 * 5 * 5, out_features=256)
        self.relu3 = nn.ReLU()
        
        # 5. Second fully connected layer with 128 output dimensions, followed by relu activation
        self.fc2 = nn.Linear(in_features=256, out_features=128)
        self.relu4 = nn.ReLU()
        
        # 6. Output layer with num_classes output dimensions (in this case, 100)
        self.fc3 = nn.Linear(in_features=128, out_features=num_classes)

        

        # certain definitions

    def forward(self, x):
        shape_dict = {}
        
        x = self.pool1(self.relu1(self.conv1(x)))
        shape_dict[1] = list(x.shape[:])  # Save the shape after the first convolutional layer
        
        x = self.pool2(self.relu2(self.conv2(x)))
        shape_dict[2] = list(x.shape[:])  # Save the shape after the second convolutional layer
        
        x = self.flatten(x)
        shape_dict[3] = list(x.shape)  # Save the shape after flattening
        
        x = self.relu3(self.fc1(x))
        shape_dict[4] = list(x.shape[:])  # Save the shape after the first fully connected layer
        
        x = self.relu4(self.fc2(x))
        shape_dict[5] = list(x.shape[:])  # Save the shape after the second fully connected layer
        
        out = self.fc3(x)  # Output layer
        
        shape_dict[6] = list(out.shape[:])  # Save the shape of the final output
        
        return out, shape_dict


def count_model_params():
    '''
    return the number of trainable parameters of LeNet.
    '''
    model = LeNet()
    model_params = sum(p.numel() for p in model.parameters() if p.requires_grad)
    model_params = model_params / 1e6


    return model_params


def train_model(model, train_loader, optimizer, criterion, epoch):
    """
    model (torch.nn.module): The model created to train
    train_loader (pytorch data loader): Training data loader
    optimizer (optimizer.*): A instance of some sort of optimizer, usually SGD
    criterion (nn.CrossEntropyLoss) : Loss function used to train the network
    epoch (int): Current epoch number
    """
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    
    model.train()
    train_loss = 0.0
    
    # print(f"Using {device}")
    for input, target in tqdm(train_loader, total=len(train_loader)):
        input, target = input.to(device), target.to(device)
        ###################################
        # fill in the standard training loop of forward pass,
        # backward pass, loss computation and optimizer step
        ###################################

        # 1) zero the parameter gradients
        optimizer.zero_grad()
        # 2) forward + backward + optimize
        output, _ = model(input)
        # print(_)
        loss = criterion(output, target)
        loss.backward()
        optimizer.step()

        # Update the train_loss variable
        # .item() detaches the node from the computational graph
        # Uncomment the below line after you fill block 1 and 2
        train_loss += loss.item()

    train_loss /= len(train_loader)
    print('[Training set] Epoch: {:d}, Average loss: {:.4f}'.format(epoch+1, train_loss))

    return train_loss


def test_model(model, test_loader, epoch):
    model.eval()
    correct = 0
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    with torch.no_grad():
        for input, target in test_loader:
            input, target = input.to(device), target.to(device)
            output, _ = model(input)
            pred = output.max(1, keepdim=True)[1]
            correct += pred.eq(target.view_as(pred)).sum().item()

    test_acc = correct / len(test_loader.dataset)
    print('[Test set] Epoch: {:d}, Accuracy: {:.2f}%\n'.format(
        epoch+1, 100. * test_acc))

    return test_acc


