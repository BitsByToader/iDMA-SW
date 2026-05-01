#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>

#include "virt-dma.h"
#include "idma_common.h"

#define DRIVER_NAME "idma-provider"

/* Hardware Register Offsets (from idma_desc64.hjson) */
#define IDMA_REG_DESC_ADDR  0x00
#define IDMA_REG_STATUS     0x08

#define IDMA_STATUS_BUSY      BIT(0)
#define IDMA_STATUS_FIFO_FULL BIT(1)

/* Hardware Descriptor Flags (from idma_desc64_top.sv) */
#define IDMA_FLAG_IRQ_EN      BIT(0)
#define IDMA_FLAG_SRC_INCR    (1 << 1)
#define IDMA_FLAG_DST_INCR    (1 << 3)
#define IDMA_FLAG_DECOUPLE    BIT(5)
#define IDMA_FLAG_SERIALIZE   BIT(6)
#define IDMA_FLAG_DEBURST     BIT(7)

/* Protocol definitions extracted from baremetal (Bits 26:24 and 29:27) */
#define IDMA_PROT_MEM         0x0  // AXI
#define IDMA_PROT_STREAM      0x5  // AXIS

/* Macro to pack protocol settings */
#define IDMA_SET_PROT(src, dst) ((((dst) & 0x7) << 27) | (((src) & 0x7) << 24))

/* Common flags for our transactions: 0x6B */
#define IDMA_FLAGS_BASE (IDMA_FLAG_SRC_INCR | \
                         IDMA_FLAG_DST_INCR | IDMA_FLAG_DECOUPLE | \
                         IDMA_FLAG_SERIALIZE)

/* * Hardware descriptor mapped to SV packed `descriptor_t`.
 * 32-bytes total. __aligned(32) ensures burst neatness.
 */
struct idma_hw_desc {
    u32 length;
    u32 flags;
    u64 next_desc;
    u64 src;
    u64 dst;
} __attribute__((packed, aligned(32)));

struct idma_sw_desc {
    struct virt_dma_desc vdesc;
    struct idma_hw_desc *hw_vaddr;
    dma_addr_t hw_paddr;
    unsigned int desc_cnt;
    struct work_struct free_work; /* queue deferred freeing */
};

struct idma_dev {
    struct dma_device dma_dev;
    struct virt_dma_chan vchan;
    void __iomem *regs;
    struct dma_slave_config slave_cfg;
    struct list_head submitted_jobs;
};

static inline struct idma_dev *to_idma_dev(struct dma_chan *chan) {
    return container_of(chan->device, struct idma_dev, dma_dev);
}

static inline struct idma_sw_desc *to_idma_sw_desc(struct virt_dma_desc *vdesc) {
    return container_of(vdesc, struct idma_sw_desc, vdesc);
}

static void idma_feed_hardware(struct idma_dev *mdev)
{
    struct virt_dma_desc *vd;
    struct idma_sw_desc *sw_desc;
    u32 status;

    /* Loop as long as there are jobs waiting in the virt-dma queue */
    while (!list_empty(&mdev->vchan.desc_issued)) {
        /* Check if the hardware can accept another job */
        status = readq(mdev->regs + IDMA_REG_STATUS);
        if (status & IDMA_STATUS_FIFO_FULL) {
            pr_info("[iDMA][Backend] Descriptor FIFO is full. Job is stalled until FIFO empties.\n");
            break; /* Hardware FIFO is full, stop feeding! */
        }

        /* Safe to pop the next job from virt-dma */
        vd = vchan_next_desc(&mdev->vchan);
        sw_desc = to_idma_sw_desc(vd);

        /* Move it to our shadow queue so the ISR can find it later */
        list_move_tail(&vd->node, &mdev->submitted_jobs);

        /* Write to the hardware register to queue it up */
        pr_info("[iDMA][Backend] Submitting DMA Chain (Physical Addr: %pad)\n", &sw_desc->hw_paddr);
        writeq(sw_desc->hw_paddr, mdev->regs + IDMA_REG_DESC_ADDR);
    }
}

