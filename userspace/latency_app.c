#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <time.h>
#include <sys/mman.h>

#define NSEC_PER_SEC 1000000000ULL
#define INTERVAL_NS  10000000ULL // 10ms

int main(int argc, char *argv[])
{
    struct sched_param param;
    param.sched_priority = 80;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler failed (run as root)");
    }
    mlockall(MCL_CURRENT | MCL_FUTURE);

    int fd = open("/dev/fakesensor", O_RDONLY);
    if (fd < 0) {
        perror("Cannot open /dev/fakesensor");
        return 1;
    }

    FILE *fp = fopen("/tmp/latency_log.csv", "w");
    if (!fp) {
        perror("Cannot create log file");
        close(fd);
        return 1;
    }
    fprintf(fp, "sample,expected_ns,actual_ns,latency_us\n");

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    printf("Starting measurement (1000 samples)...\n");
    for (int i = 0; i < 1000; i++) {
        ts.tv_nsec += INTERVAL_NS;
        while (ts.tv_nsec >= NSEC_PER_SEC) {
            ts.tv_nsec -= NSEC_PER_SEC;
            ts.tv_sec += 1;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);

        uint32_t val;
        read(fd, &val, sizeof(val));

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        uint64_t expected_ns = (uint64_t)ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
        uint64_t actual_ns   = (uint64_t)now.tv_sec * NSEC_PER_SEC + now.tv_nsec;
        long latency_us = (long)((actual_ns - expected_ns) / 1000);

        fprintf(fp, "%d,%llu,%llu,%ld\n", i, expected_ns, actual_ns, latency_us);
    }

    printf("Done. Log saved to /tmp/latency_log.csv\n");
    fclose(fp);
    close(fd);
    return 0;
}
