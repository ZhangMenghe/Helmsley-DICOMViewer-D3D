import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
with open('.\\chess', 'rb') as f:
    data = f.read()
total_size = len(data)
size = total_size/21
images = []
for start in np.arange(0, total_size, size):
    images.append(data[int(start):int(start) + int(size)])
for img, index in zip(images, range(len(images))):
    plt.imsave(str(index) + '.png', np.frombuffer(img, dtype=np.uint8).reshape(480,640), cmap=cm.gray)