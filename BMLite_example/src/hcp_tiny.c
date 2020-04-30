#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "bep_host_if.h"
#include "platform.h"
#include "fpc_crc.h"

#include "fpc_hcp_common.h"
#include "hcp_tiny.h"

#define DEBUG(...) printf(__VA_ARGS__)

static uint32_t fpc_com_ack = FPC_BEP_ACK;

static fpc_bep_result_t _rx_application(HCP_comm_t *hcp_comm);
static fpc_bep_result_t _rx_link(HCP_comm_t *hcp_comm);
static fpc_bep_result_t _tx_application(HCP_comm_t *hcp_comm);
static fpc_bep_result_t _tx_link(HCP_comm_t *hcp_comm);

static fpc_bep_result_t hcp_init_cmd(HCP_comm_t *hcp_comm, uint16_t cmd);
static fpc_bep_result_t hcp_add_arg(HCP_comm_t *hcp_comm, uint16_t arg, uint8_t *data, uint16_t size);
static fpc_bep_result_t hcp_get_arg(HCP_comm_t *hcp_comm, uint16_t arg_type, uint16_t *size, uint8_t **payload);

typedef struct {
    uint16_t cmd;
    uint16_t args_nr;
    uint8_t args[];
} _HCP_cmd_t;

typedef struct {
    uint16_t arg;
    uint16_t size;
    uint8_t pld[];
} _CMD_arg_t;

typedef struct {
    uint16_t lnk_chn;
    uint16_t lnk_size;
    uint16_t t_size;
    uint16_t t_seq_nr;
    uint16_t t_seq_len;
   _HCP_cmd_t t_pld;
} _HPC_pkt_t;

fpc_bep_result_t hcp_init_cmd(HCP_comm_t *hcp_comm, uint16_t cmd)
{
    _HCP_cmd_t *out = (_HCP_cmd_t *)hcp_comm->data_buffer;
    out->cmd = cmd;
    out->args_nr = 0;
    hcp_comm->data_size = 4;
    return FPC_BEP_RESULT_OK;
}

fpc_bep_result_t hcp_add_arg(HCP_comm_t *hcp_comm, uint16_t arg, uint8_t *data, uint16_t size)
{
    if(hcp_comm->data_size + 4 + size > hcp_comm->data_size_max) {
        return FPC_BEP_RESULT_NO_MEMORY;
    }

    ((_HCP_cmd_t *)hcp_comm->data_buffer)->args_nr++;
    _CMD_arg_t *args = (_CMD_arg_t *)(&hcp_comm->data_buffer[hcp_comm->data_size]);
    args->arg = arg;
    args->size = size;
    if(size) {
        memcpy(&args->pld, data, size);
    }
    hcp_comm->data_size += 4 + size;
    return FPC_BEP_RESULT_OK;
}

fpc_bep_result_t hcp_get_arg(HCP_comm_t *hcp_comm, uint16_t arg_type, uint16_t *size, uint8_t **payload)
{
    uint16_t i = 0;
    uint8_t *buffer = hcp_comm->data_buffer;
    uint16_t args_nr = ((_HCP_cmd_t *)(buffer))->args_nr;
    uint8_t *pdata = (uint8_t *)&((_HCP_cmd_t *)(buffer))->args;
    while (i < args_nr && (pdata - buffer) <= hcp_comm->data_size) {
        _CMD_arg_t *parg = (_CMD_arg_t *)pdata;
        if(parg->arg == arg_type) {
            *size = parg->size;
            *payload = parg->pld;
            return FPC_BEP_RESULT_OK;
        } else {
            i++;
            pdata += 4 + parg->size;
        }
    }
    return FPC_BEP_RESULT_INVALID_ARGUMENT;
}