static irqreturn_t idma_irq_handler(int irq, void *dev_id)
{
    struct idma_dev *mdev = dev_id;
    struct virt_dma_desc *vd;
    unsigned long flags;

    spin_lock_irqsave(&mdev->vchan.lock, flags);

    pr_info("[iDMA][Backend] Got IRQ!");

    /* Complete the oldest submitted job */
    vd = list_first_entry_or_null(&mdev->submitted_jobs, struct virt_dma_desc, node);
    if (vd) {
        list_del(&vd->node);         /* Remove from our tracking list */
        vchan_cookie_complete(vd);   /* Tell the kernel this job is done! */
    } else {
        pr_warn("[iDMA][Backend] Spurious IRQ! No jobs in the submitted queue.\n");
    }

    /* A job just finished, which means a slot just opened up in the HW FIFO. Feed it another job immediately. */
    idma_feed_hardware(mdev);

    spin_unlock_irqrestore(&mdev->vchan.lock, flags);

    return IRQ_HANDLED;
}

static void idma_desc_free_worker(struct work_struct *work)
{
    /* Recover the pointer to our software descriptor */
    struct idma_sw_desc *desc = container_of(work, struct idma_sw_desc, free_work);
    struct idma_dev *mdev = to_idma_dev(desc->vdesc.tx.chan);

    /* Safe to call blocking free */
    if (desc->hw_vaddr) {
        dma_free_coherent(mdev->dma_dev.dev,
                          desc->desc_cnt * sizeof(struct idma_hw_desc),
                          desc->hw_vaddr, desc->hw_paddr);
    }
    
    kfree(desc);
}

static void idma_desc_free(struct virt_dma_desc *vdesc) {
    struct idma_sw_desc *desc = to_idma_sw_desc(vdesc);

    // Schedule freeing for later
    INIT_WORK(&desc->free_work, idma_desc_free_worker);
    schedule_work(&desc->free_work);
}

static int idma_config(struct dma_chan *chan, struct dma_slave_config *cfg) {
    struct idma_dev *mdev = to_idma_dev(chan);
    memcpy(&mdev->slave_cfg, cfg, sizeof(*cfg));
    return 0;
}

/* Mem2Mem Preparation */
static struct dma_async_tx_descriptor *idma_prep_memcpy(
    struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
    size_t len, unsigned long flags)
{
    struct idma_dev *mdev = to_idma_dev(chan);
    struct idma_sw_desc *desc;

    pr_info("[iDMA][Backend] Begin prep memcpy.\n");

    desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
    if (!desc) return NULL;

    pr_info("\tAllocated SW descriptor.\n");

    desc->desc_cnt = 1;
    desc->hw_vaddr = dma_alloc_coherent(mdev->dma_dev.dev,
                                        sizeof(struct idma_hw_desc),
                                        &desc->hw_paddr, GFP_NOWAIT);
    if (!desc->hw_vaddr) {
        kfree(desc);
        return NULL;
    }

    pr_info("\tAllocated HW descriptor.\n");

    desc->hw_vaddr[0].length = len;
    desc->hw_vaddr[0].flags  = IDMA_FLAGS_BASE | IDMA_FLAG_IRQ_EN | IDMA_SET_PROT(IDMA_PROT_MEM, IDMA_PROT_MEM);
    desc->hw_vaddr[0].next_desc = 0xFFFFFFFFFFFFFFFFULL; /* End of chain */
    desc->hw_vaddr[0].src = src;
    desc->hw_vaddr[0].dst = dest;

    pr_info("[iDMA][Backend] Prepped mem2mem transfer. Descriptor at virt address: %p. Phys addr: %llx\n", desc->hw_vaddr, desc->hw_paddr);
    pr_info("[iDMA][Backend] \tSRC addr: %llx. DST addr: %llx. Next desc: %llx.\n", desc->hw_vaddr[0].src, desc->hw_vaddr[0].dst, desc->hw_vaddr[0].next_desc);

    return vchan_tx_prep(&mdev->vchan, &desc->vdesc, flags);
}

