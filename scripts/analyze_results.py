import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def analyze_and_plot():
    results_dir = os.path.join(os.path.dirname(__file__), "../results")
    mainline_csv = os.path.join(results_dir, "latency_mainline.csv")
    rt_csv = os.path.join(results_dir, "latency_rt.csv")
    output_png = os.path.join(results_dir, "latency_comparison.png")

    if not os.path.exists(mainline_csv) or not os.path.exists(rt_csv):
        print("err: not found file latency_mainline.csv or latency_rt.csv in folder results/")
        return

    df_mainline = pd.read_csv(mainline_csv)
    df_rt = pd.read_csv(rt_csv)

    lat_m = df_mainline['latency_us']
    lat_rt = df_rt['latency_us']

    # In bảng thống kê
    print("="*55)
    print(f"{'Metric (us)':<20} | {'Mainline (Baseline)':<15} | {'PREEMPT_RT':<12}")
    print("="*55)
    print(f"{'Minimum Latency':<20} | {lat_m.min():<15.1f} | {lat_rt.min():<12.1f}")
    print(f"{'Average Latency':<20} | {lat_m.mean():<15.1f} | {lat_rt.mean():<12.1f}")
    print(f"{'Maximum Latency':<20} | {lat_m.max():<15.1f} | {lat_rt.max():<12.1f}")
    print(f"{'Jitter (Std Dev)':<20} | {lat_m.std():<15.1f} | {lat_rt.std():<12.1f}")
    print("="*55)

    # Vẽ biểu đồ Histogram
    plt.figure(figsize=(10, 6))
    plt.hist(lat_m, bins=50, alpha=0.6, color='red', label=f'Mainline (Max: {lat_m.max():.0f} us)')
    plt.hist(lat_rt, bins=50, alpha=0.6, color='blue', label=f'PREEMPT_RT (Max: {lat_rt.max():.0f} us)')

    plt.title('Latency Distribution Comparison: Mainline vs PREEMPT_RT', fontsize=14, fontweight='bold')
    plt.xlabel('Latency (microseconds)', fontsize=12)
    plt.ylabel('Sample Count', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(fontsize=11)

    plt.tight_layout()
    plt.savefig(output_png, dpi=300)
    print(f"Distributed Diagram is saved at: {output_png}")

if __name__ == '__main__':
    analyze_and_plot()
