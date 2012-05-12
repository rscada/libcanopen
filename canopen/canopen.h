//------------------------------------------------------------------------------
// Copyright (C) 2012, Robert Johansson, Raditex AB
// All rights reserved.
//
// This file is part of the rSCADA system.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

// 
// CANopen protocol parsing/packing routines and data types.
//

#ifndef _OPENCAN_H_
#define _OPENCAN_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string.h>
#include <stdint.h> 

//#include <canopen-com.h> 

//
// constants and protocol 
//
#define CANOPEN_FLAG_NORMAL 0x0
#define CANOPEN_FLAG_RTR    0x1

#define CANOPEN_FLAG_STANDARD 0x0
#define CANOPEN_FLAG_EXTENDED 0x1

//
// function codes (combined with the ID to give a CAN cob-id)
//
#define CANOPEN_FC_NMT_MC        0x00
#define CANOPEN_FC_EMERGENCY     0x01
#define CANOPEN_FC_SYNC          0x01
#define CANOPEN_FC_TIMESTAMP     0x02
#define CANOPEN_FC_PDO1_TX       0x03
#define CANOPEN_FC_PDO1_RX       0x04
#define CANOPEN_FC_PDO2_TX       0x05
#define CANOPEN_FC_PDO2_RX       0x06
#define CANOPEN_FC_PDO3_TX       0x07
#define CANOPEN_FC_PDO3_RX       0x08
#define CANOPEN_FC_PDO4_TX       0x09
#define CANOPEN_FC_PDO4_RX       0x0A
#define CANOPEN_FC_SDO_TX        0x0B
#define CANOPEN_FC_SDO_RX        0x0C
#define CANOPEN_FC_NMT_NG        0x0E

// XXX:  NG -> EC (Error Control) ??

//
// Network ManagemenT Module Control
//
#define CANOPEN_NMT_MC_CS_START          0x01
#define CANOPEN_NMT_MC_CS_START_STR      "Start remote node"
#define CANOPEN_NMT_MC_CS_STOP           0x02
#define CANOPEN_NMT_MC_CS_STOP_STR       "Stop remote node"
#define CANOPEN_NMT_MC_CS_PREOP          0x80
#define CANOPEN_NMT_MC_CS_PREOP_STR      "Enter pre-operation state"
#define CANOPEN_NMT_MC_CS_RESET_APP      0x81
#define CANOPEN_NMT_MC_CS_RESET_APP_STR  "Reset application"
#define CANOPEN_NMT_MC_CS_RESET_COM      0x82
#define CANOPEN_NMT_MC_CS_RESET_COM_STR  "Reset communication"
typedef struct _canopen_nmt_mc {
    uint8_t cs; // command specifier
    uint8_t id;
    uint8_t align[6];
} canopen_nmt_mc_t;

//
// Network ManagemenT Node Guard (and NMT Boot-up as a special case)
//
// XXX: use a table instead?
#define CANOPEN_NMT_NG_STATE_MASK            0x7F
#define CANOPEN_NMT_NG_STATE_BOOTUP          0x00
#define CANOPEN_NMT_NG_STATE_BOOTUP_STR      "Boot-up"
#define CANOPEN_NMT_NG_STATE_DISCON          0x01
#define CANOPEN_NMT_NG_STATE_DISCON_STR      "Disconnected"
#define CANOPEN_NMT_NG_STATE_CON             0x02
#define CANOPEN_NMT_NG_STATE_CON_STR         "Connecting"
#define CANOPEN_NMT_NG_STATE_PREP            0x03
#define CANOPEN_NMT_NG_STATE_PREP_STR        "Preparing"
#define CANOPEN_NMT_NG_STATE_STOP            0x04
#define CANOPEN_NMT_NG_STATE_STOP_STR        "Stopped"
#define CANOPEN_NMT_NG_STATE_OP              0x05
#define CANOPEN_NMT_NG_STATE_OP_STR          "Operational"
#define CANOPEN_NMT_NG_STATE_PREOP           0x7F
#define CANOPEN_NMT_NG_STATE_PREOP_STR       "Pre-operational"
typedef struct _canopen_nmt_ng {
    uint8_t state;
    uint8_t align[7];
} canopen_nmt_ng_t;

//
// This struct describes and SDO object (8-byte payload data in an CANopen frame)
//
#define CANOPEN_SDO_CS_MASK     0xE0
#define CANOPEN_SDO_CS_RX_IDD   0x20
#define CANOPEN_SDO_CS_TX_IDD   0x60
#define CANOPEN_SDO_CS_IDD_STR  "Initiate Domain Download"
#define CANOPEN_SDO_CS_RX_DDS   0x00
#define CANOPEN_SDO_CS_TX_DDS   0x20
#define CANOPEN_SDO_CS_DDS_STR  "Download Domain Segment"
#define CANOPEN_SDO_CS_RX_IDU   0x40
#define CANOPEN_SDO_CS_TX_IDU   0x40
#define CANOPEN_SDO_CS_IDU_STR  "Initiate Domain Upload"
#define CANOPEN_SDO_CS_RX_UDS   0x60
#define CANOPEN_SDO_CS_TX_UDS   0x00
#define CANOPEN_SDO_CS_UDS_STR  "Upload Domain Segment"
#define CANOPEN_SDO_CS_RX_ADT   0x80
#define CANOPEN_SDO_CS_TX_ADT   0x80
#define CANOPEN_SDO_CS_ADT_STR  "Abort Domain Transfer"
#define CANOPEN_SDO_CS_RX_BD    0xC0
#define CANOPEN_SDO_CS_TX_BD    0xA0
#define CANOPEN_SDO_CS_BD_STR  "Block Download"

//#define CANOPEN_SDO_CS_RX_IBD_   0xC0


