import matplotlib.pyplot as plt
import numpy as np

plt.rcParams["figure.figsize"] = [10,10]  # Set default figure size

# plt.scatter(range(len(milli_data)), milli_data)
# plt.savefig("freq_data.png")
# plt.show()

def add_plot(axis, data, title):
    milli_data = [item / 1000000 for item in data]
    axis.hist(milli_data, edgecolor="white")
    axis.set_ylabel("Number of occurrences")
    axis.set_xlabel("Map message interval (ms)")
    axis.set_title(title)

### load the data for the map intervals in nanoseconds
data_list = []
data_list.append(np.loadtxt("3clients.txt"))
data_list.append(np.loadtxt("13clients.txt"))
data_list.append(np.loadtxt("103clients.txt"))
data_list.append(np.loadtxt("1003clients.txt"))

fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(2,2)
plt.subplots_adjust(left=0.1,
                    bottom=0.1,
                    right=0.9,
                    top=0.9,
                    wspace=0.4,
                    hspace=0.4)
add_plot(ax0, data_list[0], "3 Clients")
add_plot(ax1, data_list[1], "13 Clients")
add_plot(ax2, data_list[2], "103 Clients")
add_plot(ax3, data_list[3], "1003 Clients")
fig.savefig("freq_data.png")
