/* add your code here */

#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOPacketQueue.h>



#include <net/if.h>

extern "C" {
#include "iwlwifi/iwl-csr.h"
#include "iwl-drv.h"
#include "iwl-trans.h"
#include "iwl-fh.h"
#include "iwl-prph.h"
#include "iwl-config.h"
#include "iwl-eeprom-parse.h"
#include "iwl-trans-pcie.h"
}


#include "Logging.h"

#include "IntelEeprom.hpp"




// Configuration
#define CONFIG_IWLMVM // Need NVM mode at least to see that code is compiling
#define CONFIG_IWLWIFI_PCIE_RTPM // Powerman

#ifdef DEBUG
#define CONFIG_IWLWIFI_DEBUG
#endif









#define    RELEASE(x)    if(x){(x)->release();(x)=NULL;}

class IntelWifi : public IOEthernetController
{
    OSDeclareDefaultStructors(IntelWifi)
    
public:
    virtual bool init(OSDictionary *properties) override;
    virtual void free() override;
    
    virtual bool start(IOService *provider) override;
    virtual void stop(IOService *provider) override;
    
    virtual bool configureInterface(IONetworkInterface* netif) override;
    
    virtual IOReturn enable(IONetworkInterface* netif) override;
    virtual IOReturn disable(IONetworkInterface* netif) override;
    virtual IOReturn getHardwareAddress(IOEthernetAddress* addrP) override;
    virtual IOReturn setHardwareAddress(const IOEthernetAddress* addrP) override;
    virtual UInt32 outputPacket(mbuf_t m, void* param) override;
    
    virtual IOReturn setPromiscuousMode(bool active) override
    {
        return kIOReturnSuccess;
    }
    
    virtual IOReturn setMulticastMode(bool active) override
    {
        return kIOReturnSuccess;
    }
    
    bool createMediumDict();
    
    virtual const OSString* newVendorString() const override;
    virtual const OSString* newModelString() const override;
    
protected:
    IOPCIDevice *pciDevice;
    IOEthernetInterface *netif;
    IOWorkLoop *fWorkLoop;
    OSDictionary *mediumDict;
    
    IONetworkStats *fNetworkStats;
    IOEthernetStats *fEthernetStats;
    IOInterruptEventSource* fInterruptSource;
    
    IntelIO* io;
    
    IntelEeprom* eeprom;
    
    IOMemoryMap *fMemoryMap;
    
    struct iwl_nvm_data *fNvmData;
    const struct iwl_cfg* fConfiguration;
    struct iwl_trans* fTrans;
    
    
private:
    inline void releaseAll() {
        RELEASE(fInterruptSource);
        RELEASE(fWorkLoop);
        RELEASE(mediumDict);
        RELEASE(eeprom);
        RELEASE(io);
        RELEASE(fMemoryMap);
        if (fTrans) {
            iwl_trans_pcie_free(fTrans);
            fTrans = NULL;
        }
        
        RELEASE(pciDevice);
    }
    
    static void interruptOccured(OSObject* owner, IOInterruptEventSource* sender, int count);
    static bool interruptFilter(OSObject* owner, IOFilterInterruptEventSource * src);
    
