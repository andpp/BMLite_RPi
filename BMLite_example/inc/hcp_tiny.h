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
    uint8_t *pkt_buffer;
    /** Size of data buffer */
    uint32_t pkt_size_max;
    /** Current size of incoming or outcoming command packet */
    uint32_t pkt_size;
    /** Buffer of MTU size for transport layer */
    uint8_t *txrx_buffer;
} HCP_comm_t;

/**
 * @brief Helper function for sending HCP commands
 *
 * @param chain HCP communication chain
 * @param command_id command to send
 * @param arg_key1 first key to add to the command
 * @param arg_data1 first argument data to add
 * @param arg_data1_length first data length of argument data
 * @param arg_key2 second key to add to the command
 * @param arg_data2 second argument data to add
 * @param arg_data2_length second data length of argument data
 * @return ::fpc_bep_result_t
 */
fpc_bep_result_t send_command_args2(HCP_comm_t *chain, fpc_hcp_cmd_t command_id,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length);

/**
 * @brief Helper function for receiving HCP commands

 * @param command_id command to send
 * @param arg_key1 first key to receive
 * @param arg_data1 first argument data
 * @param arg_data1_length first argument data length
 * @param arg_key2 second key to receive
 * @param arg_data2 second argument data
 * @param arg_data2_length second argument
 * @return ::fpc_bep_result_t
 */
fpc_bep_result_t receive_result_args2(HCP_comm_t *chain,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length);

#endif 