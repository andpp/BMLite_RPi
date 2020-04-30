#ifndef HCP_H
#define HCP_H

#include "fpc_bep_types.h"
#include "fpc_hcp_common.h"

/** MTU for HCP physical layer */
#define MTU 256

/** Communication acknowledge definition */
#define FPC_BEP_ACK 0x7f01ff7f


typedef struct {
    /** Send data to BM-Lite */
    fpc_bep_result_t (*write) (uint16_t, const uint8_t *, uint32_t, void *);  
    /** Receive data from BM-Lite */
    fpc_bep_result_t (*read)(uint16_t, uint8_t *, uint32_t, void *);
    /** Receive timeout (msec). Applys ONLY to receiving packet from BM-Lite on physical layer */
    uint32_t phy_rx_timeout;
    /** Data buffer for application layer */
    uint8_t *data_buffer;
    /** Size of data buffer */
    uint32_t data_size_max;
    /** Current size of incoming or outcoming command packet */
    uint32_t data_size;
    /** Buffer of MTU size for transport layer */
    uint8_t *txrx_buffer;
} HCP_comm_t;

fpc_bep_result_t send_command_args2(HCP_comm_t *chain, fpc_hcp_cmd_t command_id,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length);

fpc_bep_result_t receive_result_args2(HCP_comm_t *chain,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length);

#endif 