/* Mem2Stream and Stream2Mem Preparation */
static struct dma_async_tx_descriptor *idma_prep_slave_sg(
    struct dma_chan *chan, struct scatterlist *sgl,
    unsigned int sg_len, enum dma_transfer_direction direction,
    unsigned long flags, void *context)
{
    struct idma_dev *mdev = to_idma_dev(chan);
    struct idma_sw_desc *desc;
    struct scatterlist *sg;
    int i;

    desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
    if (!desc) return NULL;

    desc->desc_cnt = sg_len;
    desc->hw_vaddr = dma_alloc_coherent(mdev->dma_dev.dev,
                                        sg_len * sizeof(struct idma_hw_desc),
                                        &desc->hw_paddr, GFP_NOWAIT);
    if (!desc->hw_vaddr) {
        kfree(desc);
        return NULL;
    }

    pr_info("[iDMA][Backend] Prepping SG transfer with %d descriptors.\n", sg_len);
    for_each_sg(sgl, sg, sg_len, i) {
        desc->hw_vaddr[i].length = sg_dma_len(sg);

        if (direction == DMA_MEM_TO_DEV) {
            /* Mem2Stream (AXI -> AXIS) */
            desc->hw_vaddr[i].flags = IDMA_FLAGS_BASE | IDMA_SET_PROT(IDMA_PROT_MEM, IDMA_PROT_STREAM);
            desc->hw_vaddr[i].src = sg_dma_address(sg);
            desc->hw_vaddr[i].dst = 0;
        } else if (direction == DMA_DEV_TO_MEM) {
            /* Stream2Mem (AXIS -> AXI) */
            desc->hw_vaddr[i].flags = IDMA_FLAGS_BASE | IDMA_SET_PROT(IDMA_PROT_STREAM, IDMA_PROT_MEM);
            desc->hw_vaddr[i].src = 0;
            desc->hw_vaddr[i].dst = sg_dma_address(sg);
        }

        if (i == sg_len - 1) {
            desc->hw_vaddr[i].flags |= IDMA_FLAG_IRQ_EN;
            desc->hw_vaddr[i].next_desc = 0xFFFFFFFFFFFFFFFFULL;
        } else {
            desc->hw_vaddr[i].next_desc = desc->hw_paddr + ((i + 1) * sizeof(struct idma_hw_desc));
        }

        pr_info("[iDMA][Backend] \tPrepped descriptor for stream transfer. Descriptor at virt address: %p. Phys addr: %llx\n", &desc->hw_vaddr[i], desc->hw_paddr + (i* sizeof(struct idma_hw_desc)));
        pr_info("[iDMA][Backend] \t\tSRC addr: %llx. DST addr: %llx. Next desc: %llx.\n", desc->hw_vaddr[i].src, desc->hw_vaddr[i].dst, desc->hw_vaddr[i].next_desc);
    }

    return vchan_tx_prep(&mdev->vchan, &desc->vdesc, flags);
}

static void idma_issue_pending(struct dma_chan *chan)
{
    struct idma_dev *mdev = to_idma_dev(chan);
    unsigned long flags;
    
    spin_lock_irqsave(&mdev->vchan.lock, flags);

    if (vchan_issue_pending(&mdev->vchan)) {
       idma_feed_hardware(mdev); 
    }

    spin_unlock_irqrestore(&mdev->vchan.lock, flags);
}

static void idma_free_chan_resources(struct dma_chan *chan) {
    vchan_free_chan_resources(to_virt_chan(chan));
}

