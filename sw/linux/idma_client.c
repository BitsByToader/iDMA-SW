#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include "idma_common.h"

#define CLIENT_NAME "idma"

static struct dma_chan *dma_chan;
static struct completion dma_comp;

/* 
 * Callback: Called when the backend finishes the job 
 */
static void dma_callback_func(void *param)
{
    struct completion *comp = param;
    complete(comp);
}

/* 
 * Helper: Create Scatter-Gather list from User Pages
 */
static int pin_and_map_buffer(unsigned long user_addr, size_t len, int write,
                              struct page ***pages_out, struct scatterlist **sg_out, 
                              int *n_pages, int *n_ents)
{
    struct page **pages;
    struct scatterlist *sg;
    int ret, i;
    unsigned long start = user_addr;
    unsigned long end = user_addr + len;
    unsigned long page_count = (end >> PAGE_SHIFT) - (start >> PAGE_SHIFT) + 1;

    /* Allocate array to hold page pointers */
    pages = kvmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
    if (!pages) return -ENOMEM;

    /* Pin User Pages (Locks them in RAM) */
    /* Note: FOLL_WRITE if DMA writes TO memory. */
    ret = pin_user_pages_fast(user_addr, page_count, 
                              write ? FOLL_WRITE | FOLL_FORCE : FOLL_FORCE, 
                              pages);
    if (ret < 0) {
        kvfree(pages);
        return ret;
    }
    *n_pages = ret;

    /* Allocate Scatter-Gather List */
    sg = kvmalloc_array(*n_pages, sizeof(struct scatterlist), GFP_KERNEL);
    if (!sg) {
        unpin_user_pages(pages, *n_pages);
        kvfree(pages);
        return -ENOMEM;
    }

    sg_init_table(sg, *n_pages);
    for (i = 0; i < *n_pages; i++) {
        /* Handle first/last page offset if buffer isn't page-aligned */
        unsigned int offset = (i == 0) ? (user_addr & ~PAGE_MASK) : 0;
        unsigned int p_len = PAGE_SIZE - offset;
        if (i == *n_pages - 1) {
            /* Last page might be partial */
            unsigned int end_offset = (user_addr + len) & ~PAGE_MASK;
            if (end_offset) p_len = end_offset - offset;
        }
        sg_set_page(&sg[i], pages[i], p_len, offset);
    }

    /* Map SG to DMA Addresses */
    *n_ents = dma_map_sg(dma_chan->device->dev, sg, *n_pages, 
                         write ? DMA_FROM_DEVICE : DMA_TO_DEVICE);
    
    *pages_out = pages;
    *sg_out = sg;
    return 0;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct idma_ioctl_data data;
    struct dma_async_tx_descriptor *tx = NULL;
    struct dma_slave_config cfg = {0};
    
    /* Memory tracking */
    struct page **src_pages = NULL, **dst_pages = NULL;
    struct scatterlist *src_sg = NULL, *dst_sg = NULL;
    int src_n_pages = 0, dst_n_pages = 0;
    int src_n_ents = 0, dst_n_ents = 0;
    int ret = 0;
    dma_cookie_t cookie;

    if (cmd != IOCTL_DMA_SUBMIT) return -EINVAL;
    if (copy_from_user(&data, (void __user *)arg, sizeof(data))) return -EFAULT;

    if (!dma_chan) return -ENODEV;

    pr_info("[iDMA][Frontend] Prepping transfer...\n");

    /* --- SETUP BASED ON MODE --- */
    
    if (data.mode == MODE_MEM_TO_MEM) {
        /* MEMCPY: Pin Source and Dest */
        
        // Map Source (Read from User -> DMA TO DEV)
        ret = pin_and_map_buffer(data.src_ptr, data.len, 0, &src_pages, &src_sg, &src_n_pages, &src_n_ents);
        if (ret) goto out;

        // Map Dest (Write to User -> DMA FROM DEV)
        ret = pin_and_map_buffer(data.dst_ptr, data.len, 1, &dst_pages, &dst_sg, &dst_n_pages, &dst_n_ents);
        if (ret) goto out;

        pr_info("\tPinned and mapped buffers for transfer.\n");

        /* NOTE: dmaengine_prep_dma_memcpy usually takes dma_addr_t, not SG list.
           Real efficient drivers implement prep_dma_sg for memcpy. 
           For this example, we take the first mapped chunk. */
        tx = dmaengine_prep_dma_memcpy(dma_chan, 
                                       sg_dma_address(dst_sg), 
                                       sg_dma_address(src_sg), 
                                       data.len, 
                                       DMA_CTRL_ACK | DMA_PREP_INTERRUPT);

    } else if (data.mode == MODE_MEM_TO_STREAM) {
        /* MEM -> STREAM */

        // Configure Stream (Slave)
        cfg.direction = DMA_MEM_TO_DEV;
        cfg.dst_addr = 0;
        cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
        dmaengine_slave_config(dma_chan, &cfg);

        // Map Source Memory
        ret = pin_and_map_buffer(data.src_ptr, data.len, 0, &src_pages, &src_sg, &src_n_pages, &src_n_ents);
        if (ret) goto out;

        // Prep Slave SG
        tx = dmaengine_prep_slave_sg(dma_chan, src_sg, src_n_ents, 
                                     DMA_MEM_TO_DEV, DMA_CTRL_ACK | DMA_PREP_INTERRUPT);

    } else if (data.mode == MODE_STREAM_TO_MEM) {
        /* STREAM -> MEM */

        // Configure Stream (Slave)
        cfg.direction = DMA_DEV_TO_MEM;
        cfg.src_addr = 0;
        cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
        dmaengine_slave_config(dma_chan, &cfg);

        // Map Dest Memory
        ret = pin_and_map_buffer(data.dst_ptr, data.len, 1, &dst_pages, &dst_sg, &dst_n_pages, &dst_n_ents);
        if (ret) goto out;

        // Prep Slave SG
        tx = dmaengine_prep_slave_sg(dma_chan, dst_sg, dst_n_ents, 
                                     DMA_DEV_TO_MEM, DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
    }

    pr_info("[iDMA][Frontend] Prepped transfer. Submitting...\n");

    if (!tx) {
        ret = -EIO;
        goto out;
    }

    /* --- SUBMIT --- */
    init_completion(&dma_comp);
    tx->callback = dma_callback_func;
    tx->callback_param = &dma_comp;

    cookie = dmaengine_submit(tx);
    dma_async_issue_pending(dma_chan);

    /* Wait for completion (Timeout 1s) */
    if (!wait_for_completion_timeout(&dma_comp, msecs_to_jiffies(1000))) {
        dmaengine_terminate_all(dma_chan);
        ret = -ETIMEDOUT;
    }

    pr_info("[iDMA][Frontend] Transfer done.\n");

out:
    /* --- CLEANUP --- */
    if (src_pages) {
        dma_unmap_sg(dma_chan->device->dev, src_sg, src_n_pages, DMA_TO_DEVICE);
        unpin_user_pages(src_pages, src_n_pages);
        kvfree(src_sg);
        kvfree(src_pages);
    }
    if (dst_pages) {
        dma_unmap_sg(dma_chan->device->dev, dst_sg, dst_n_pages, DMA_FROM_DEVICE);
        unpin_user_pages(dst_pages, dst_n_pages);
        kvfree(dst_sg);
        kvfree(dst_pages);
    }
    
    return ret;
}

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = my_ioctl,
};

static struct miscdevice my_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "idma",
    .fops = &my_fops,
};

static bool idma_filter_fn(struct dma_chan *chan, void *param)
{
    /* Check if the device attached to this channel has our provider's name */
    if (chan->device && chan->device->dev && chan->device->dev->driver) {
        if (strcmp(chan->device->dev->driver->name, "idma-provider") == 0) {
            return true;
        }
    }
    return false; // Not iDMA driver, keep looking.
}

static int __init idma_client_init(void)
{
    dma_cap_mask_t mask;
    
    // Ask for a channel that supports MEMCPY and SLAVE
    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY, mask);
    dma_cap_set(DMA_SLAVE, mask);

    // Request ANY channel matching mask
    dma_chan = dma_request_channel(mask, idma_filter_fn, NULL);
    if (!dma_chan) {
        pr_err("No DMA channel found!\n");
        return -ENODEV;
    }

    pr_info("[iDMA][Frontend] iDMA client loaded.\n");

    return misc_register(&my_misc_dev);
}

static void __exit idma_client_exit(void)
{
    if (dma_chan) dma_release_channel(dma_chan);
    misc_deregister(&my_misc_dev);
}

module_init(idma_client_init);
module_exit(idma_client_exit);
MODULE_LICENSE("GPL");
