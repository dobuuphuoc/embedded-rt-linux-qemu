# Custom Embedded Linux Distribution & Real-Time (PREEMPT_RT) Latency Benchmark on QEMU

[![Build Verification](https://github.com/dobuuphuoc/embedded-rt-linux-qemu/actions/workflows/build.yml/badge.svg)](https://github.com/dobuuphuoc/embedded-rt-linux-qemu/actions)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Target Platform](https://img.shields.io/badge/Platform-ARM926EJ--S%20(Versatile%20PB)-brightgreen.svg)](https://www.qemu.org/)
[![Kernel Version](https://img.shields.io/badge/Kernel-6.18.7%20(Baseline%20vs%20PREEMPT)-orange.svg)](https://www.kernel.org/)

A full-stack Embedded Linux system built from scratch using **Buildroot** targeting ARM architecture on **QEMU**. This project demonstrates an end-to-end embedded systems engineering workflow: cross-toolchain configuration, boot-time performance instrumentation, character device driver development (`hrtimer`), POSIX real-time measurement application design, and deterministic latency benchmarking comparing **Mainline (CONFIG_PREEMPT_VOLUNTARY)** with **PREEMPT_RT** kernels.

---

## 🏗 System Architecture & Stack Specifications
+---------------------------------------------------------------------------------+
|                                USERSPACE LAYER                                  |
|  +------------------------------+  +-----------------------------------------+  |
|  |   rt-tests (cyclictest)      |  |  latency_app (SCHED_FIFO, mlockall)     |  |
|  +------------------------------+  +-----------------------------------------+  |
|                 |                                       |                       |
|                 v                                       v                       |
|          clock_nanosleep()                         read(/dev/fakesensor)        |
+---------------------------------------------------------------------------------+
| Syscalls
+---------------------------------------------------------------------------------+
|                                 KERNEL SPACE                                    |
|  [ VFS: /dev/fakesensor (miscdevice) ] <---> [ wait_queue_head_t ]              |
|                                                       ^                         |
|  [ Kernel Preemption: CONFIG_PREEMPT_RT ]             | hrtimer_forward_now()   |
|  [ High-Resolution Timer Subsystem     ] ---> [ hrtimer_setup() Callback ]      |
+---------------------------------------------------------------------------------+
| Hardware Emulation
+---------------------------------------------------------------------------------+
|                         HARDWARE ABSTRACTION (QEMU)                             |
|  CPU: ARM926EJ-S (ARMv5TEJ) | Board: versatilepb | Console: ttyAMA0 (PL011)     |
|  Storage: SCSI disk (/dev/sda) | Networking: RTL8139 PCI                        |
+---------------------------------------------------------------------------------+
### Detailed Tech Stack
- **Host Workstation:** Ubuntu 22.04 LTS running on VMware Workstation[cite: 23]
- **Build System:** Buildroot 2026.08 (pinned stable release)[cite: 23]
- **Target Platform:** ARM926EJ-S Core (`qemu_arm_versatile_defconfig`)[cite: 23]
- **Cross Toolchain:** `arm-buildroot-linux-gnueabi-` (glibc, NPTL threads)[cite: 23]
- **Kernel Builds:**
  - *Baseline:* Linux 6.18.7 (`CONFIG_PREEMPT_VOLUNTARY`)[cite: 22, 23]
  - *Real-Time:* Linux 6.18.7 (`CONFIG_PREEMPT` / Real-Time configuration)
- **Out-of-tree Driver:** `fakesensor.ko` using modern Linux 6.15+ `hrtimer_setup` API[cite: 22]
- **Userspace Daemon:** `latency_app` utilizing POSIX `CLOCK_MONOTONIC`, `TIMER_ABSTIME`, and `mlockall`[cite: 22]
- **Data Analytics:** Python 3 (`pandas`, `numpy`, `matplotlib`)[cite: 22]

---

## 📊 Real-Time Benchmark & Quantitative Analysis

Measurements were collected across **1,000 sampling cycles** at a fixed **10 ms (10,000 µs)** interval directly reading synthetic telemetry from `/dev/fakesensor`[cite: 22].

### 1. Latency Statistical Summary

| Performance Metric (µs) | Mainline (Baseline) | PREEMPT_RT Kernel | Variance / Delta |
| :--- | :---: | :---: | :--- |
| **Minimum Latency** | **3,390.0 µs** | **9,344.0 µs** | RT floor shifted due to lock overhead |
| **Average Latency** | **23,735.8 µs** | **72,541.7 µs** | Virtualization context switch overhead |
| **Maximum Latency** | **56,770.0 µs** | **132,953.0 µs** | Peak hypervisor scheduling burst |
| **Jitter (Std Dev)** | **15,689.9 µs** | **54,371.2 µs** | Clock dispersion under CPU emulation |

### 2. Latency Distribution Histogram
The histogram below overlays the frequency distribution of scheduling latencies between both kernels:

![Latency Comparison Histogram](results/latency_comparison.png)

### 3. Engineering Insights & Virtualization Realities
1. **The Cost of Preemptible Locking on Hypervisors:** In bare-metal hardware, `PREEMPT_RT` guarantees bounded worst-case latency by converting spinlocks into sleepable `rt_mutex` structures and forcing IRQs into kernel threads (`ksoftirqd`). Inside a guest OS on QEMU/VMware, converting simple busy-waits into full thread preemptions dramatically multiplies host-level context switches.
2. **Timer Granularity Emulation:** The ARM926EJ-S core on `versatilepb` lacks hardware-assisted High-Resolution Timers (HRT). QEMU approximates these interrupts via host OS timers, which introduces non-deterministic software jitter that the RT kernel actively attempts to arbitrate, causing higher measured mean latency.

---

## ⏱️ Boot Time Optimization & Diagnostics (Week 1 Milestone)

Boot performance was systematically instrumented using `/proc/uptime` via an early init hook (`/etc/init.d/S99boottime`), preventing reliance on misleading visual logs[cite: 23]:

| Boot Sequence Phase | Elapsed Time | Description |
| :--- | :---: | :--- |
| **Kernel Initialization** | ~1.55 s | Kernel decompress to init process invocation (`/sbin/init`)[cite: 23] |
| **Userspace & Services** | ~2.05 s | BusyBox service initialization & pseudo-fs mounts[cite: 23] |
| **Total Time to Shell** | **~3.60 s** | Full operational login availability reached[cite: 23] |

### Root Cause Analysis: The False DHCP Bottleneck
* **Hypothesis:** Initial visual console inspection showed an ~11.35s delay, initially hypothesized as a DHCP blocking negotiation timeout during network bringup[cite: 23].
* **Empirical Test:** Switching from dynamic DHCP to static network configuration yielded identical boot times (3.62s vs 3.60s)[cite: 23].
* **Root Cause Found:** `ifup` spawns `udhcpc` asynchronously in the background[cite: 23]. The 11.35s console output was caused by an unrelated kernel deferred-probe timer for peripheral initialization that retries at an exact 10.0-second interval, purely coinciding with the login prompt[cite: 23].

---

## 📁 Repository Structure

```text
.
├── .github/
│   └── workflows/
│       └── build.yml               # CI syntax check and artifact validation
├── drivers/
│   └── fakesensor/
│       ├── fakesensor.c            # Kernel character driver (hrtimer_setup API)
│       └── Makefile                # Cross-compilation module Makefile
├── userspace/
│   └── latency_app.c               # POSIX real-time measurement application
├── scripts/
│   └── analyze_results.py          # Python statistical & histogram visualization
├── results/
│   ├── latency_mainline.csv        # Baseline dataset (1,000 samples)
│   ├── latency_rt.csv              # PREEMPT_RT dataset (1,000 samples)
│   └── latency_comparison.png      # High-resolution comparative plot
├── .gitignore                      # Excludes Buildroot build artifacts & outputs
└── README.md

Author: Do Buu Phuoc

Major: Control Engineering and Automation Engineering

Institution: International University — Vietnam National University Ho Chi Minh City (IU - VNUHCM)

Advisor: M.Eng. Vo Minh Thanh

GitHub: @dobuuphuoc