static int idma_terminate_all(struct dma_chan *chan)
{
    struct idma_dev *mdev = to_idma_dev(chan);
    unsigned long flags;
    LIST_HEAD(head);

    spin_lock_irqsave(&mdev->vchan.lock, flags);

    /* Extract all pending, issued, and queued descriptors */
    vchan_get_all_descriptors(&mdev->vchan, &head);

    /* Clear out submitted jobs. */
    list_splice_tail_init(&mdev->submitted_jobs, &head);
    
    spin_unlock_irqrestore(&mdev->vchan.lock, flags);
    
    /* Free cleared jobs. */
    vchan_dma_desc_free_list(&mdev->vchan, &head);
    
    pr_info("[iDMA][Backend] Terminated all pending software descriptors.\n");
    return 0;
}

static int idma_probe(struct platform_device *pdev)
{
    struct idma_dev *mdev;
    struct dma_device *dma;
    int ret;

    pr_info("[iDMA][Backend] Begin iDMA probe.\n");
    
    // Alloc memory for data structures.
    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) return -ENOMEM;
    pr_info("[iDMA][Backend] Allocated memory for device structures.\n");   
    
    /* Get mapped hardware registers */
    mdev->regs = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(mdev->regs)) {
        dev_err(&pdev->dev, "Failed to map memory region\n");
        return PTR_ERR(mdev->regs);
    }

    int irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(&pdev->dev, "Failed to get IRQ from Device Tree\n");
        return irq;
    }

    // Configure dma API.
    dma = &mdev->dma_dev;

    dma_cap_zero(dma->cap_mask);
    dma_cap_set(DMA_MEMCPY, dma->cap_mask);
    dma_cap_set(DMA_SLAVE, dma->cap_mask);

    dma->dev = &pdev->dev;
    dma->device_prep_dma_memcpy = idma_prep_memcpy;
    dma->device_prep_slave_sg = idma_prep_slave_sg;
    dma->device_config = idma_config;
    dma->device_issue_pending = idma_issue_pending;
    dma->device_free_chan_resources = idma_free_chan_resources;
    dma->device_tx_status = dma_cookie_status;
    dma->device_terminate_all = idma_terminate_all;

    dma->src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
    dma->dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
    dma->directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV) | BIT(DMA_MEM_TO_MEM);
    dma->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;

    INIT_LIST_HEAD(&dma->channels);
    INIT_LIST_HEAD(&mdev->submitted_jobs);
    pr_info("[iDMA][Backend] Initiated DMA channels.\n");

    vchan_init(&mdev->vchan, dma);
    mdev->vchan.desc_free = idma_desc_free;

    ret = dma_async_device_register(dma);
    if (ret) return ret;
   
    // Set up interrupt.
    ret = devm_request_irq(&pdev->dev, irq, idma_irq_handler, 0, dev_name(&pdev->dev), mdev);
    if (ret) {
        dev_err(&pdev->dev, "[iDMA] IRQ %d register failed: %d\n", irq, ret);
        dma_async_device_unregister(dma);
        return ret;
    }
    pr_info("[iDMA][Backend] IRQ registered!\n");

    platform_set_drvdata(pdev, mdev);
    pr_info("[iDMA][Backend] Registered async DMA.\n");
    pr_info("[iDMA][Backend] iDMA Provider Loaded.\n");

    return 0;
}

static int idma_remove(struct platform_device *pdev) {
    struct idma_dev *mdev = platform_get_drvdata(pdev);
    
    dma_async_device_unregister(&mdev->dma_dev);
    tasklet_kill(&mdev->vchan.task);
    
    return 0;
}

static const struct of_device_id idma_of_match[] = {
    { .compatible = "pulp,idma-provider", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, idma_of_match);

static struct platform_driver idma_driver = {
    .probe = idma_probe, .remove = idma_remove,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = idma_of_match,
    },
};

module_platform_driver(idma_driver);

MODULE_LICENSE("GPL"); MODULE_DESCRIPTION("iDMA DMA Engine Provider");
