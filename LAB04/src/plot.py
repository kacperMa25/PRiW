import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("../mandelbrot_times_pc.csv")

sizes = sorted(df["size"].unique())

for size in sizes:
    subset = df[df["size"] == size]

    plt.figure(figsize=(8, 5))
    for method in ["Static", "Dynamic", "Guided"]:
        data = subset[subset["method"] == method]
        plt.plot(
            data["threads"], data["time_seconds"], marker="o", label=f"{method}{size}"
        )

    plt.xlabel("Threads")
    plt.ylabel("Time (seconds)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig(f"../Plots/mandelbrot_size_{size}.png")
