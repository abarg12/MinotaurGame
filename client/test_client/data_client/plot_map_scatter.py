import matplotlib.pyplot as plt
import numpy

### load the data for the map intervals in nanoseconds
data1 = numpy.loadtxt("3clients.txt")
data2 = numpy.loadtxt("13clients.txt")
data3 = numpy.loadtxt("103clients.txt")
data4 = numpy.loadtxt("1003clients.txt")

### get the data in milliseconds
milli_data1 = [item / 1000000 for item in data1]
milli_data2 = [item / 1000000 for item in data2]
milli_data3 = [item / 1000000 for item in data3]
milli_data4 = [item / 1000000 for item in data4]

plt.scatter(range(len(milli_data1)), milli_data1, s=2, label="3 clients")
plt.scatter(range(len(milli_data2)), milli_data2, s=2, label="13 clients")
plt.scatter(range(len(milli_data3)), milli_data3, s=2, label="103 clients")
plt.scatter(range(len(milli_data4)), milli_data4, s=2, label="1003 clients")
plt.legend(bbox_to_anchor=(1,1), shadow=True, ncol=1, markerscale=2)
plt.xlabel("Server tick")
plt.ylabel("Map message interval (ms)")
plt.title("Interval between map receipt with varying client load")
plt.savefig("freq_data_4.png", dpi=1200, bbox_inches="tight")
