import sys
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

x = float(sys.argv[1])
y = float(sys.argv[2])

fig, ax = plt.subplots()

# 3x3 firkant centreret i (2.5, 2.5), altså fra (1,1) til (4,4)
firkant = Rectangle((1, 1), 3, 3, fill=False, edgecolor="blue")
ax.add_patch(firkant)

ax.plot(x, y, "rx")
ax.set_xlim(0, 5)
ax.set_ylim(0, 5)
plt.show()