//
//  iwl-trans-pcie.h
//  This file contains ported structure iwl_trans_pcie from original linux driver.
//  All linux specific things were changed to macos specific.
//  Some fields are redundand here, because those things will be handled in the main driver class IntelWifi
//
//  Created by Roman Peshkov on 29/12/2017.
//  Copyright © 2017 Roman Peshkov. All rights reserved.
//

#ifndef iwl_trans_pcie_h
#define iwl_trans_pcie_h

#include <linux/types.h>

extern "C" {
#include "iwl-modparams.h"
#include "iwl-fh.h"
}


/* We need 2 entries for the TX command and header, and another one might
 * be needed for potential data in the SKB's head. The remaining ones can
 * be used for frags.
 */
#define IWL_PCIE_MAX_FRAGS(x) (x->max_tbs - 3)

/*
 * RX related structures and functions
 */
#define RX_NUM_QUEUES 1
#define RX_POST_REQ_ALLOC 2
#define RX_CLAIM_REQ_ALLOC 8
#define RX_PENDING_WATERMARK 16


/**
 * enum iwl_shared_irq_flags - level of sharing for irq
 * @IWL_SHARED_IRQ_NON_RX: interrupt vector serves non rx causes.
 * @IWL_SHARED_IRQ_FIRST_RSS: interrupt vector serves first RSS queue.
 */
enum iwl_shared_irq_flags {
    IWL_SHARED_IRQ_NON_RX        = BIT(0),
    IWL_SHARED_IRQ_FIRST_RSS    = BIT(1),
};

/*This file includes the declaration that are internal to the
 * trans_pcie layer */

/**
 * struct iwl_rx_mem_buffer
 * @page_dma: bus address of rxb page
 * @page: driver's pointer to the rxb page
 * @invalid: rxb is in driver ownership - not owned by HW
 * @vid: index of this rxb in the global table
 */
struct iwl_rx_mem_buffer {
    dma_addr_t page_dma;
    //struct page *page;
    u16 vid;
    bool invalid;
    struct list_head list;
};

/**
 * struct isr_statistics - interrupt statistics
 *
 */
struct isr_statistics {
    u32 hw;
    u32 sw;
    u32 err_code;
    u32 sch;
    u32 alive;
    u32 rfkill;
    u32 ctkill;
    u32 wakeup;
    u32 rx;
    u32 tx;
    u32 unhandled;
};

/**
 * struct iwl_rxq - Rx queue
 * @id: queue index
 * @bd: driver's pointer to buffer of receive buffer descriptors (rbd).
 *    Address size is 32 bit in pre-9000 devices and 64 bit in 9000 devices.
 * @bd_dma: bus address of buffer of receive buffer descriptors (rbd)
 * @ubd: driver's pointer to buffer of used receive buffer descriptors (rbd)
 * @ubd_dma: physical address of buffer of used receive buffer descriptors (rbd)
 * @read: Shared index to newest available Rx buffer
 * @write: Shared index to oldest written Rx packet
 * @free_count: Number of pre-allocated buffers in rx_free
 * @used_count: Number of RBDs handled to allocator to use for allocation
 * @write_actual:
 * @rx_free: list of RBDs with allocated RB ready for use
 * @rx_used: list of RBDs with no RB attached
 * @need_update: flag to indicate we need to update read/write index
 * @rb_stts: driver's pointer to receive buffer status
 * @rb_stts_dma: bus address of receive buffer status
 * @lock:
 * @queue: actual rx queue. Not used for multi-rx queue.
 *
 * NOTE:  rx_free and rx_used are used as a FIFO for iwl_rx_mem_buffers
 */
struct iwl_rxq {
    int id;
    void *bd;
    dma_addr_t bd_dma;
    __le32 *used_bd;
    dma_addr_t used_bd_dma;
    u32 read;
    u32 write;
    u32 free_count;
    u32 used_count;
    u32 write_actual;
    u32 queue_size;
    struct list_head rx_free;
    struct list_head rx_used;
    bool need_update;
    struct iwl_rb_status *rb_stts;
    dma_addr_t rb_stts_dma;
    IOSimpleLock *lock;
    //struct napi_struct napi;
    struct iwl_rx_mem_buffer *queue[RX_QUEUE_SIZE];
};

/**
* struct iwl_rb_allocator - Rx allocator
* @req_pending: number of requests the allcator had not processed yet
* @req_ready: number of requests honored and ready for claiming
* @rbd_allocated: RBDs with pages allocated and ready to be handled to
*    the queue. This is a list of &struct iwl_rx_mem_buffer
* @rbd_empty: RBDs with no page attached for allocator use. This is a list
*    of &struct iwl_rx_mem_buffer
* @lock: protects the rbd_allocated and rbd_empty lists
* @alloc_wq: work queue for background calls
* @rx_alloc: work struct for background calls
*/
struct iwl_rb_allocator {
//    atomic_t req_pending;
//    atomic_t req_ready;
    int req_pending;
    int req_ready;
    struct list_head rbd_allocated;
    struct list_head rbd_empty;
    IOSimpleLock *lock;
//    struct workqueue_struct *alloc_wq;
//    struct work_struct rx_alloc;
};





struct iwl_trans_pcie {
    struct iwl_rxq *rxq;
    struct iwl_rx_mem_buffer rx_pool[RX_POOL_SIZE];
    struct iwl_rx_mem_buffer *global_table[RX_POOL_SIZE];
    struct iwl_rb_allocator rba;
    struct iwl_trans *trans;
    
    /* INT ICT Table */
    __le32 *ict_tbl;
    dma_addr_t ict_tbl_dma;
    int ict_index;
    bool use_ict;
    bool is_down, opmode_down;
    bool debug_rfkill;
    struct isr_statistics isr_stats;
    
    IOSimpleLock* irq_lock;
    IOLock *mutex;
    u32 inta_mask;
    
    /* PCI bus related data */
    volatile void* hw_base;
    
    bool ucode_write_complete;
    IOLock* ucode_write_waitq;
//    wait_queue_t wait_command_queue;
//    wait_queue_t d0i3_waitq;


    
    UInt8 max_tbs;
    UInt16 tfd_size;
    UInt8 max_skb_frags;
    UInt32 hw_rev;
    
    
    enum iwl_amsdu_size rx_buf_size;
    bool bc_table_dword;
    bool scd_set_active;
    bool sw_csum_tx;
    u32 rx_page_order;

    /*protect hw register */
    
    IOSimpleLock* reg_lock;
    bool cmd_hold_nic_awake;
    
    
    
    bool msix_enabled;
    u8 shared_vec_mask;
    u32 alloc_vecs;
    u32 def_irq;
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
};

/*static inline struct iwl_trans_pcie* iwl_trans_pcie_alloc() {
    struct iwl_trans_pcie* trans = (struct iwl_trans_pcie *)IOMalloc(sizeof(struct iwl_trans_pcie));
    trans->reg_lock = IOSimpleLockAlloc();
    trans->mutex = IOLockAlloc();
    return trans;
}

static inline void iwl_trans_pcie_free(struct iwl_trans_pcie *trans) {
    IOLockFree(trans->mutex);
    IOSimpleLockFree(trans->reg_lock);
    IOFree(trans, sizeof(struct iwl_trans_pcie));
}*/

#endif /* iwl_trans_pcie_h */
