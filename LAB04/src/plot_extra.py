import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("../mandelbrot_times_pc_sizes.csv")

sizes = sorted(df["blockSize"].unique())


plt.figure(figsize=(8, 5))
for method in ["Static", "Dynamic", "Guided"]:
    data = df[df["method"] == method]
    plt.plot(data["blockSize"], data["time_seconds"], marker="o", label=f"{method}")

    plt.xlabel("BlockSize")
    plt.ylabel("Time (seconds)")
    plt.xscale("log", base=2)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    plt.savefig("../Plots/mandelbrot_block_size.png")
