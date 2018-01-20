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

#include <sys/kernel_types.h>

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
    int req_pending;
    int req_ready;
    struct list_head rbd_allocated;
    struct list_head rbd_empty;
    IOSimpleLock *lock;
//    struct workqueue_struct *alloc_wq;
//    struct work_struct rx_alloc;
};

struct iwl_dma_ptr {
    dma_addr_t dma;
    void *addr;
    size_t size;
};


/**
 * iwl_queue_inc_wrap - increment queue index, wrap back to beginning
 * @index -- current index
 */
static inline int iwl_queue_inc_wrap(int index)
{
    return ++index & (TFD_QUEUE_SIZE_MAX - 1);
}

/**
 * iwl_queue_dec_wrap - decrement queue index, wrap back to end
 * @index -- current index
 */
static inline int iwl_queue_dec_wrap(int index)
{
    return --index & (TFD_QUEUE_SIZE_MAX - 1);
}

struct iwl_cmd_meta {
    /* only for SYNC commands, iff the reply skb is wanted */
    struct iwl_host_cmd *source;
    u32 flags;
    u32 tbs;
};


#define TFD_TX_CMD_SLOTS 256
#define TFD_CMD_SLOTS 32

/*
 * The FH will write back to the first TB only, so we need to copy some data
 * into the buffer regardless of whether it should be mapped or not.
 * This indicates how big the first TB must be to include the scratch buffer
 * and the assigned PN.
 * Since PN location is 8 bytes at offset 12, it's 20 now.
 * If we make it bigger then allocations will be bigger and copy slower, so
 * that's probably not useful.
 */
#define IWL_FIRST_TB_SIZE    20
#define IWL_FIRST_TB_SIZE_ALIGN ALIGN(IWL_FIRST_TB_SIZE, 64)

struct iwl_pcie_txq_entry {
    struct iwl_device_cmd *cmd;
    struct sk_buff *skb;
    /* buffer to free after command completes */
    const void *free_buf;
    struct iwl_cmd_meta meta;
};

struct iwl_pcie_first_tb_buf {
    u8 buf[IWL_FIRST_TB_SIZE_ALIGN];
};

/**
 * struct iwl_txq - Tx Queue for DMA
 * @q: generic Rx/Tx queue descriptor
 * @tfds: transmit frame descriptors (DMA memory)
 * @first_tb_bufs: start of command headers, including scratch buffers, for
 *    the writeback -- this is DMA memory and an array holding one buffer
 *    for each command on the queue
 * @first_tb_dma: DMA address for the first_tb_bufs start
 * @entries: transmit entries (driver state)
 * @lock: queue lock
 * @stuck_timer: timer that fires if queue gets stuck
 * @trans_pcie: pointer back to transport (for timer)
 * @need_update: indicates need to update read/write index
 * @ampdu: true if this queue is an ampdu queue for an specific RA/TID
 * @wd_timeout: queue watchdog timeout (jiffies) - per queue
 * @frozen: tx stuck queue timer is frozen
 * @frozen_expiry_remainder: remember how long until the timer fires
 * @bc_tbl: byte count table of the queue (relevant only for gen2 transport)
 * @write_ptr: 1-st empty entry (index) host_w
 * @read_ptr: last used entry (index) host_r
 * @dma_addr:  physical addr for BD's
 * @n_window: safe queue window
 * @id: queue id
 * @low_mark: low watermark, resume queue if free space more than this
 * @high_mark: high watermark, stop queue if free space less than this
 *
 * A Tx queue consists of circular buffer of BDs (a.k.a. TFDs, transmit frame
 * descriptors) and required locking structures.
 *
 * Note the difference between TFD_QUEUE_SIZE_MAX and n_window: the hardware
 * always assumes 256 descriptors, so TFD_QUEUE_SIZE_MAX is always 256 (unless
 * there might be HW changes in the future). For the normal TX
 * queues, n_window, which is the size of the software queue data
 * is also 256; however, for the command queue, n_window is only
 * 32 since we don't need so many commands pending. Since the HW
 * still uses 256 BDs for DMA though, TFD_QUEUE_SIZE_MAX stays 256.
 * This means that we end up with the following:
 *  HW entries: | 0 | ... | N * 32 | ... | N * 32 + 31 | ... | 255 |
 *  SW entries:           | 0      | ... | 31          |
 * where N is a number between 0 and 7. This means that the SW
 * data is a window overlayed over the HW queue.
 */
struct iwl_txq {
    void *tfds;
    struct iwl_pcie_first_tb_buf *first_tb_bufs;
    dma_addr_t first_tb_dma;
    struct iwl_pcie_txq_entry *entries;
    IOSimpleLock *lock;
    unsigned long frozen_expiry_remainder;
    //struct timer_list stuck_timer;
    struct iwl_trans_pcie *trans_pcie;
    bool need_update;
    bool frozen;
    bool ampdu;
    int block;
    unsigned long wd_timeout;
    
    mbuf_t overflow_q;
    struct iwl_dma_ptr bc_tbl;
    
    int write_ptr;
    int read_ptr;
    dma_addr_t dma_addr;
    int n_window;
    u32 id;
    int low_mark;
    int high_mark;
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
    u32 scd_base_addr;
    struct iwl_dma_ptr scd_bc_tbls;
    struct iwl_dma_ptr kw;
    
    struct iwl_txq *txq_memory;
    struct iwl_txq *txq[IWL_MAX_TVQM_QUEUES];
    unsigned long queue_used[BITS_TO_LONGS(IWL_MAX_TVQM_QUEUES)];
    unsigned long queue_stopped[BITS_TO_LONGS(IWL_MAX_TVQM_QUEUES)];
    
    /* PCI bus related data */
    volatile void* hw_base;
    
    bool ucode_write_complete;
    IOLock* ucode_write_waitq;
    IOLock* wait_command_queue;
    IOLock* d0i3_waitq;

    u8 page_offs, dev_cmd_offs;
    
    u8 cmd_queue;
    u8 cmd_fifo;
    unsigned int cmd_q_wdg_timeout;
    u8 n_no_reclaim_cmds;
    u8 no_reclaim_cmds[MAX_NO_RECLAIM_CMDS];
    u8 max_tbs;
    u16 tfd_size;

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
    bool ref_cmd_in_flight;
  
    bool msix_enabled;
    u8 shared_vec_mask;
    u32 alloc_vecs;
    u32 def_irq;
    u32 fh_init_mask;
    u32 hw_init_mask;
    u32 fh_mask;
    u32 hw_mask;
};

static inline bool iwl_queue_used(const struct iwl_txq *q, int i)
{
    return q->write_ptr >= q->read_ptr ?
    (i >= q->read_ptr && i < q->write_ptr) :
    !(i < q->read_ptr && i >= q->write_ptr);
}


#endif /* iwl_trans_pcie_h */
