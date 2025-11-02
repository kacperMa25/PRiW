import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
df = pd.read_csv("mandelbrot_times_pc.csv")
print(df)
# Plot setup
plt.figure(figsize=(8, 5))
for method in df["method"].unique():
    subset = df[df["method"] == method]
    plt.plot(subset["threads"], subset["time_seconds"], marker="o", label=method)

# Labels and legend
plt.xlabel("Threads")
plt.ylabel("Time (seconds)")
plt.xscale("log", base=2)
plt.title("Execution Time vs Threads by Method")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.5)
plt.tight_layout()
plt.show()
