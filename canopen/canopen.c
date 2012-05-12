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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <stdio.h> 
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdlib.h>

#include "canopen.h"

static SDO_abort_code_t SDO_abort_codes[] = {
    { 0x05030000, "Toggle bit not alternated" },
    { 0x05040000, "SDO protocol timed out" },
    { 0x05040001, "Client/Server command specifier not valid or unknown" },
    { 0x05040002, "Invalid block size (Block Transfer mode only)" },
    { 0x05040003, "Invalid sequence number (Block Transfer mode only)" },
    { 0x05030004, "CRC error (Block Transfer mode only)" },
    { 0x05030005, "Out of memory"},
    { 0x06010000, "Unsupported access to an object"},
    { 0x06010001, "Attempt to read a write-only object"},
    { 0x06010002, "Attempt to write a read-only object"},
    { 0x06020000, "Object does not exist in the Object Dictionary"},
    { 0x06040041, "Object can not be mapped to the PDO"},
    { 0x06040042, "The number and length of the objects to be mapped would exceed PDO length"},
    { 0x06040043, "General parameter incompatibility reason"},
    { 0x06040047, "General internal incompatibility in the device"},
    { 0x06060000, "Object access failed due to a hardware error"},
    { 0x06060010, "Data type does not match, lengh of service parameter does not match"},
    { 0x06060012, "Data type does not match, lengh of service parameter is too high"},
    { 0x06060013, "Data type does not match, lengh of service parameter is too low"},
    { 0x06090011, "Sub-index does not exist"},
    { 0x06090030, "Value range of parameter exceeded (only for write access)"},
    { 0x06090031, "Value of parameter written too high"},
    { 0x06090032, "Value of parameter written too low"},
    { 0x06090036, "Maximum value is less than minimum value"},
    { 0x08000000, "General error"},
    { 0x08000020, "Data can not be transferred or stored to the application"},
    { 0x08000021, "Data can not be transferred or stored to the application because of local control"},
    { 0x08000022, "Data can not be transferred or stored to the application because of the present device state"},
    { 0x08000023, "Object Dictionary dynamic generation fails or no Object Dictionary is present (e.g. OD is generated from file and generation fails because of a file error)"},
    {0x0000000, NULL}
};

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: int canopen_frame_parse(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
//SF    
//SF     Parse the CAN frame data payload as a CANopen packet.
//SF 
//------------------------------------------------------------------------------
int
canopen_frame_parse(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
{
    int i;

    if (canopen_frame == NULL || can_frame == NULL)
    {
        return -1;
    }

    bzero((void *)canopen_frame, sizeof(canopen_frame_t));

    //
    // Parse basic protocol fields
    //

    if (can_frame->can_id & CAN_EFF_FLAG)
    {
        canopen_frame->type = CANOPEN_FLAG_EXTENDED;
        canopen_frame->id   = can_frame->can_id & CAN_EFF_MASK;
    }
    else
    { 
        canopen_frame->type = CANOPEN_FLAG_STANDARD;
        canopen_frame->function_code = (can_frame->can_id & 0x00000780U) >> 7;
        canopen_frame->id            = (can_frame->can_id & 0x0000007FU);
    }

    canopen_frame->rtr = (can_frame->can_id & CAN_RTR_FLAG) ? 
                         CANOPEN_FLAG_RTR : CANOPEN_FLAG_NORMAL;
    
    canopen_frame->data_len = can_frame->can_dlc;
    for (i = 0; i < can_frame->can_dlc; i++)
    {
        canopen_frame->payload.data[i] = can_frame->data[i];
    }

    //
    // Parse payload data
    //

    // NMT protocol
    switch (canopen_frame->function_code)
    {
        // ---------------------------------------------------------------------
        // Network ManagemenT frame: Module Control
        // 
        case CANOPEN_FC_NMT_MC:
            

            break;

        // ---------------------------------------------------------------------
        // Network ManagemenT frame: Node Guarding
        // 
        case CANOPEN_FC_NMT_NG:


            break;

        //default:
            // unhandled type...
    }

    return 0;
}


//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: int canopen_frame_pack(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
//SF    
//SF     Pack a CANopen frame into a CAN frame that is ready to be transmitted
//SF     to the CAN network.
//SF     
//SF     * *canopen_frame*: a pointer to a CANopen frame data struct (input)
//SF     * *can_frame*:     a pointer to a CAN frame data struct (output)
//SF      
//------------------------------------------------------------------------------
int
canopen_frame_pack(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
{
    int i;

    if (canopen_frame == NULL || can_frame == NULL)
    {
        return -1;
    }

    can_frame->can_id = (canopen_frame->function_code<<7) | canopen_frame->id;

    if (canopen_frame->type == CANOPEN_FLAG_EXTENDED)
    {
        can_frame->can_id |= CAN_EFF_FLAG;  // set extended frame flag
    }

    if (canopen_frame->rtr == CANOPEN_FLAG_RTR)
    {
        can_frame->can_id |= CAN_RTR_FLAG;  // set RTR frame flag
    }
    
    can_frame->can_dlc = canopen_frame->data_len;
    for (i = 0; i < canopen_frame->data_len; i++)
    {
        can_frame->data[i] = canopen_frame->payload.data[i];
    }

    return 0;
}

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: int canopen_frame_dump_short(canopen_frame_t *frame)
//SF    
//SF     Print a CAN frame in a human-readable (compact) format to the
//SF     standard output.
//SF 
//------------------------------------------------------------------------------
int
canopen_frame_dump_short(canopen_frame_t *frame) // rename to analyze
{
    static int block_mode[256], bm_init = 0;
    int i;

    if (frame == NULL)
    {
        return -1;
    }

    if (bm_init == 0)
    {
        bzero((void *)block_mode, sizeof(block_mode));
        bm_init = 1;
    }
    
    if (frame->type == CANOPEN_FLAG_EXTENDED) // XXX flag!?!
    {
        printf("EXTENDED ");    
        printf("Node ID=0x%.7X ", frame->id);    
    }
    else
    {
        printf("STANDARD [0x%.3X] ", (frame->function_code<<7)|frame->id);    
        printf("FC=0x%.1X ", frame->function_code);
        printf("ID=0x%.2X ", frame->id);
    }

    if (frame->rtr)
    {
        printf("RTR ");
    }

    // raw payload data
    printf("[%u] ", frame->data_len);
    for (i = 0; i < frame->data_len; i++)
    {
        printf("0x%.2X ", frame->payload.data[i]);
    }

    printf(" => ");

    // NMT protocol
    switch (frame->function_code)
    {
        // ---------------------------------------------------------------------
        // Network ManagemenT frame: Module Control
        // 
        case CANOPEN_FC_NMT_MC:
            
            printf("NMT [Module Control] Node=0x%.2X ", frame->payload.nmt_mc.id);
            switch (frame->payload.nmt_mc.cs)
            {
                case CANOPEN_NMT_MC_CS_START:
                    printf("Command='%s'", CANOPEN_NMT_MC_CS_START_STR);
                    break;
                case CANOPEN_NMT_MC_CS_STOP:
                    printf("Command='%s'", CANOPEN_NMT_MC_CS_STOP_STR);
                    break;
                case CANOPEN_NMT_MC_CS_PREOP:
                    printf("Command='%s'", CANOPEN_NMT_MC_CS_PREOP_STR);
                    break;
                case CANOPEN_NMT_MC_CS_RESET_APP:
                    printf("Command='%s'", CANOPEN_NMT_MC_CS_RESET_APP_STR);
                    break;
                case CANOPEN_NMT_MC_CS_RESET_COM:
                    printf("Command='%s'", CANOPEN_NMT_MC_CS_RESET_COM_STR);
                    break;
                default:
                    printf("Command=Unknown");
            }

            break;

        // ---------------------------------------------------------------------
        // Network ManagemenT frame: Node Guarding
        // 
        case CANOPEN_FC_NMT_NG:

            if (frame->data_len == 0 && frame->rtr)
            {
                printf("NMT [Node Guarding] Node=0x%.2X Pull", frame->id);
                break;
            }

            printf("NMT [Node Guarding] Node=0x%.2X ", frame->id);
            switch (frame->payload.nmt_ng.state & CANOPEN_NMT_NG_STATE_MASK)
            {
                case CANOPEN_NMT_NG_STATE_BOOTUP:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_BOOTUP_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_DISCON:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_DISCON_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_CON:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_CON_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_PREP:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_PREP_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_STOP:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_STOP_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_OP:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_OP_STR);
                    break;
                case CANOPEN_NMT_NG_STATE_PREOP:
                    printf("State='%s'", CANOPEN_NMT_NG_STATE_PREOP_STR);
                    break;
                default:
                    printf("State=Unknown [%.2X]", frame->payload.nmt_ng.state & CANOPEN_NMT_NG_STATE_MASK);
            }
            break;

        // ---------------------------------------------------------------------
        // Emergancy / Sync
        // 
        case CANOPEN_FC_SYNC:
        //case CANOPEN_FC_EMERGENCY: // same as SYNC

            if (frame->id == 0x00)
            {
                printf("SYNC [counter = 0x%x] ", frame->payload.data[0]);
            }
            else
            {
                printf("EMERGANCY Node=0x%x ", frame->id);
            }

            break;

        // ---------------------------------------------------------------------
        // Time stamp
        // 
        case CANOPEN_FC_TIMESTAMP:

            printf("TIMESTAMP Node=0x%x ", frame->id);
            break;

        // ---------------------------------------------------------------------
        // PDOs
        // 
        case CANOPEN_FC_PDO1_TX:
            printf("PDO1 TX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO1_RX:
            printf("PDO1 RX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO2_TX:
            printf("PDO2 TX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO2_RX:
            printf("PDO2 RX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO3_TX:
            printf("PDO3 TX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO3_RX:
            printf("PDO3 RX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO4_TX:
            printf("PDO4 TX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;
        case CANOPEN_FC_PDO4_RX:
            printf("PDO4 RX Node=0x%x ", frame->id);
            for (i = 0; i < frame->data_len; i++)
                printf("0x%.2x ", frame->payload.data[i]);

            break;

        // ---------------------------------------------------------------------
        // SDOs
        // 
        case CANOPEN_FC_SDO_TX: // 58 : Server to Client = upload (server perspective)

            printf("SDO TX Node=0x%.2X ", frame->id);
               
            printf("Command=0x%.2X ", frame->payload.sdo.command);
/*
            if (block_mode[frame->id])
            {
                printf("[%s] [C %d] [SeqNo = %d] 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                       CANOPEN_SDO_CS_BD_STR, (frame->payload.data[0] & 0x80) ? 1 : 0,
                       frame->payload.data[0] & 0x7F,
                       frame->payload.data[1], frame->payload.data[2], frame->payload.data[3],
                       frame->payload.data[4], frame->payload.data[5], frame->payload.data[6],
                       frame->payload.data[7]);
                
                if (frame->payload.sdo.command & 0x80)
                {   
                    //block_mode[frame->id] = 0;
                }
                break;
            }
*/

            switch (frame->payload.sdo.command & CANOPEN_SDO_CS_MASK)
            {
                case CANOPEN_SDO_CS_TX_IDD:
                    printf("[%s] ", CANOPEN_SDO_CS_IDD_STR);
                    printf("Index=0x%.4X ",    SDO_index(frame->payload.sdo));
                    printf("SubIndex=0x%.2X ", frame->payload.sdo.subindex);
                    printf("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                           frame->payload.sdo.data[0], frame->payload.sdo.data[1],
                           frame->payload.sdo.data[2], frame->payload.sdo.data[3]);
                    break;
                case CANOPEN_SDO_CS_TX_DDS:
                    printf("[%s] ", CANOPEN_SDO_CS_DDS_STR);

                    printf("[T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);
                    break;
                case CANOPEN_SDO_CS_TX_IDU:
                    printf("[%s] ", CANOPEN_SDO_CS_IDU_STR);

                    if (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_E_FLAG)
                    {
                        printf("[EXP] ");

                        if (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_S_FLAG)
                        {
                            int n = (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_N_MASK) >> CANOPEN_SDO_CS_ID_N_SHIFT;
                            printf("[SI: %d] ", n);
                        }

                        printf("Index=0x%.4X ",    SDO_index(frame->payload.sdo));
                        printf("SubIndex=0x%.2X ", frame->payload.sdo.subindex);
                        printf("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                               frame->payload.sdo.data[0], frame->payload.sdo.data[1],
                               frame->payload.sdo.data[2], frame->payload.sdo.data[3]);
                    }
                    else
                    {
                        printf("[SEG] [SIZE %d]", frame->payload.sdo.data[0]);
                    }

                    break;
                case CANOPEN_SDO_CS_TX_UDS:
                    printf("[%s] ", CANOPEN_SDO_CS_UDS_STR);

                    if (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_C_FLAG)
                        printf("[LAST] [T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);
                    else
                        printf("[CONT] [T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);    

                    {
                        int n = (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_N_MASK) >> CANOPEN_SDO_CS_DS_N_SHIFT;
                        printf("[SI: %d] ", n);
                    }
                    break;
                case CANOPEN_SDO_CS_TX_ADT:
                    printf("[%s] ", CANOPEN_SDO_CS_ADT_STR);
                    printf("[%s] ", canopen_sdo_abort_code_lookup(&(frame->payload.sdo)));
                    break;
                case CANOPEN_SDO_CS_TX_BD:
                    printf("[%s] ", CANOPEN_SDO_CS_BD_STR);

                    switch (frame->payload.sdo.command & 0x03)
                    {
                        case 0x00: // IBD
                        {
                            printf("[S-CRC %d] ", (frame->payload.sdo.command &  CANOPEN_SDO_CS_BD_CRC_FLAG) ? 1 : 0);
                            printf("[SS %d] ", (frame->payload.sdo.command & 0x03));

                            printf("Index=0x%.4X ",   SDO_index(frame->payload.sdo));
                            printf("SubIndex=0x%.2X ",frame->payload.sdo.subindex);
                            printf("BlkSize=%d ", frame->payload.sdo.data[0]);   
                            block_mode[frame->id] = 1;
                            break;
                        }
                        case 0x01: // EBD
                        {
                            printf("[SS %d] ", (frame->payload.sdo.command & 0x03));
                            block_mode[frame->id] = 0;
                            break;
                        }
                        case 0x02: // BD ack
                        {
                            printf("[SS %d] ", (frame->payload.sdo.command & 0x03));
                            printf("AckSeq=%d ", frame->payload.data[1]);
                            printf("BlkSize=%d ", frame->payload.data[2]);
                            break;
                        }
                    }
                    break;
                default:
                    printf("[unknown cs] ");
            }
    
            break;

        case CANOPEN_FC_SDO_RX: // 60 : Client to Server = download [server perspective]

            printf("SDO RX Node=0x%.2X ", frame->id);

            printf("Command=0x%.2X ", frame->payload.sdo.command);

            if (block_mode[frame->id])
            {
                printf("[%s] [C %d] [SeqNo = %d] 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                       CANOPEN_SDO_CS_BD_STR, (frame->payload.data[0] & 0x80) ? 1 : 0,
                       frame->payload.data[0] & 0x7F,
                       frame->payload.data[1], frame->payload.data[2], frame->payload.data[3],
                       frame->payload.data[4], frame->payload.data[5], frame->payload.data[6],
                       frame->payload.data[7]);
                
                if (frame->payload.sdo.command & 0x80)
                {   
                    block_mode[frame->id] = 0;
                }
                break;
            }

            switch (frame->payload.sdo.command & CANOPEN_SDO_CS_MASK)
            {
                case CANOPEN_SDO_CS_RX_IDD:
                    printf("[%s] ", CANOPEN_SDO_CS_IDD_STR);

                    if (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_E_FLAG)
                    {
                        printf("[EXP] ");
                        if (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_S_FLAG)
                        {
                            int n = (frame->payload.sdo.command & CANOPEN_SDO_CS_ID_N_MASK) >> CANOPEN_SDO_CS_ID_N_SHIFT;
                            printf("[SI: %d] ", n);
                        } 
                    }
                    else
                    {
                        printf("[SEG] ");                        
                    }

                    printf("Index=0x%.4X ",   SDO_index(frame->payload.sdo));
                    printf("SubIndex=0x%.2X ",frame->payload.sdo.subindex);
                    printf("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                           frame->payload.sdo.data[0], frame->payload.sdo.data[1],
                           frame->payload.sdo.data[2], frame->payload.sdo.data[3]);

                    break;
                case CANOPEN_SDO_CS_RX_DDS:
                    printf("[%s] ", CANOPEN_SDO_CS_DDS_STR);

                    if (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_C_FLAG)
                        printf("[LAST] [T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);
                    else
                        printf("[CONT] [T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);

                    {
                        int n = (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_N_MASK) >> CANOPEN_SDO_CS_DS_N_SHIFT;
                        printf("[SI: %d] ", n);
                    }
                    break;
                case CANOPEN_SDO_CS_RX_IDU:
                    printf("[%s] ", CANOPEN_SDO_CS_IDU_STR);
                    printf("Index=0x%.4X ",   SDO_index(frame->payload.sdo));
                    printf("SubIndex=0x%.2X ",frame->payload.sdo.subindex);
                    printf("0x%.2X 0x%.2X 0x%.2X 0x%.2X ", 
                           frame->payload.sdo.data[0], frame->payload.sdo.data[1],
                           frame->payload.sdo.data[2], frame->payload.sdo.data[3]);
                    break;
                case CANOPEN_SDO_CS_RX_UDS:
                    printf("[%s] ", CANOPEN_SDO_CS_UDS_STR);

                    printf("[T %d] ", (frame->payload.sdo.command & CANOPEN_SDO_CS_DS_T_FLAG) ? 1 : 0);
                    break;
                case CANOPEN_SDO_CS_RX_ADT:
                    printf("[%s] ", CANOPEN_SDO_CS_ADT_STR);
                    printf("[%s] ", canopen_sdo_abort_code_lookup(&(frame->payload.sdo)));
                    block_mode[frame->id] = 0;
                    break;
                case CANOPEN_SDO_CS_RX_BD:
                    printf("[%s] ", CANOPEN_SDO_CS_BD_STR);

                    switch (frame->payload.sdo.command & 0x01)
                    {
                        case 0x00: // IBD
                        {
                            printf("[CS %d] ", (frame->payload.sdo.command & 0x01));

                            printf("[C-CRC %d] ", (frame->payload.sdo.command &  CANOPEN_SDO_CS_BD_CRC_FLAG) ? 1 : 0);
                            printf("[S %d] ",  (frame->payload.sdo.command &  CANOPEN_SDO_CS_BD_S_FLAG) ? 1 : 0);

                            printf("Index=0x%.4X ",   SDO_index(frame->payload.sdo));
                            printf("SubIndex=0x%.2X ",frame->payload.sdo.subindex);
                            printf("Size=%d ", 
                                   frame->payload.sdo.data[0]|(frame->payload.sdo.data[1]<<8)|
                                   (frame->payload.sdo.data[2]<<16)|(frame->payload.sdo.data[3]<<24));                    

                            //block_mode[frame->id] = 1;
                            break;
                        }
                        case 0x01: // EBD
                        {
                            printf("[CS %d] ", (frame->payload.sdo.command & 0x01));

                            printf("[EXCESS %d] ", (frame->payload.sdo.command >> CANOPEN_SDO_CS_DB_N_SHIFT) & CANOPEN_SDO_CS_DB_N_MASK);

                            printf("CRC=0x%.2X%.2X ", frame->payload.data[1], frame->payload.data[2]);

                            block_mode[frame->id] = 0;
                            break;
                        }
                    }
                    break;
                default:
                    printf("[unknown cs] ");
            }

            break;

        // ---------------------------------------------------------------------
        // default fallback: should never occur
        // 
        default:
            printf("Unknown frame");
    }
    printf("\n");

    return 0;
}

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: int canopen_frame_dump_verbose(canopen_frame_t *frame)
//SF    
//SF     Print a CAN frame in a human-readable (verbose) format to the
//SF     standard output.
//SF 
//------------------------------------------------------------------------------
int
canopen_frame_dump_verbose(canopen_frame_t *frame)
{
    int i;

    if (frame == NULL)
    {
        return -1;
    }

    printf("============================================\n");
    printf("Recieved CANopen frame:\n");

    if (frame->type == CANOPEN_FLAG_EXTENDED) // XXX flag!?!
    {
        printf("    - Frame format EXTENDED\n");    
        printf("    - Node ID = 0x%.7x\n", frame->id);    
    }
    else
    {
        printf("    - Frame format STANDARD\n");    
        printf("    - Function code = 0x%.1x ",  frame->function_code);
        printf("    - Node ID       = 0x%.3x\n", frame->id);
    }

    printf("    - Data length = %u\n", frame->data_len);
    printf("    - Data        =");

    for (i = 0; i < frame->data_len; i++)
    {
        printf(" 0x%.2x", frame->payload.data[i]);
    }

    printf("\n\n");
    return 0;
}

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: canopen_frame_t *canopen_frame_new()
//SF    
//SF     Allocate a new CANopen frame data struct.
//SF 
//------------------------------------------------------------------------------
canopen_frame_t *
canopen_frame_new()
{
    canopen_frame_t *frame;

    if ((frame = (canopen_frame_t *)malloc(sizeof(canopen_frame_t))) == NULL)
    {
        // error message here
        return NULL;
    }
    
    bzero((void *)frame, sizeof(canopen_frame_t));

    return frame;
}

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: void canopen_frame_free(canopen_frame_t *frame)
//SF    
//SF     Free the memory associated with the CANopen frame data struct.
//SF 
//------------------------------------------------------------------------------
void
canopen_frame_free(canopen_frame_t *frame)
{
    if (frame)
    {
        free(frame);
    }
}

//------------------------------------------------------------------------------
//SF 
//SF .. c:function:: int canopen_frame_set_nmt_mc(canopen_frame_t *frame, uint8_t cs, uint8_t node)
//SF    
//SF     Initiate a Network ManagemenT Module Control frame. 
//SF 
//SF     Arguments:
//SF         
//SF         frame (canopen_frame_t *): A pointer to a CANopen frame struct
//SF         in which the NMT MC frame with be prepared.
//SF         
//SF         cs (uint8_t): command specifyer, taking one of the following values
//SF           
//SF             * CANOPEN_NMT_MC_CS_START
//SF             * CANOPEN_NMT_MC_CS_STOP
//SF             * CANOPEN_NMT_MC_CS_PREOP
//SF             * CANOPEN_NMT_MC_CS_RESET_APP
//SF             * CANOPEN_NMT_MC_CS_RESET_COM
//SF     
//SF         node (uint8_t): node id for the target slave.
//SF     
//SF     Returns: 
//SF         
//SF         An integer indicating success (0) or failure (<0).
//SF     
//------------------------------------------------------------------------------
int
canopen_frame_set_nmt_mc(canopen_frame_t *frame, uint8_t cs, uint8_t node)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_NMT_MC;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = 0;

    frame->payload.nmt_mc.cs = cs;
    frame->payload.nmt_mc.id = node;

    frame->data_len = 2;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_nmt_ng(canopen_frame_t *frame, uint8_t node)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_RTR;
    frame->function_code = CANOPEN_FC_NMT_NG;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->data_len = 0;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_idu(canopen_frame_t *frame, 
                          uint8_t node, uint16_t index, uint8_t subindex)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_IDU;
    frame->payload.sdo.index_lsb = (index&0x00FF);
    frame->payload.sdo.index_msb = (index&0xFF00)>>8;
    frame->payload.sdo.subindex  = subindex;
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_idd(canopen_frame_t *frame, 
                          uint8_t node, uint16_t index, 
                          uint8_t subindex, uint32_t data, uint8_t len)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_IDD;
    frame->payload.sdo.command |= CANOPEN_SDO_CS_ID_E_FLAG;
    frame->payload.sdo.command |= CANOPEN_SDO_CS_ID_S_FLAG;
    frame->payload.sdo.command |= ((4-len)&0x03)<<CANOPEN_SDO_CS_ID_N_SHIFT;
    frame->payload.sdo.index_lsb = (index&0x00FF);
    frame->payload.sdo.index_msb = (index&0xFF00)>>8;
    frame->payload.sdo.subindex  = subindex;

    frame->payload.sdo.data[0] = (data&0x000000FF);
    frame->payload.sdo.data[1] = (data&0x0000FF00)>>8;
    frame->payload.sdo.data[2] = (data&0x00FF0000)>>16;
    frame->payload.sdo.data[3] = (data&0xFF000000)>>24;
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_idd_seg(canopen_frame_t *frame, 
                              uint8_t node, uint16_t index, 
                              uint8_t subindex, uint32_t data_size)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_IDD;
    frame->payload.sdo.index_lsb = (index&0x00FF);
    frame->payload.sdo.index_msb = (index&0xFF00)>>8;
    frame->payload.sdo.subindex  = subindex;

    if (data_size)
    {
        frame->payload.sdo.command |= CANOPEN_SDO_CS_ID_S_FLAG;
        frame->payload.sdo.data[0] = (data_size&0x000000FF);
        frame->payload.sdo.data[1] = (data_size&0x0000FF00)>>8;
        frame->payload.sdo.data[2] = (data_size&0x00FF0000)>>16;
        frame->payload.sdo.data[3] = (data_size&0xFF000000)>>24;
    }
    else
    {
        // we don't have to specify the size of the download
        frame->payload.sdo.data[0] = 0x00;
        frame->payload.sdo.data[1] = 0x00;
        frame->payload.sdo.data[2] = 0x00;
        frame->payload.sdo.data[3] = 0x00;
    }
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_uds(canopen_frame_t *frame, 
                          uint8_t node,     uint16_t index,
                          uint8_t subindex, uint8_t toggle)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_UDS;
    if (toggle)
        frame->payload.sdo.command |= CANOPEN_SDO_CS_DS_T_FLAG;

    frame->payload.sdo.index_lsb = (index&0x00FF);
    frame->payload.sdo.index_msb = (index&0xFF00)>>8;
    frame->payload.sdo.subindex  = subindex;

    frame->payload.sdo.data[0] = 0x00;
    frame->payload.sdo.data[1] = 0x00;
    frame->payload.sdo.data[2] = 0x00;
    frame->payload.sdo.data[3] = 0x00;
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_dds(canopen_frame_t *frame, uint8_t node, 
                                                  uint8_t *data,  uint8_t len, 
                                                  uint8_t toggle, uint8_t cont)
{
    int n;

    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_DDS;
    frame->payload.sdo.command |= ((7-len)&0x07)<<CANOPEN_SDO_CS_DS_N_SHIFT;
    //frame->payload.sdo.command |= ((7-len)<<CANOPEN_SDO_CS_DS_N_SHIFT)&CANOPEN_SDO_CS_DS_N_MASK;

    if (toggle)
        frame->payload.sdo.command |= CANOPEN_SDO_CS_DS_T_FLAG;

    if (cont)
        frame->payload.sdo.command |= CANOPEN_SDO_CS_DS_C_FLAG;

    for (n = 0; n < len && n < 7; n++)
        frame->payload.data[n+1] = data[n];
   
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_ibd(canopen_frame_t *frame, 
                          uint8_t node, uint16_t index, 
                          uint8_t subindex, uint32_t data_size)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.sdo.command = CANOPEN_SDO_CS_RX_BD | CANOPEN_SDO_CS_DB_CS_IBD;
    frame->payload.sdo.index_lsb = (index&0x00FF);
    frame->payload.sdo.index_msb = (index&0xFF00)>>8;
    frame->payload.sdo.subindex  = subindex;

    //frame->payload.sdo.command |= CANOPEN_SDO_CS_BD_CRC_FLAG; // use CRC

    if (data_size)
    {
        frame->payload.sdo.command |= CANOPEN_SDO_CS_BD_S_FLAG;
        frame->payload.sdo.data[0] = (data_size&0x000000FF);
        frame->payload.sdo.data[1] = (data_size&0x0000FF00)>>8;
        frame->payload.sdo.data[2] = (data_size&0x00FF0000)>>16;
        frame->payload.sdo.data[3] = (data_size&0xFF000000)>>24;
    }
    else
    {
        // we don't have to specify the size of the download
        frame->payload.sdo.data[0] = 0x00;
        frame->payload.sdo.data[1] = 0x00;
        frame->payload.sdo.data[2] = 0x00;
        frame->payload.sdo.data[3] = 0x00;
    }
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_ebd(canopen_frame_t *frame,
                          uint8_t node, uint8_t n, uint16_t crc)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->payload.data[0]  = CANOPEN_SDO_CS_RX_BD | CANOPEN_SDO_CS_DB_CS_EBD;
    frame->payload.data[0] |= (n & CANOPEN_SDO_CS_DB_N_MASK) << CANOPEN_SDO_CS_DB_N_SHIFT;

    frame->payload.data[1] = (crc&0x00FF);
    frame->payload.data[2] = (crc&0xFF00)>>8;

    frame->payload.data[3] = 0x00;
    frame->payload.data[4] = 0x00;
    frame->payload.data[5] = 0x00;
    frame->payload.data[6] = 0x00;
    frame->payload.data[7] = 0x00;
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_sdo_bd(canopen_frame_t *frame, 
                         uint8_t node, uint8_t *data,
                         uint8_t len,  uint8_t seqno, uint8_t cont)
{
    uint8_t i;

    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_NORMAL;
    frame->function_code = CANOPEN_FC_SDO_RX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    bzero((void *)(frame->payload.data), 8);

    frame->payload.data[0] = seqno;
    if (cont) // cont == 1 => this is the last segment of the block
        frame->payload.data[0] |= CANOPEN_SDO_CS_BD_C_FLAG;

    for (i = 0; i < len && i < 7; i++)
        frame->payload.data[i+1] = data[i];
    
    frame->data_len = 8;

    return 0;
}

//------------------------------------------------------------------------------
// frame-building functions
//
//------------------------------------------------------------------------------
int
canopen_frame_set_pdo_request(canopen_frame_t *frame, uint8_t node)
{
    if (frame == NULL)
        return -1;

    frame->rtr = CANOPEN_FLAG_RTR;
    frame->function_code = CANOPEN_FC_PDO1_TX;
    frame->type = CANOPEN_FLAG_STANDARD;    
    frame->id = node;

    frame->data_len = 0;

    return 0;
}


//------------------------------------------------------------------------------
// Look up the error description in the Abort Domain Transfer SDO frame.
//------------------------------------------------------------------------------
const char *
canopen_sdo_abort_code_lookup(canopen_sdo_t *sdo)
{
    int i;
    uint32_t code;

    if (sdo == NULL)
        return "SDO null";

    code = (sdo->data[3]<<24)|(sdo->data[2]<<16)|(sdo->data[1]<<8)|(sdo->data[0]);

    for (i = 0; SDO_abort_codes[i].code != 0; i++)
    {
        if (SDO_abort_codes[i].code == code)
        {
            return SDO_abort_codes[i].description;
        }
    }

    return "Unknown error";
}


//==============================================================================
// SDO parsing
//==============================================================================


//------------------------------------------------------------------------------
// Decode unsigned int
//------------------------------------------------------------------------------
int
canopen_sdo_get_size(canopen_sdo_t *sdo)
{
    int n = 0;

    switch (sdo->command & CANOPEN_SDO_CS_MASK)
    {
        //case CANOPEN_SDO_CS_TX_IDD:
        //    break;

        //case CANOPEN_SDO_CS_TX_DDS:
        //    break;

        case CANOPEN_SDO_CS_TX_IDU:
        case CANOPEN_SDO_CS_RX_IDD:            

            if ((sdo->command & CANOPEN_SDO_CS_ID_E_FLAG) &&
                (sdo->command & CANOPEN_SDO_CS_ID_S_FLAG))
            {
                n = 4 - ((sdo->command & CANOPEN_SDO_CS_ID_N_MASK) >> CANOPEN_SDO_CS_ID_N_SHIFT);
            }
            break;

        case CANOPEN_SDO_CS_TX_UDS:
        //case CANOPEN_SDO_CS_RX_DDS: // same as CANOPEN_SDO_CS_TX_UDS
            n = 4 - ((sdo->command & CANOPEN_SDO_CS_DS_N_MASK) >> CANOPEN_SDO_CS_DS_N_SHIFT);
            break;

        //case CANOPEN_SDO_CS_TX_ADT:
        //    break;

        //case CANOPEN_SDO_CS_TX_IBD:
        //   break;

        //case CANOPEN_SDO_CS_RX_UDS:
        //    break;

        //case CANOPEN_SDO_CS_RX_ADT:
        //    break;

        //case CANOPEN_SDO_CS_RX_IBD:
        //    break;
    }

    return n;
}

//==============================================================================
// Encoding and decoding of data
//==============================================================================


//------------------------------------------------------------------------------
// Decode unsigned int
//------------------------------------------------------------------------------
uint32_t
canopen_decode_uint(uint8_t *data, uint8_t len)
{
    uint8_t n;
    uint32_t value = 0;

    if (data == NULL)
        return value;

    for (n = 0; n < len; n++)
        value += data[n] << (8 * n);

    return value;
}

//------------------------------------------------------------------------------
// Encode unsigned int
//------------------------------------------------------------------------------
int
canopen_encode_uint(uint8_t *data, uint8_t len, uint32_t value)
{
    uint8_t n;

    if (data == NULL)
        return -1;

    for (n = 0; n < len; n++)
        data[n] = (value >> (8 * n)) & 0xFF;

    return 0;
}    


