#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include "idma_common.h"

#define DEVICE_FILE "/dev/idma"
#define BUFFER_SIZE 4*1024  /* Exactly 1 Page to prevent SG-list corruption in current driver */
#define ITERATIONS  5     /* Number of transfers to average */

/* Helper to calculate time difference in seconds */
double get_elapsed_time(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_nsec - start->tv_nsec) / 1e9;
}

int main() {
    int fd;
    struct idma_ioctl_data data;
    char *src_buf, *dst_buf;
    struct timespec start_time, end_time;
    double total_time = 0.0;
    double total_bytes = (double)BUFFER_SIZE * ITERATIONS;

    printf("[Benchmark] Opening DMA Driver...\n");
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open driver");
        return -1;
    }

    /* Allocate page-aligned memory */
    if (posix_memalign((void **)&src_buf, 4096, BUFFER_SIZE) || 
        posix_memalign((void **)&dst_buf, 4096, BUFFER_SIZE)) {
        perror("Aligned malloc failed");
        close(fd);
        return -1;
    }

    /* Initialize with dummy data */
    memset(src_buf, 0xAB, BUFFER_SIZE);
    memset(dst_buf, 0x00, BUFFER_SIZE);

    /* Setup IOCTL Data */
    data.src_ptr = (unsigned long)src_buf;
    data.dst_ptr = (unsigned long)dst_buf; 
    data.len = BUFFER_SIZE;
    data.mode = MODE_MEM_TO_MEM;

    printf("[Benchmark] Running %d iterations of %d bytes...\n", ITERATIONS, BUFFER_SIZE);

    /* WARM-UP RUN (Don't measure this one, ensures caches/TLB are hot) */
    ioctl(fd, IOCTL_DMA_SUBMIT, &data);

    /* ACTUAL BENCHMARK LOOP */
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (int i = 0; i < ITERATIONS; i++) {
        if (ioctl(fd, IOCTL_DMA_SUBMIT, &data) < 0) {
            perror("DMA IOCTL Failed during benchmark");
            goto cleanup;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    /* Verify Data Integrity on the last run */
    if (memcmp(src_buf, dst_buf, BUFFER_SIZE) != 0) {
        printf("[Benchmark] ERROR: Data corruption detected during run!\n");
        goto cleanup;
    }

    /* Calculate Statistics */
    total_time = get_elapsed_time(&start_time, &end_time);
    
    /* Convert to Megabytes per second (MB/s) */
    double mb_transferred = total_bytes / (1024.0 * 1024.0);
    double mbps = mb_transferred / total_time;
    double avg_latency_us = (total_time / ITERATIONS) * 1e6;

    printf("\n--- DMA Benchmark Results ---\n");
    printf("Total Transfer:    %.2f MB\n", mb_transferred);
    printf("Total Time:        %.4f seconds\n", total_time);
    printf("Avg Latency:       %.2f microseconds / transfer\n", avg_latency_us);
    printf("Throughput:        %.2f MB/s\n", mbps);
    printf("-----------------------------\n");

cleanup:
    free(src_buf);
    free(dst_buf);
    close(fd);
    return 0;
}