// initiate download flags
#define CANOPEN_SDO_CS_ID_N_MASK    0x0C    
#define CANOPEN_SDO_CS_ID_N_SHIFT   0x02
#define CANOPEN_SDO_CS_ID_E_FLAG    0x02
#define CANOPEN_SDO_CS_ID_S_FLAG    0x01

// domain segment flags
#define CANOPEN_SDO_CS_DS_N_MASK    0x0E
#define CANOPEN_SDO_CS_DS_N_SHIFT   0x01
#define CANOPEN_SDO_CS_DS_C_FLAG    0x01
#define CANOPEN_SDO_CS_DS_T_FLAG    0x10

// block download flags
#define CANOPEN_SDO_CS_BD_S_FLAG   0x02
#define CANOPEN_SDO_CS_BD_CRC_FLAG 0x04
#define CANOPEN_SDO_CS_BD_C_FLAG   0x80

#define CANOPEN_SDO_CS_DB_CS_IBD   0x00
#define CANOPEN_SDO_CS_DB_CS_EBD   0x01

#define CANOPEN_SDO_CS_DB_N_MASK    0x07
#define CANOPEN_SDO_CS_DB_N_SHIFT   0x02

#define CANOPEN_SDO_CS_DB_SS_IBD_ACK 0x00
#define CANOPEN_SDO_CS_DB_SS_BD_ACK  0x02
#define CANOPEN_SDO_CS_DB_SS_BD_END  0x01
#define CANOPEN_SDO_CS_DB_SS_MASK    0x03

typedef struct _canopen_sdo {

    uint8_t command;
    uint8_t index_lsb;
    uint8_t index_msb;
    uint8_t subindex;
    uint8_t data[4];

} canopen_sdo_t;
#define SDO_index(sdo)  ((sdo.index_msb<<8)|(sdo.index_lsb))

//
//ST .. c:type:: canopen_frame_t
//ST    
//ST     Data structure that contains a parsed CANopen frame.
//ST 
//ST .. c:member:: uint32_t canopen_frame_t.id
//ST 
//ST     The ID part of the COB-ID in the CAN frame.    
//ST 
//ST .. c:member:: uint8_t canopen_frame_t.function_code
//ST 
//ST     The CANopen function code part of the COB-ID in the CAN frame.    
//ST 
typedef struct _canopen_frame {

    // basic
    uint8_t  rtr;           // CANOPEN_FLAG_RTR or CANOPEN_FLAG_NORMAL
    uint8_t  function_code; // 
    uint8_t  type;          // CANOPEN_FLAG_STANDARD or CANOPEN_FLAG_EXTENDED 
    uint32_t id;            // the 7 (STANDARD) or 29? (EXTENDED) bit ID field,
                            // not including the function code. 
    union {
        canopen_nmt_mc_t    nmt_mc;
        canopen_nmt_ng_t    nmt_ng;
        canopen_sdo_t       sdo;    
        uint8_t             data[8]; // raw data access
    } payload;

    uint8_t  data_len;

} canopen_frame_t;

//
// Error codes
//
typedef struct _SDO_abort_code {
      uint32_t code;
      char *description;
} SDO_abort_code_t;


const char *canopen_sdo_abort_code_lookup(canopen_sdo_t *sdo);


// protocol parsing and packing
int canopen_frame_parse(canopen_frame_t *canopen_frame, struct can_frame *can_frame);
int canopen_frame_pack(canopen_frame_t *canopen_frame, struct can_frame *can_frame);

// debug print-out to the standard output
int canopen_frame_dump_short(canopen_frame_t *frame);
int canopen_frame_dump_verbose(canopen_frame_t *frame);

//
// frame-building functions
//

// nmt
int canopen_frame_set_nmt_mc(canopen_frame_t *frame, uint8_t cs, uint8_t node);
int canopen_frame_set_nmt_ng(canopen_frame_t *frame, uint8_t node);

// sdo expediated
int canopen_frame_set_sdo_idu(canopen_frame_t *frame, uint8_t node, uint16_t index, uint8_t subindex);
int canopen_frame_set_sdo_idd(canopen_frame_t *frame, uint8_t node, uint16_t index, uint8_t subindex, uint32_t data, uint8_t len);

// sdo segmented
int canopen_frame_set_sdo_idd_seg(canopen_frame_t *frame, uint8_t node, uint16_t index, uint8_t subindex, uint32_t data_size);
int canopen_frame_set_sdo_uds(canopen_frame_t *frame, uint8_t node, uint16_t index, uint8_t subindex, uint8_t toggle);
int canopen_frame_set_sdo_dds(canopen_frame_t *frame, uint8_t node, uint8_t *data, uint8_t len, uint8_t toggle, uint8_t cont);

// sdo block
int canopen_frame_set_sdo_ibd(canopen_frame_t *frame, uint8_t node, uint16_t index, uint8_t subindex, uint32_t data_size);
int canopen_frame_set_sdo_bd(canopen_frame_t *frame, uint8_t node, uint8_t *data, uint8_t len, uint8_t seqno, uint8_t cont);
int canopen_frame_set_sdo_ebd(canopen_frame_t *frame, uint8_t node, uint8_t n, uint16_t crc);

// pdo
int canopen_frame_set_pdo_request(canopen_frame_t *frame, uint8_t node);

// sync
int canopen_frame_set_sync(canopen_frame_t *frame);

//
// memory management
//
canopen_frame_t *canopen_frame_new();
void             canopen_frame_free(canopen_frame_t *frame);



// SDO parsing
int canopen_sdo_get_size(canopen_sdo_t *);

// decode/encode

uint32_t canopen_decode_uint(uint8_t *data, uint8_t len);

#endif /* _OPENCAN_H_ */