    int findMSIInterruptTypeIndex();
    
    
    // trans.c
    void _iwl_disable_interrupts(struct iwl_trans *trans);
    void iwl_disable_interrupts(struct iwl_trans *trans);
    struct iwl_trans* iwl_trans_pcie_alloc(const struct iwl_cfg *cfg);
    void iwl_trans_pcie_free(struct iwl_trans* trans);
    int _iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power);
    int iwl_trans_pcie_start_hw(struct iwl_trans *trans, bool low_power);
    void iwl_pcie_sw_reset(struct iwl_trans *trans);
    void iwl_enable_rfkill_int(struct iwl_trans *trans);
    void iwl_enable_hw_int_msk_msix(struct iwl_trans *trans, u32 msk);
    bool iwl_pcie_check_hw_rf_kill(struct iwl_trans *trans);
    bool iwl_is_rfkill_set(struct iwl_trans *trans);
    void iwl_trans_pcie_rf_kill(struct iwl_trans *trans, bool state);
    void iwl_pcie_apm_lp_xtal_enable(struct iwl_trans *trans);
    int iwl_pcie_apm_init(struct iwl_trans *trans);
    void iwl_pcie_apm_config(struct iwl_trans *trans);
    void iwl_pcie_apm_stop(struct iwl_trans *trans, bool op_mode_leave);
    void iwl_pcie_apm_stop_master(struct iwl_trans *trans);
    void iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power);
    void _iwl_trans_pcie_stop_device(struct iwl_trans *trans, bool low_power);
    int iwl_pcie_prepare_card_hw(struct iwl_trans *trans);
    int iwl_pcie_set_hw_ready(struct iwl_trans *trans);
    void iwl_enable_interrupts(struct iwl_trans *trans);
    void _iwl_enable_interrupts(struct iwl_trans *trans);
    int iwl_trans_pcie_start_fw(struct iwl_trans *trans, const struct fw_img *fw, bool run_in_rfkill);
    int iwl_pcie_nic_init(struct iwl_trans *trans);
    void iwl_enable_fw_load_int(struct iwl_trans *trans);
    int iwl_pcie_load_given_ucode(struct iwl_trans *trans, const struct fw_img *image);
    int iwl_pcie_load_given_ucode_8000(struct iwl_trans *trans, const struct fw_img *image);
    int iwl_pcie_load_cpu_sections(struct iwl_trans *trans,
                                              const struct fw_img *image,
                                              int cpu,
                                   int *first_ucode_section);
    void iwl_pcie_apply_destination(struct iwl_trans *trans);
    int iwl_pcie_load_section(struct iwl_trans *trans, u8 section_num,
                                         const struct fw_desc *section);
    void iwl_pcie_load_firmware_chunk_fh(struct iwl_trans *trans,
                                                    u32 dst_addr, dma_addr_t phy_addr,
                                                    u32 byte_cnt);
    int iwl_pcie_load_firmware_chunk(struct iwl_trans *trans,
                                                u32 dst_addr, dma_addr_t phy_addr,
                                                u32 byte_cnt);
    void iwl_trans_pcie_handle_stop_rfkill(struct iwl_trans *trans,
                                                      bool was_in_rfkill);
    void iwl_pcie_set_interrupt_capa(/*struct pci_dev *pdev,*/
                                     struct iwl_trans *trans);
    void iwl_pcie_conf_msix_hw(struct iwl_trans_pcie *trans_pcie);
    void iwl_pcie_map_non_rx_causes(struct iwl_trans *trans);
    void iwl_pcie_map_rx_causes(struct iwl_trans *trans);
    void iwl_pcie_init_msix(struct iwl_trans_pcie *trans_pcie);
    int iwl_pcie_load_cpu_sections_8000(struct iwl_trans *trans,
                                                   const struct fw_img *image,
                                                   int cpu,
                                                   int *first_ucode_section);
    
    // trans-gen2.c
    int iwl_pcie_gen2_apm_init(struct iwl_trans *trans);
    void iwl_pcie_gen2_apm_stop(struct iwl_trans *trans, bool op_mode_leave);
    void _iwl_trans_pcie_gen2_stop_device(struct iwl_trans *trans, bool low_power);
    void iwl_trans_pcie_gen2_stop_device(struct iwl_trans *trans, bool low_power);
    int iwl_pcie_gen2_nic_init(struct iwl_trans *trans);
    void iwl_trans_pcie_gen2_fw_alive(struct iwl_trans *trans, u32 scd_addr);
    int iwl_trans_pcie_gen2_start_fw(struct iwl_trans *trans,
                                     const struct fw_img *fw, bool run_in_rfkill);

    

    
    // rx.c
    int iwl_rxq_space(const struct iwl_rxq *rxq);
    __le32 iwl_pcie_dma_addr2rbd_ptr(dma_addr_t dma_addr);
    int iwl_pcie_rx_stop(struct iwl_trans *trans);
    void iwl_pcie_rxq_inc_wr_ptr(struct iwl_trans *trans, struct iwl_rxq *rxq);
    void iwl_pcie_rxq_check_wrptr(struct iwl_trans *trans);
    
    void iwl_pcie_rxmq_restock(struct iwl_trans *trans,
                                          struct iwl_rxq *rxq);
    void iwl_pcie_rxsq_restock(struct iwl_trans *trans,
                                          struct iwl_rxq *rxq);
    void iwl_pcie_rxq_restock(struct iwl_trans *trans, struct iwl_rxq *rxq);
    
    
    void iwl_pcie_enable_rx_wake(struct iwl_trans *trans, bool enable);
    void iwl_pcie_rx_mq_hw_init(struct iwl_trans *trans);
    
    int iwl_pcie_rx_alloc(struct iwl_trans *trans);
    int _iwl_pcie_rx_init(struct iwl_trans *trans);
    
    void iwl_pcie_rx_init_rxb_lists(struct iwl_rxq *rxq);
    
    int iwl_pcie_alloc_ict(struct iwl_trans *trans);
    void iwl_pcie_disable_ict(struct iwl_trans *trans);
    void iwl_pcie_free_ict(struct iwl_trans *trans);
    irqreturn_t iwl_pcie_irq_handler(int irq, void *dev_id);
    
    u32 iwl_pcie_int_cause_non_ict(struct iwl_trans *trans);
    void iwl_pcie_handle_rfkill_irq(struct iwl_trans *trans);
    void iwl_pcie_irq_handle_error(struct iwl_trans *trans);
    void iwl_pcie_reset_ict(struct iwl_trans *trans);
    u32 iwl_pcie_int_cause_ict(struct iwl_trans *trans);
    void iwl_pcie_rx_hw_init(struct iwl_trans *trans, struct iwl_rxq *rxq);
    int iwl_pcie_rx_init(struct iwl_trans *trans);
    
    // tx.c
    int iwl_queue_space(const struct iwl_txq *q);
    int iwl_queue_init(struct iwl_txq *q, int slots_num);
    int iwl_pcie_alloc_dma_ptr(struct iwl_trans *trans, struct iwl_dma_ptr *ptr, size_t size); // line 127
    void iwl_pcie_txq_inval_byte_cnt_tbl(struct iwl_trans *trans, struct iwl_txq *txq); // line 217
    void iwl_pcie_txq_inc_wr_ptr(struct iwl_trans *trans, struct iwl_txq *txq); // line 244
    void iwl_pcie_txq_check_wrptrs(struct iwl_trans *trans); // line 292
    dma_addr_t iwl_pcie_tfd_tb_get_addr(struct iwl_trans *trans, void *_tfd, u8 idx); // line 312
    void iwl_pcie_tfd_set_tb(struct iwl_trans *trans, void *tfd,
                             u8 idx, dma_addr_t addr, u16 len); // line 341
    u8 iwl_pcie_tfd_get_num_tbs(struct iwl_trans *trans, void *_tfd); // line 357
    int iwl_pcie_txq_alloc(struct iwl_trans *trans, struct iwl_txq *txq, int slots_num, bool cmd_queue); // line 487
    int iwl_pcie_txq_init(struct iwl_trans *trans, struct iwl_txq *txq, int slots_num, bool cmd_queue); // line 551
    void iwl_pcie_clear_cmd_in_flight(struct iwl_trans *trans); // line 593
    void iwl_pcie_tx_start(struct iwl_trans *trans, u32 scd_base_addr); // line 715
    void iwl_pcie_tx_stop_fh(struct iwl_trans *trans); // line 812
    int iwl_pcie_tx_alloc(struct iwl_trans *trans); // line 907
    int iwl_pcie_tx_init(struct iwl_trans *trans); // line 973
    void iwl_pcie_txq_progress(struct iwl_txq *txq); // line 1034
    int iwl_pcie_set_cmd_in_flight(struct iwl_trans *trans, const struct iwl_host_cmd *cmd); // line 1168
    void iwl_pcie_cmdq_reclaim(struct iwl_trans *trans, int txq_id, int idx); // line 1211
    int iwl_pcie_txq_set_ratid_map(struct iwl_trans *trans, u16 ra_tid, u16 txq_id); // line 1254
    bool iwl_trans_pcie_txq_enable(struct iwl_trans *trans, int txq_id, u16 ssn,
                                   const struct iwl_trans_txq_scd_cfg *cfg,
                                   unsigned int wdg_timeout); // line 1283
    void iwl_trans_pcie_txq_set_shared_mode(struct iwl_trans *trans, u32 txq_id,
                                            bool shared_mode);
    void iwl_trans_pcie_txq_disable(struct iwl_trans *trans, int txq_id, bool configure_scd); // line 1404
    int iwl_trans_pcie_tx(struct iwl_trans *trans, struct sk_buff *skb,
                          struct iwl_device_cmd *dev_cmd, int txq_id); // line 2256
    
    // SCD
    void iwl_scd_txq_set_chain(struct iwl_trans *trans,
                               u16 txq_id);
    
    
    void iwl_scd_txq_enable_agg(struct iwl_trans *trans,
                                u16 txq_id);
    
    void iwl_scd_txq_disable_agg(struct iwl_trans *trans,
                                 u16 txq_id);
    
    void iwl_scd_disable_agg(struct iwl_trans *trans);
    
    void iwl_scd_activate_fifos(struct iwl_trans *trans);
    
    void iwl_scd_deactivate_fifos(struct iwl_trans *trans);
    
    void iwl_scd_enable_set_active(struct iwl_trans *trans,
                                   u32 value);
    
    unsigned int SCD_QUEUE_WRPTR(unsigned int chnl);
    
    unsigned int SCD_QUEUE_RDPTR(unsigned int chnl);
    
    unsigned int SCD_QUEUE_STATUS_BITS(unsigned int chnl);
    
    void iwl_scd_txq_set_inactive(struct iwl_trans *trans,
                                  u16 txq_id);
    
    
    ifnet_t fIfNet;
    
    UInt16 fDeviceId;
    UInt16 fSubsystemId;
    
    
};

static inline struct iwl_trans_pcie *
IWL_TRANS_GET_PCIE_TRANS(struct iwl_trans *trans)
{
    return (struct iwl_trans_pcie *)trans->trans_specific;
}

