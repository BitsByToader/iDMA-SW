#ifndef _MY_DMA_COMMON_H
#define _MY_DMA_COMMON_H

#include <linux/ioctl.h>
#include <linux/types.h>

/* Helper Constants */
#define MY_DMA_MAGIC 'I'
#define MODE_MEM_TO_MEM    0
#define MODE_MEM_TO_STREAM 1
#define MODE_STREAM_TO_MEM 2

/* * User -> Kernel Interface
 * @src_ptr: Userspace virtual address of the source buffer (ignored in dev2mem mode).
 * @dst_ptr: Userspace virtual address of the destination buffer (ignored in mem2dev mode).
 * @len: Length in bytes
 * @mode: 0=Mem2Mem, 1=Mem2Dev, 2=Dev2Mem
 */
struct idma_ioctl_data {
    __u64 src_ptr;
    __u64 dst_ptr;
    __u32 len;
    __u32 mode;
};

#define IOCTL_DMA_SUBMIT _IOW(MY_DMA_MAGIC, 1, struct idma_ioctl_data)

#endif
