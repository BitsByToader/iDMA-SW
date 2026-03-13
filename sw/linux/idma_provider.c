#include <linux/module.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/interrupt.h>
#include "virt-dma.h"
#include "idma_common.h"

#define DRIVER_NAME "idma-provider"

#define IDMA_JOB_DONE_IRQ_NO    89

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
#define IDMA_FLAGS_BASE (IDMA_FLAG_IRQ_EN | IDMA_FLAG_SRC_INCR | \
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
};

struct idma_dev {
    struct dma_device dma_dev;
    struct virt_dma_chan vchan;
    void __iomem *regs;
    struct dma_slave_config slave_cfg;
};

static irqreturn_t idma_irq_handler(int irq, void *dev_id) {
    pr_info("[iDMA][Backend][IRQ] Got IRQ!");
    return IRQ_HANDLED;
}

static inline struct idma_dev *to_idma_dev(struct dma_chan *chan) {
    return container_of(chan->device, struct idma_dev, dma_dev);
}

static inline struct idma_sw_desc *to_idma_sw_desc(struct virt_dma_desc *vdesc) {
    return container_of(vdesc, struct idma_sw_desc, vdesc);
}

static void idma_desc_free(struct virt_dma_desc *vdesc) {
    struct idma_sw_desc *desc = to_idma_sw_desc(vdesc);
    struct idma_dev *mdev = to_idma_dev(vdesc->tx.chan);

    if (desc->hw_vaddr) {
        dma_free_coherent(mdev->dma_dev.dev,
                          desc->desc_cnt * sizeof(struct idma_hw_desc),
                          desc->hw_vaddr, desc->hw_paddr);
    }
    kfree(desc);
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

    desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
    if (!desc) return NULL;

    desc->desc_cnt = 1;
    desc->hw_vaddr = dma_alloc_coherent(mdev->dma_dev.dev,
                                        sizeof(struct idma_hw_desc),
                                        &desc->hw_paddr, GFP_NOWAIT);
    if (!desc->hw_vaddr) {
        kfree(desc);
        return NULL;
    }

    desc->hw_vaddr[0].length = len;
    desc->hw_vaddr[0].flags  = IDMA_FLAGS_BASE | IDMA_SET_PROT(IDMA_PROT_MEM, IDMA_PROT_MEM);
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
    struct virt_dma_desc *vd;
    struct idma_sw_desc *desc;
    u64 status;

    spin_lock_irqsave(&mdev->vchan.lock, flags);

    if (vchan_issue_pending(&mdev->vchan)) {
        vd = vchan_next_desc(&mdev->vchan);
        if (vd) {
            desc = to_idma_sw_desc(vd);

            /* Hardware Safety: Check if descriptor FIFO is full before writing */
            status = readq(mdev->regs + IDMA_REG_STATUS);
            if (status & IDMA_STATUS_FIFO_FULL) {
                pr_warn("[iDMA][Backend] iDMA FIFO Full! Will drop descriptor!\n");
            }

            pr_info("[iDMA][Backend] Submitting DMA Chain (Physical Addr: %pad)\n", &desc->hw_paddr);

            /* Write physical address to start DMA transfer */
            writeq(desc->hw_paddr, mdev->regs + IDMA_REG_DESC_ADDR);

            // FIXME: Complete mock interrupt.
            /* * MOCK INTERRUPT: simulate immediate completion.
             * In real hardware, an IRQ handler will intercept the completion
             * triggered by IDMA_FLAG_IRQ_EN and call vchan_cookie_complete() there.
             */
            vchan_cookie_complete(vd);
        }
    }

    spin_unlock_irqrestore(&mdev->vchan.lock, flags);
}

static void idma_free_chan_resources(struct dma_chan *chan) {
    vchan_free_chan_resources(to_virt_chan(chan));
}

static int idma_probe(struct platform_device *pdev)
{
    // TODO: Error handling!
    struct idma_dev *mdev;
    struct dma_device *dma;
    int ret;

    pr_info("[iDMA][Backend] Begin iDMA probe.\n");

    // Set up interrupt.
    // FIXME: Get IRQ NO from device tree.
    ret = request_irq(IDMA_JOB_DONE_IRQ_NO, idma_irq_handler, IRQF_TRIGGER_RISING, "idma_done", NULL);
    if (ret) {
        pr_err("[iDMA] IRQ register failed!\n");
        return ret;
    }
    pr_info("[iDMA][Backend] IRQ registered!\n");
    
    // Alloc memory for data structures.
    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev) return -ENOMEM;
    pr_info("[iDMA][Backend] Allocated memory for device structures.\n");   
    
    /* Get mapped hardware registers */
    // FIXME: Get this from device tree.
    mdev->regs = devm_ioremap(&pdev->dev, 0xA0000000, 0x1000);

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

    dma->src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
    dma->dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_8_BYTES);
    dma->directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV) | BIT(DMA_MEM_TO_MEM);
    dma->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;

    INIT_LIST_HEAD(&dma->channels);
    pr_info("[iDMA][Backend] Initiated DMA channels.\n");

    vchan_init(&mdev->vchan, dma);
    mdev->vchan.desc_free = idma_desc_free;

    ret = dma_async_device_register(dma);
    if (ret) return ret;
    
    platform_set_drvdata(pdev, mdev);
    pr_info("[iDMA][Backend] Registered async DMA.\n");
    pr_info("[iDMA][Backend] iDMA Provider Loaded.\n");

    return 0;
}

static int idma_remove(struct platform_device *pdev) {
    struct idma_dev *mdev = platform_get_drvdata(pdev);
    dma_async_device_unregister(&mdev->dma_dev);
    //FIXME: free irq
    //FIXME: free other memory (?)
    return 0;
}

static struct platform_driver idma_driver = {
    .probe = idma_probe, .remove = idma_remove,
    .driver = { .name = DRIVER_NAME, },
};
static struct platform_device *pdev_fake;

static int __init idma_init(void) {
    int ret = platform_driver_register(&idma_driver);
    if (ret)
        return ret;

    pdev_fake = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
    if (IS_ERR(pdev_fake)) {
        platform_driver_unregister(&idma_driver);
    }

    return 0;
}

static void __exit idma_exit(void) {
    platform_device_unregister(pdev_fake);
    platform_driver_unregister(&idma_driver); 
}

module_init(idma_init); module_exit(idma_exit);
MODULE_LICENSE("GPL"); MODULE_DESCRIPTION("iDMA DMA Engine Provider");
