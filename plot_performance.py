import matplotlib.pyplot as plt

labels = ["1x1", "1x2", "1x4", "2x1", "2x2", "4x1", "4x2", "4x4"]

times = [
    8.94992,
    8.55845,
    8.56258,
    6.66266,
    6.32864,
    6.1689,
    4.94433,
    4.93027
]

plt.figure()

plt.plot(labels, times, marker='o')

plt.title("Execution Time vs MPI x OpenMP Configuration")
plt.xlabel("Configuration (MPI x OpenMP)")
plt.ylabel("Time (seconds)")

plt.grid()


plt.savefig("performance_plot.png")