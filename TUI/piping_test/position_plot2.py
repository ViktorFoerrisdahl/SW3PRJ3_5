import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle

x = float(sys.argv[1])
y = float(sys.argv[2])

fig, ax = plt.subplots()

# 3x3 firkant centreret i (2.5, 2.5), altså fra (1,1) til (4,4)
mic_rectangle = Rectangle((1, 1), 3, 3, fill=False, edgecolor="blue")
ax.add_patch(mic_rectangle)

# mic text:
# mic_words = ["mic 1", "mic 2", "mic 3", "mic 4"]
# mic_coordinates = np.array([[1, 1], [1, 3], [3, 1], [3, 3]])

# for word


ax.plot(x, y, "rx")
ax.set_xlim(0, 5)
ax.set_ylim(0, 5)
ax.set_ylabel("Meter")
ax.set_xlabel("Meter")
plt.show()