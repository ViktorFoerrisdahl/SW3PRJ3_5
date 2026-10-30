import sys
import matplotlib.pyplot as plt
 
x = float(sys.argv[1])
y = float(sys.argv[2])
 
plt.plot(x, y, "rx")
plt.xlim(0, 5)
plt.ylim(0, 5)
plt.show()
