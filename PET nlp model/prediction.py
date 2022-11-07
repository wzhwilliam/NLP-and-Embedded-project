import numpy as np
import torch
a=torch.Tensor([[1,1,-1,0],[-1,0,1,1]])
b=torch.Tensor([[0.2,0.3,0.4,0.5],[0.2,0.3,0.4,0.5]])
print(a[0].nonzero())