static fpc_bep_result_t _rx_application(HCP_comm_t *hcp_comm)
{
        fpc_bep_result_t status = FPC_BEP_RESULT_OK;
        fpc_bep_result_t com_result = FPC_BEP_RESULT_OK;
        uint16_t seq_nr = 0;
        uint16_t seq_len = 1;
        uint16_t len;
        uint8_t *p = hcp_comm->data_buffer;
        _HPC_pkt_t *pkt = (_HPC_pkt_t *)hcp_comm->txrx_buffer;
        uint16_t buf_len = 0;

        while(seq_nr < seq_len) {
            status = _rx_link(hcp_comm);

            if (!status) {
                len = pkt->lnk_size - 4;
                seq_nr = pkt->t_seq_nr;
                seq_len = pkt->t_seq_len;
                if(buf_len + len < hcp_comm->data_size_max) {
                    memcpy(p, &pkt->t_pld, len);
                    p += len;
                    buf_len += len;
                } else {
                    com_result = FPC_BEP_RESULT_NO_MEMORY;
                }
                if (seq_len > 1)
                    DEBUG("S: Seqence %d of %d\n", seq_nr, seq_len);
            } else {
                DEBUG("S: Receiving chunk error %d\n", status);
                return status;
            }
        }
        hcp_comm->data_size = buf_len;
        return com_result;
}

static fpc_bep_result_t _rx_link(HCP_comm_t *hcp_comm)
{
        // Get size, msg and CRC
        uint16_t result = hcp_comm->read(4, hcp_comm->txrx_buffer, hcp_comm->phy_rx_timeout, NULL);
        _HPC_pkt_t *pkt = (_HPC_pkt_t *)hcp_comm->txrx_buffer;
        uint16_t size;

        if (result) {
            //DEBUG("Timed out waiting for response.\n");
            return result;
        }

        size = pkt->lnk_size;

        // Check if size plus header and crc is larger than max package size.
        if (MTU < size + 8) {
            DEBUG("S: Invalid size %d, larger than MTU %d.\n", size, MTU);
            return FPC_BEP_RESULT_IO_ERROR;
        }
        
        hcp_comm->read(size + 4, hcp_comm->txrx_buffer + 4, 100, NULL);
#ifdef DBG        
        // Print received data to log
        DEBUG("SRX: Received %d bytes\n", size + 4);
        DEBUG("SRX: Received data:");

        for (int i=0; i<size + 4; i++) {
            DEBUG(" 0x%02X", hcp_comm->txrx_buffer[i]);
        }
        DEBUG("\n");
#endif
        uint32_t crc = *(uint32_t *)(hcp_comm->txrx_buffer + 4 + size);
        uint32_t crc_calc = fpc_crc(0, hcp_comm->txrx_buffer+4, size);

        if (crc_calc != crc) {
            DEBUG("S: CRC mismatch. Calculated %08X, received %08X\n", crc_calc, crc);
            return FPC_BEP_RESULT_IO_ERROR;
        }

        // Send Ack
        hcp_comm->write(4, (uint8_t *)&fpc_com_ack, 0, NULL);

        return 0;
}

static fpc_bep_result_t _tx_application(HCP_comm_t *hcp_comm)
{
        uint16_t seq_nr = 1;
        fpc_bep_result_t status = FPC_BEP_RESULT_OK;
        uint16_t data_left = hcp_comm->data_size;
        uint8_t *p = hcp_comm->data_buffer;

        _HPC_pkt_t *pkt = (_HPC_pkt_t *)hcp_comm->txrx_buffer;

        // Application MTU size is PHY MTU - (Transport and Link overhead)
        uint16_t app_mtu = MTU - 6 - 8;

        // Calculate sequence length
        uint16_t seq_len = (data_left / app_mtu) + 1;

        pkt->lnk_chn = 0;
        pkt->t_seq_len = seq_len;

        for (seq_nr = 1; seq_nr <= seq_len && !status; seq_nr++) {
            pkt->t_seq_nr = seq_nr;
            if (data_left < app_mtu) {
                pkt->t_size = data_left;
                memcpy(hcp_comm->txrx_buffer + 10, p, data_left);
                pkt->lnk_size = data_left + 6;
            } else {
                pkt->t_size = app_mtu;
                memcpy(hcp_comm->txrx_buffer + 10, p, app_mtu);
                pkt->lnk_size = app_mtu + 6;
                p += app_mtu;
            }

            status = _tx_link(hcp_comm);
        }

        return status;
}

