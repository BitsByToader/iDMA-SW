#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "idma_common.h"

#define DEVICE_FILE "/dev/idma"
#define BUFFER_SIZE 3*1048 // 3KB, should be less than one page

int main() {
    int fd;
    struct idma_ioctl_data data;
    char *src_buf, *dst_buf;
    int ret;

    printf("[User] Opening DMA Driver...\n");
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open driver (did you run 'make load'?)");
        return -1;
    }

    /* Allocate aligned memory for zero-copy efficiency */
    if (posix_memalign((void **)&src_buf, 4096, BUFFER_SIZE)) {
        perror("Aligned malloc failed");
        return -1;
    }
    if (posix_memalign((void **)&dst_buf, 4096, BUFFER_SIZE)) {
        perror("Aligned malloc failed");
        return -1;
    }

    /* Initialize Data */
    memset(src_buf, 0x00, BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; i+=8) {
        src_buf[i] = 0xA0 + i/8;
    }
    printf("Initial buffer:\n");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%x ", src_buf[i]);
    }
    printf("\n");
    
    memset(dst_buf, 0x00, BUFFER_SIZE); // Fill Dest with 0x00

    printf("[User] Preparing DMA Memcpy (0xAA pattern)...\n");

    /* Setup IOCTL Data */
    data.src_ptr = (unsigned long)src_buf;
    data.dst_ptr = (unsigned long)dst_buf; 
    data.len = BUFFER_SIZE;
    data.mode = MODE_MEM_TO_MEM;

    /* Call the Driver */
    ret = ioctl(fd, IOCTL_DMA_SUBMIT, &data);
    if (ret < 0) {
        perror("M2S IOCTL Failed");
        goto cleanup;
    }

    //data.src_ptr = 0;
    //data.dst_ptr = (unsigned long) dst_buf;
    //data.len = BUFFER_SIZE;
    //data.mode = MODE_STREAM_TO_MEM;
    
    //ret = ioctl(fd, IOCTL_DMA_SUBMIT, &data);
    //if (ret < 0) {
    //    perror("S2M IOCTL Failed");
    //    goto cleanup;
    //}

    printf("[User] DMA Complete!\n");
   
    for (int i = 0; i < 100000000; i++) {}

    /* Verify Result */
    if (memcmp(src_buf, dst_buf, BUFFER_SIZE) == 0) {
        printf("[User] SUCCESS: Destination matches Source!\n");
    } else {
        printf("[User] FAILURE: Data mismatch.\n");
        printf("Expected: %02x, Got: %02x\n", (unsigned char)src_buf[0], (unsigned char)dst_buf[0]);
    }

    /* Cleanup */
cleanup:
    free(src_buf);
    free(dst_buf);
    close(fd);
    return 0;
}
