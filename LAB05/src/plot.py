import pandas as pd
import matplotlib.pyplot as plt
import os

df = pd.read_csv("results.csv")

os.makedirs("../Plots", exist_ok=True)

block_sizes = df["BlockSize"].unique()
methods = df["Name"].unique()

for b in block_sizes:
    sub_b = df[df["BlockSize"] == b]

    plt.figure()
    for method in methods:
        sub_m = sub_b[sub_b["Name"] == method]

        mean_times = (
            sub_m.groupby("Threads")["AvgTime"]
            .mean()
            .reset_index()
            .sort_values("Threads")
        )

        plt.plot(mean_times["Threads"], mean_times["AvgTime"], marker="o", label=method)

    plt.xlabel("Threads")
    plt.ylabel("Average Time (s)")
    plt.title(f"Time vs Threads (BlockSize = {b})")
    plt.legend()

    filename = f"plot_BlockSize_{b}.png"
    plt.savefig(f"../Plots/{filename}")
    plt.close()

print("Wykresy zapisane.")
