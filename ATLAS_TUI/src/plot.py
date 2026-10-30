import matplotlib
import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Rectangle

matplotlib.use("webagg")
matplotlib.rcParams["webagg.port"] = 8988

fig, axis = plt.subplots()

amountOfMics = int(sys.argv[1])

x_array = []
y_array = []
micName_array = []

for mic in range(amountOfMics):
    x_array.append(float(sys.argv[2 + mic * 2]))
    y_array.append(float(sys.argv[3 + mic * 2]))
    micName_array.append(f"mic {mic}")

axis.plot(x_array,y_array,'ro')

for x, y, name in zip(x_array, y_array, micName_array):
    axis.annotate(name, (x, y), textcoords="offset points", xytext=(5, 5), fontsize=10)

axis.set_ylabel("Meter")
axis.set_xlabel("Meter")
axis.set_xlim(0, 5)
axis.set_ylim(0, 5)
plt.show()
