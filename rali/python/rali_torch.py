import torch
from  rali_common import *
import numpy as np

class PyTorchIterator:
    def __init__(self, pipeline, tensor_layout = TensorLayout.NCHW, reverse_channels = False, multiplier = [1.0,1.0,1.0], offset = [0.0, 0.0, 0.0], tensor_dtype=TensorDataType.FLOAT32):
        self.pipe = pipeline
        self.tensor_format =tensor_layout
        self.multiplier = multiplier
        self.offset = offset
        self.reverse_channels = reverse_channels
        self.tensor_dtype = tensor_dtype
        if pipeline.build() != 0:
            raise Exception('Failed to build the augmentation graph')
        self.w = pipeline.getOutputWidth()
        self.h = pipeline.getOutputHeight()
        self.b = pipeline.getBatchSize()
        self.n = pipeline.getOutputImageCount()
        color_format = pipeline.getOutputColorFormat()
        self.p = (1 if color_format is ColorFormat.IMAGE_U8 else 3)
        if self.tensor_dtype == TensorDataType.FLOAT32:
            self.out = np.zeros(( self.b*self.n, self.p, self.h/self.b, self.w,), dtype = "float32")
        elif self.tensor_dtype == TensorDataType.FLOAT16:
            self.out = np.zeros(( self.b*self.n, self.p, self.h/self.b, self.w,), dtype = "float16")
        labels = []
        for image in self.pipe.output_images:
            labels.append(image.get_labels())

        self.labels_tensor = torch.LongTensor(labels)
            
    def next(self):
        return self.__next__()

    def __next__(self):

        if self.pipe.getReaminingImageCount() <= 0:
            raise StopIteration

        if self.pipe.run() != 0:
            raise StopIteration

        if(TensorLayout.NCHW == self.tensor_format):
            self.pipe.copyToTensorNCHW(self.out, self.multiplier, self.offset, self.reverse_channels, int(self.tensor_dtype))
        else:
            self.pipe.copyToTensorNHWC(self.out, self.multiplier, self.offset, self.reverse_channels, int(self.tensor_dtype))

        if self.tensor_dtype == TensorDataType.FLOAT32:
            return torch.from_numpy(self.out), self.labels_tensor
        elif self.tensor_dtype == TensorDataType.FLOAT16:
            return torch.from_numpy(self.out.astype(np.float16)), self.labels_tensor

    def reset(self):
        self.pipe.reset()

    def __iter__(self):
        self.pipe.reset()
        return self

    def imageCount(self):
        return self.pipe.getReaminingImageCount()
