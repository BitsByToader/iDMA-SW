#include "stdint.h"
#include "xparameters.h"
#include "xil_cache.h"

#define DEBUG

#define IDMA_REG_BASE_ADDR 0xA0000000
volatile uint64_t *idma_reg_descr_addr = (uint64_t*) (IDMA_REG_BASE_ADDR);

typedef struct {
    uint64_t flags;
    uint64_t next_descr_ptr;
    uint64_t src_ptr;
    uint64_t dst_ptr;
} idma_descr;

// AXI spec implies slaves (thus base acceseses to data) are 4kb aligned and addr space is multiple of 4kb.
// AXI spec *mandates* that transfers don't cross a 4kb boundary, so that a burst doesn't access multiple slaves.
// So align data by 4kb.
__attribute__((aligned(4096))) volatile idma_descr tx_desc;
__attribute__((aligned(4096))) volatile idma_descr rx_desc;
__attribute__((aligned(4096))) volatile uint64_t src_data[16];
__attribute__((aligned(4096))) volatile uint64_t dst_data[16];

int main() {
    volatile idma_descr *tx_desc_ptr = &tx_desc;
    volatile idma_descr *rx_desc_ptr = &rx_desc;
    uint64_t first_descr_addr = (uint64_t) tx_desc_ptr;

    tx_desc_ptr->flags = 0x2800006B00000080;
    tx_desc_ptr->next_descr_ptr = (uint64_t) rx_desc_ptr;
    tx_desc_ptr->src_ptr = (uint64_t) src_data;
    tx_desc_ptr->dst_ptr = 0; // dst is AXIS so leave empty

    rx_desc_ptr->flags = 0x0500006B00000080;
    rx_desc_ptr->next_descr_ptr = 0xFFFFFFFFFFFFFFFF; // this descriptor is the last one
    rx_desc_ptr->src_ptr = 0; // leave empty since rxsrc is AXIS so leave empty
    rx_desc_ptr->dst_ptr = (uint64_t) dst_data;

    for (int i = 0; i < 16; i++) {
        src_data[i] = i+1;
        dst_data[i] = 0xdeadbeef;
    }

    Xil_DCacheFlushRange((UINTPTR)src_data, 16*sizeof(uint64_t));
    Xil_DCacheFlushRange((UINTPTR)dst_data, 16*sizeof(uint64_t));
    Xil_DCacheFlushRange((UINTPTR)tx_desc_ptr, 4*sizeof(uint64_t));
    Xil_DCacheFlushRange((UINTPTR)rx_desc_ptr, 4*sizeof(uint64_t));
    
    *idma_reg_descr_addr = first_descr_addr;

    for (uint64_t i = 0; i < 1000; i++) {
        ;
    }
    
    Xil_DCacheFlushRange((UINTPTR)dst_data, 16*sizeof(uint64_t));

    while(1);

    return 0;
}