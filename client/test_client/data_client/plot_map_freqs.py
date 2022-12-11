import matplotlib.pyplot as plt
import numpy

### load the data for the map intervals in nanoseconds
data = numpy.loadtxt("logfile.txt")

### get the data in milliseconds
milli_data = [item / 1000000 for item in data]

plt.scatter(range(len(milli_data)), milli_data)
plt.savefig("freq_data.png")