fpc_bep_result_t _tx_link(HCP_comm_t *hcp_comm)
{
    fpc_bep_result_t result;

    _HPC_pkt_t *pkt = (_HPC_pkt_t *)hcp_comm->txrx_buffer;

    uint32_t crc_calc = fpc_crc(0, &pkt->t_size, pkt->lnk_size);
    *(uint32_t *)(hcp_comm->txrx_buffer + 4 + pkt->lnk_size) = crc_calc;
    uint16_t size = pkt->lnk_size + 8;

    result = hcp_comm->write(size, hcp_comm->txrx_buffer, 0, NULL);
    if(result) {
        DEBUG("S: Sending error\n");
        return result;
    }

#ifdef DBG        
    // Print sent data to log
    DEBUG("STX: Sent %d bytes\n", size);
    DEBUG("STX: Sent data:    ");

    for (int i=0; i < size; i++) {
        DEBUG(" 0x%02X", hcp_comm->txrx_buffer[i]);
    }
    DEBUG("\n");
#endif
    // Wait for ACK
    uint32_t ack;
    result = hcp_comm->read(4, (uint8_t *)&ack, 100, NULL);
    if (result == FPC_BEP_RESULT_TIMEOUT) {
        DEBUG("S: ASK timeout\n");
        return FPC_BEP_RESULT_OK;
    }

    if(ack != fpc_com_ack) {
        return FPC_BEP_RESULT_IO_ERROR;
    }

    return FPC_BEP_RESULT_OK;
}

fpc_bep_result_t send_command_args2(HCP_comm_t *chain, fpc_hcp_cmd_t command_id,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length)
{
    fpc_bep_result_t bep_result;
    hcp_init_cmd(chain, command_id);
    
    if(arg_key1 != ARG_NONE) {
        bep_result = hcp_add_arg(chain, arg_key1, arg_data1, arg_data1_length);
        if(bep_result) {
            return bep_result;
        }
    }

    if(arg_key2 != ARG_NONE) {
        bep_result = hcp_add_arg(chain, arg_key2, arg_data2, arg_data2_length);
        if(bep_result) {
            return bep_result;
        }
    }

    bep_result = _tx_application(chain);

    if (bep_result != FPC_BEP_RESULT_OK) {
        DEBUG("%s:%u ERROR %d\n", __func__, __LINE__, bep_result);
    }

    return bep_result;
}

fpc_bep_result_t receive_result_args2(HCP_comm_t *chain,
        fpc_hcp_arg_t arg_key1, void *arg_data1, uint16_t arg_data1_length,
        fpc_hcp_arg_t arg_key2, void *arg_data2, uint16_t arg_data2_length)
{
        fpc_bep_result_t bep_result;
        uint16_t size;
        uint8_t *pld;

        
        bep_result = _rx_application(chain);
        if(bep_result) {
            DEBUG("S: Receive err: %d\n", bep_result);
            return bep_result;
        }

        if (arg_key1 != ARG_NONE) {
            bep_result = hcp_get_arg(chain, arg_key1, &size, &pld);
            if(bep_result == FPC_BEP_RESULT_OK) {
                if(arg_data1 == NULL) {
                    return FPC_BEP_RESULT_NO_MEMORY;
                }
                memcpy(arg_data1, pld, arg_data1_length);
            } else {
                DEBUG("Arg1 0x%04X not found\n", arg_key1);
                return FPC_BEP_RESULT_INVALID_ARGUMENT;
            }
        }

        if (arg_key2 != ARG_NONE) {
            bep_result = hcp_get_arg(chain, arg_key2, &size, &pld);
            if(bep_result == FPC_BEP_RESULT_OK) {
                if(arg_data2 == NULL) {
                    return FPC_BEP_RESULT_NO_MEMORY;
                }
                memcpy(arg_data2, pld, arg_data2_length);
            } else {
                DEBUG("Arg2 0x%04X not found\n", arg_key2);
                //return FPC_BEP_RESULT_INVALID_ARGUMENT;
            }
        }

    return FPC_BEP_RESULT_OK;

}


