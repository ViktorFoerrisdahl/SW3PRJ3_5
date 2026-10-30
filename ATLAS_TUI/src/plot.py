import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle

matplotlib.use("webagg")
matplotlib.rcParams["webagg.port"] = 8988

fig, axis = plt.subplots()

eventAreaRectangle = Rectangle((1, 1), 3, 3, fill=False, edgecolor="blue")
axis.add_patch(eventAreaRectangle)

axis.set_ylabel("Meter")
axis.set_xlabel("Meter")
axis.set_xlim(0, 5)
axis.set_ylim(0, 5)
plt.show()
