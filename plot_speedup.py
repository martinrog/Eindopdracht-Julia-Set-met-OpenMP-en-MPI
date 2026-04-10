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

baseline = times[0]
speedup = [baseline / t for t in times]

plt.figure()
plt.plot(labels, speedup, marker='o')

plt.title("Speedup vs MPI x OpenMP Configuration")
plt.xlabel("Configuration (MPI x OpenMP)")
plt.ylabel("Speedup")

plt.grid()
plt.xticks(rotation=45)
plt.tight_layout()

plt.savefig("speedup_plot.png")