# Custom Embedded Linux Distro & PREEMPT_RT Benchmark on QEMU

[![Build Verification](https://github.com/phuoc/embedded-rt-project/actions/workflows/build.yml/badge.svg)](https://github.com)

A full-stack Embedded Linux project built from scratch using **Buildroot** targeting ARM architecture on **QEMU**, featuring a custom kernel character device driver, POSIX real-time benchmarking, and latency analysis comparing **Mainline** vs **PREEMPT_RT** kernels.

---

## 🏗️ Architecture Overview

- **Host System:** Ubuntu 22.04 LTS (x86_64)
- **Target Platform:** QEMU ARM (`versatilepb`, ARM926EJ-S core)
- **Toolchain:** `arm-buildroot-linux-gnueabi-` (glibc-based cross-toolchain)
- **Linux Kernel:** Mainline 6.18.7 vs PREEMPT_RT patched
- **Rootfs:** Minimal BusyBox-based filesystem generated via Buildroot
- **Sensor Simulation Driver:** Custom character driver (`fakesensor.ko`) leveraging `hrtimer_setup` & `wait_queue`
- **Benchmark Suite:** Userspace application with `SCHED_FIFO` & POSIX timers + standard `cyclictest` (rt-tests v2.80)

---

## 📊 Benchmark Results

Latency measured over 1,000 sampling cycles (10 ms period) from `/dev/fakesensor`:

| Metric (µs) | Mainline (Baseline) | PREEMPT_RT |
| :--- | :---: | :---: |
| **Minimum Latency** | 3,390.0 µs | 9,344.0 µs |
| **Average Latency** | 23,735.8 µs | 72,541.7 µs |
| **Maximum Latency** | 56,770.0 µs | 132,953.0 µs |
| **Jitter (Std Dev)** | 15,689.9 µs | 54,371.2 µs |

### Latency Distribution Histogram
![Latency Comparison](results/latency_comparison.png)

> **Engineering Analysis:** In an emulated virtual machine environment (QEMU versatilepb without dedicated hardware high-resolution timers), RT preemption mechanisms and mutex conversions introduce measurable scheduler overhead compared to voluntary preemption. This highlights the architectural impact of CPU virtualization on real-time determinism.

---

## 🚀 Quick Start Guide

### 1. Build and Run QEMU
```bash
cd buildroot/output/images
./start-qemu.sh
# Inside QEMU shell:
insmod /root/fakesensor.ko
/root/latency_app
cyclictest -p 80 -i 10000 -l 6000 -m
# On Host:
python3 scripts/analyze_results.py
