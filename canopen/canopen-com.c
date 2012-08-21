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

#include <canopen.h> 
#include <canopen-com.h> 

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string.h>
#include <stdint.h> 
#include <stdio.h> 

static int canopen_com_debug = 0;

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_frame_send(int sock, canopen_frame_t *canopen_frame)
{
    int nbytes;
    struct can_frame can_frame;

    if (canopen_frame_pack(canopen_frame, &can_frame) != 0)
    {
        fprintf(stderr, "CANopen failed to parse frame\n");
        return 1;
    }

    // send the frame to the CAN bus
    nbytes = write(sock, &can_frame, sizeof(can_frame));

    if (nbytes < 0)
    {
        //perror("write: CAN raw socket write failed");
        return 1;
    }

    if (nbytes < (int)sizeof(struct can_frame))
    {
        fprintf(stderr, "write: incomplete CAN frame\n");
        return 1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_frame_recv(int sock, canopen_frame_t *canopen_frame)
{
    int nbytes;
    struct can_frame can_frame;

    // set filters for our node?

    nbytes = read(sock, &can_frame, sizeof(struct can_frame));

    if (nbytes < 0)
    {
        //perror("read: can raw socket read");
        return 1;
    }

    if (nbytes < (int)sizeof(struct can_frame))
    {
        fprintf(stderr, "read: incomplete CAN frame\n");
        return 1;
    }

    if (canopen_frame_parse(canopen_frame, &can_frame) != 0)
    {
        fprintf(stderr, "CANopen failed to parse frame\n");
    }

    return 0;
}

//==============================================================================
// EXPEDIATED TRANSFERS
//==============================================================================

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int 
canopen_sdo_upload_exp(int sock, uint8_t node, uint16_t index, 
                                 uint8_t subindex, uint32_t *data)
{
    int frame_count = 0;
    canopen_frame_t canopen_frame;

    if (canopen_com_debug)
        printf("DEBUG: Send SDO EXP UPLOAD from Node=0x%.2X Index=0x%.4X SubIndex=0x%.2X\n", node, index, subindex);

    canopen_frame_set_sdo_idu(&canopen_frame, node, index, subindex);

    if (canopen_frame_send(sock, &canopen_frame) != 0)
    {
        return 1;
    }

    if (can_filter_node_set(sock, node) < 0)
    {
        printf("%s: Error, failed to set CAN filters\n", __PRETTY_FUNCTION__);
    }

    while (frame_count < 10) // still needed?
    {
        if (canopen_frame_recv(sock, &canopen_frame) != 0)
        {
            return 1;
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {
            switch (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_MASK)
            {
                case CANOPEN_SDO_CS_TX_IDU:
                {
                    int s = canopen_sdo_get_size(&(canopen_frame.payload.sdo));
         
                    // matching our node id and we got a SDO TX: process package   
                    if (canopen_com_debug)
                        printf("REPLY [%d] (%d): 0x%.2X 0x%.2X 0x%.2X 0x%.2X \n", 
                               frame_count, s, canopen_frame.payload.sdo.data[0], canopen_frame.payload.sdo.data[1],
                               canopen_frame.payload.sdo.data[2], canopen_frame.payload.sdo.data[3]);

                    *data = canopen_decode_uint((uint8_t *)&(canopen_frame.payload.sdo.data), s);
                    return 0;
                }
                case CANOPEN_SDO_CS_TX_UDS:
                    fprintf(stderr, "SDO read error: Fallback to %s protocol\n",
                                    CANOPEN_SDO_CS_UDS_STR);
                    return 1;
                case CANOPEN_SDO_CS_TX_ADT:
                    fprintf(stderr, "SDO read error: %s [%s]\n",
                                    CANOPEN_SDO_CS_ADT_STR, 
                                    canopen_sdo_abort_code_lookup(&(canopen_frame.payload.sdo)));
                    return 1;
                default:
                    if (canopen_com_debug)
                        printf("[unknown cs] ");
            }           
        }
        // else: not our frame, read a new one
        frame_count++;
    }

    return 0;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_sdo_download_exp(int sock, uint8_t node,     uint16_t index, 
                                   uint8_t subindex, uint32_t data, uint16_t len)
{
    int frame_count = 0;
    canopen_frame_t canopen_frame;

    canopen_frame_set_sdo_idd(&canopen_frame, node, index, subindex, data, len);

    if (canopen_frame_send(sock, &canopen_frame) != 0)
    {
        return 1;
    }

    if (can_filter_node_set(sock, node) < 0)
    {
        printf("%s: Error, failed to set CAN filters\n", __PRETTY_FUNCTION__);
    }    

    while (frame_count < 1000)
    {
        if (canopen_frame_recv(sock, &canopen_frame) != 0)
        {
            return 1;
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {   
            if (canopen_com_debug)
                printf("DEBUG: GOT REPLY [%d]\n", frame_count);

            return 0;
        }

        frame_count++;
    }

    return 0;
}


//==============================================================================
// SEGMENTED
//==============================================================================

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_sdo_upload_seg(int sock, uint8_t node,  uint16_t index, uint8_t subindex,
                                 uint8_t *data, uint16_t data_len)
{
    int frame_count = 0, n = 0, sdo_data_len = 0, toggle = 0, offset = 0;
    canopen_frame_t canopen_frame;

    if (canopen_com_debug)
        printf("DEBUG: Send SEG SDO UPLOAD request to Node=0x%.2X Index=0x%.4X SubIndex=0x%.2X\n", node, index, subindex);

    canopen_frame_set_sdo_idu(&canopen_frame, node, index, subindex);

    if (canopen_frame_send(sock, &canopen_frame) != 0)
    {
        return -1;
    }

    if (can_filter_node_set(sock, node) < 0)
    {
        printf("%s: Error, failed to set CAN filters\n", __PRETTY_FUNCTION__);
    }   

    while (frame_count < 1000)
    {
        if (canopen_frame_recv(sock, &canopen_frame) != 0)
        {
            return -1;
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {
            switch (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_MASK)
            {
                case CANOPEN_SDO_CS_TX_IDU:
                {
                    if (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_ID_E_FLAG)
                    {
                        // expediated reply
                        int s = canopen_sdo_get_size(&(canopen_frame.payload.sdo));
         
                        if (canopen_com_debug)
                            printf("IDU EXP [%d] (%d): 0x%.2X 0x%.2X 0x%.2X 0x%.2X \n", 
                                   n, s, canopen_frame.payload.sdo.data[0], canopen_frame.payload.sdo.data[1],
                                         canopen_frame.payload.sdo.data[2], canopen_frame.payload.sdo.data[3]);

                        for (n = 0; n < s && n < data_len; n++)
                            data[n] = canopen_frame.payload.sdo.data[n];
                       
                        return n;
                    }
                    else
                    {
                        sdo_data_len = canopen_frame.payload.sdo.data[0];
                        // segmented IDU

                        if (canopen_com_debug)
                           printf("IDU SEG [%d] (%d): 0x%.2X 0x%.2X 0x%.2X 0x%.2X \n", 
                                  n, sdo_data_len,
                                  canopen_frame.payload.sdo.data[0],
                                  canopen_frame.payload.sdo.data[1],
                                  canopen_frame.payload.sdo.data[2],
                                  canopen_frame.payload.sdo.data[3]);

                        toggle = 0;
                        canopen_frame_set_sdo_uds(&canopen_frame, node, index, subindex, toggle);

                        if (canopen_frame_send(sock, &canopen_frame) != 0)
                        {
                            return -1;
                        }


                    }
                    break;
                }
                case CANOPEN_SDO_CS_TX_UDS:
                {
                    int s = (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_DS_N_MASK) >> CANOPEN_SDO_CS_DS_N_SHIFT;
    
                    if (canopen_com_debug)
                        printf("UDS [%d : %d] [%s]: 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X \n", 
                               n, s, (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_DS_C_FLAG) ? "LAST" : "CONT",
                               canopen_frame.payload.sdo.data[0], canopen_frame.payload.sdo.data[1],
                               canopen_frame.payload.sdo.data[2], canopen_frame.payload.sdo.data[3],
                               canopen_frame.payload.sdo.data[4], canopen_frame.payload.sdo.data[5],
                               canopen_frame.payload.sdo.data[6], canopen_frame.payload.sdo.data[7]);

                    for (n = 1; n < (8-s) && offset < data_len; n++)
                        data[offset++] = canopen_frame.payload.data[n];

                    if (!(canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_DS_C_FLAG))
                    {
                        toggle = ~toggle;
                        canopen_frame_set_sdo_uds(&canopen_frame, node, index, subindex, toggle);

                        if (canopen_frame_send(sock, &canopen_frame) != 0)
                        {
                            return -1;
                        }
                    }
                    else
                    {
                        // we finished
                        return offset;
                    }

                    break;
                }
                case CANOPEN_SDO_CS_TX_ADT:
                {
                    fprintf(stderr, "SDO read error: %s [%s]\n",
                                    CANOPEN_SDO_CS_ADT_STR, 
                                    canopen_sdo_abort_code_lookup(&(canopen_frame.payload.sdo)));
                    return -1;
                }
                default:
                    if (canopen_com_debug)
                        printf("[unknown cs = 0x%.2X]\n", canopen_frame.payload.sdo.command);
            }           
        }
        // else: not our frame, read a new one
        frame_count++;
    }

    return 0;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_sdo_download_seg(int sock, uint8_t node, uint16_t index, uint8_t subindex,
                                   uint8_t *data, uint16_t data_len)
{
    int frame_count = 0;
    int toggle = 0, offset = 0, remaining = data_len;
    canopen_frame_t canopen_frame;

    if (canopen_com_debug)
        printf("DEBUG: Send SEG SDO DOWNLOAD request to Node=0x%.2X Index=0x%.4X SubIndex=0x%.2X\n", node, index, subindex);

    canopen_frame_set_sdo_idd_seg(&canopen_frame, node, index, subindex, data_len);

    if (canopen_frame_send(sock, &canopen_frame) != 0)
    {
        return 1;
    }

    if (can_filter_node_set(sock, node) < 0)
    {
        printf("%s: Error, failed to set CAN filters\n", __PRETTY_FUNCTION__);
    }       

    while (frame_count < 1000)
    {
        if (canopen_frame_recv(sock, &canopen_frame) != 0)
        {
            return 1;
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {
            switch (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_MASK)
            {
                case CANOPEN_SDO_CS_TX_IDD:
                    // check toogle in incoming frame ?
                    if (canopen_com_debug)
                        printf("DEBUG: IDD reply\n");
                    toggle = 0;
                case CANOPEN_SDO_CS_TX_DDS:
                {
                    int i, n, c;

                    if (remaining == 0)
                    {
                        // we finished
                        return 0;
                    }

                    if (remaining <= 7)
                    {
                        n = remaining;
                        c = 1; // last frame
                    }
                    else
                    {
                        n = 7;
                        c = 0; // more frames to follow
                    }
                
                    canopen_frame_set_sdo_dds(&canopen_frame, node, &data[offset], n, toggle, c);


                    if (canopen_frame_send(sock, &canopen_frame) != 0)
                    {
                        return 1;
                    }

                    if (canopen_com_debug)
                    {
                        printf("DEBUG: DDS [%d : %d : %d] [%s]: ", 
                               data_len, n, remaining, 
                               (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_DS_C_FLAG) ? "LAST" : "CONT");
    
                        for (i = 0; i < n; i++)
                            printf("0x%.2X ", data[offset + i]);
    
                        printf("\n");
                    }

                    offset    += n;
                    remaining -= n;
                    toggle = ~toggle;

                    break;
                }
                case CANOPEN_SDO_CS_TX_ADT:
                {
                    fprintf(stderr, "SDO read error: %s [%s]\n",
                                    CANOPEN_SDO_CS_ADT_STR, 
                                    canopen_sdo_abort_code_lookup(&(canopen_frame.payload.sdo)));
                    return 1;
                }
                default:
                    if (canopen_com_debug)
                        printf("[unknown cs = 0x%.2X]\n", canopen_frame.payload.sdo.command);
            }           
        }
        // else: not our frame, read a new one
        frame_count++;
    }

    return 0;
}


//==============================================================================
// BLOCK TRANSFERS
//==============================================================================

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_sdo_upload_block(int sock, uint8_t node,  uint16_t index, uint8_t subindex,
                                   uint8_t *data, uint32_t data_len)
{
    return -1;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
canopen_sdo_download_block(int sock, uint8_t node, uint16_t index, uint8_t subindex,
                                   uint8_t *data, uint32_t data_len)
{
    int frame_count = 0;
    uint32_t offset = 0, remaining = data_len;
    int use_crc = 0, len = 0, cont, ack_seq; 
    int blk_size, blk_size_new, seq_no, seq_count = 0, blk_count = 0;
    uint8_t excess_bytes; 

    canopen_frame_t canopen_frame;

    if (canopen_com_debug)
        printf("DEBUG: SDO BLOCK DOWNLOAD request to Node=0x%.2X Index=0x%.4X SubIndex=0x%.2X [Size=%d]\n",
                node, index, subindex, data_len);

    canopen_frame_set_sdo_ibd(&canopen_frame, node, index, subindex, data_len);

    if (canopen_frame_send(sock, &canopen_frame) != 0)
    {
        return 1;
    }

    if (can_filter_node_set(sock, node) < 0)
    {
        printf("%s: Error, failed to set CAN filters\n", __PRETTY_FUNCTION__);
    }   

    while (frame_count < 1000)
    {
        if (canopen_frame_recv(sock, &canopen_frame) != 0)
        {
            return 1;
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {
            frame_count = 0;
            switch (canopen_frame.payload.sdo.command & (CANOPEN_SDO_CS_MASK|CANOPEN_SDO_CS_DB_SS_MASK))
            {
                case CANOPEN_SDO_CS_TX_BD|CANOPEN_SDO_CS_DB_SS_IBD_ACK:
                {
                    // we got an ack for a block download initiate
                    use_crc  = (canopen_frame.payload.sdo.command & CANOPEN_SDO_CS_BD_CRC_FLAG) ? 1 : 0;
                    blk_size = canopen_frame.payload.sdo.data[0];
                    if (canopen_com_debug)
                        printf("DEBUG: IBD reply [CRC = %d, block size = %d]\n", use_crc, blk_size);

                    //
                    // recalculate block size if we dont have enough data to 
                    // fill all blocks
                    //
                    /* dont worry about this
                    if (blk_size * 7 - remaining > 7)
                    {
                        // we can't fill all the blocks
                        blk_size_new = (int)(remaining / 7.0);
    
                        if (canopen_com_debug)
                            printf("DEBUG: IBD reassinged block size = %d from %d [len = %d < %d]\n",
                                   blk_size_new, blk_size, remaining, blk_size_new * 7);

                        blk_size = blk_size_new;
                    }
                    */

                    //
                    // send all segments in block no 1
                    //
                    for (seq_no = 1; seq_no <= blk_size; seq_no++)
                    {
                        //len  = (remaining > 7)          ? 7 : remaining;
                        //cont = remaining == 0 ? 1 : 0; //cont = (seq_no == blk_size) ? 1 : 0; 
                        cont = 0;
                        if (remaining > 7)
                        {
                            len = 7;
                        }
                        else
                        {
                            len = remaining;
                            cont = 1;
                            excess_bytes = 7 - remaining;
                        }

                        canopen_frame_set_sdo_bd(&canopen_frame, node, &data[offset], len, seq_no, cont);

                        if (canopen_com_debug)
                            printf("DEBUG: BD download [seq_no = %d, blk_size = %d, remaining = %d, offset = %d, cont = %d]\n",
                                   seq_no, blk_size, remaining, offset, cont);

                        if (canopen_frame_send(sock, &canopen_frame) != 0)
                            return 1;

                        remaining -= len;
                        offset    += len;

                        seq_count++;
                    }

                    blk_count++;

                    break;
                }
                case CANOPEN_SDO_CS_TX_BD|CANOPEN_SDO_CS_DB_SS_BD_ACK:
                {
                    // we got an ack for a block

                    ack_seq      = canopen_frame.payload.data[1];
                    blk_size_new = canopen_frame.payload.data[2];

                    if (canopen_com_debug)
                        printf("DEBUG: BD reply [ack seq = %d, block size = %d]\n", ack_seq, blk_size_new);

                    if (ack_seq != blk_size)
                    {
                        // not all segments was delivered properly, must resend
                        // seqments with seq_no > ack_seq
                        if (canopen_com_debug)
                           printf("DEBUG: Warning: Must resend segments with seq_no > %d [%d bytes]\n",
                                  ack_seq, 7 * (blk_size - ack_seq));


                        // rewind the appropriate amount
                        remaining += 7 * (blk_size - ack_seq); // XXX: might be wrong in the last seg ?
                        offset    -= 7 * (blk_size - ack_seq); // XXX: might be wrong in the last seg ?

                        // XXX: should the resent data be included in next block
                        // or should be resend part of the previous block ?!?
    
                        // assume that we can let the data to be resent be included
                        // in the next block...
                    }
                    
                    // move on to next block (possibly with rewinded data offsets)
                    blk_size = blk_size_new;

                    //
                    // if we have no data left, send the end of block download command
                    //
                    if (remaining <= 0)
                    {
                         uint16_t crc = 0;

                        //excess_bytes = blk_count * 7 - data_len;

                        if (use_crc)
                            crc = 0x0000; // canopen_sdo_block_crc(data, data_len);
    
                        if (canopen_com_debug)
                           printf("DEBUG: remaining = %d: sending EBD: blk_count = %d, seq_count = %d, "
                                  "offset = %d, data_len = %d: excess = %d, crc = 0x%.4X\n",
                                  remaining, blk_count, seq_count, offset, data_len, excess_bytes, crc);

                        canopen_frame_set_sdo_ebd(&canopen_frame, node, excess_bytes, crc);

                        if (canopen_frame_send(sock, &canopen_frame) != 0)
                            return 1;

                        break;
                    }

                    //
                    // recalculate block size if we dont have enough data to 
                    // fill all blocks
                    //
                    if ((signed long)blk_size * 7 - (signed long)remaining > 7)
                    {
                        printf("DEBUG: Alert: requiring BD reassinged block size = %d from %d [len = %d < %d] ??\n",
                               blk_size_new, blk_size, remaining, blk_size_new * 7);

                        // we can't fill all the blocks
                        //blk_size_new = (int)(remaining / 7.0);
    
                        //if (canopen_com_debug)
                        //    printf("DEBUG: BD reassinged block size = %d from %d [len = %d < %d]\n",
                        //           blk_size_new, blk_size, remaining, blk_size_new * 7);

                        //blk_size = blk_size_new;
                    }

                    //
                    // send all segments in block
                    //
                    for (seq_no = 1; seq_no <= blk_size; seq_no++)
                    {
                        //len  = (remaining > 7)      ? 7 : remaining;
                        //cont =  ? 1 : 0; //seq_no 0; //cont = (seq_no == blk_size) ? 1 : 0; 
                        cont = 0;
                        if (remaining > 7)
                        {
                            len = 7;
                        }
                        else
                        {
                            len = remaining;
                            cont = 1;
                            excess_bytes = 7 - remaining;
                        }

                        canopen_frame_set_sdo_bd(&canopen_frame, node, &data[offset], len, seq_no, cont);

                        if (canopen_com_debug)
                            printf("DEBUG: BD download [seq_no = %d, blk_size = %d, remaining = %d, offset = %d, cont = %d]\n",
                                   seq_no, blk_size, remaining, offset, cont);

                        if (canopen_frame_send(sock, &canopen_frame) != 0)
                            return 1;

                        remaining -= len;
                        offset    += len;
                        seq_count++;
                    }
                    blk_count++;

                    break;
                }
                case CANOPEN_SDO_CS_TX_BD|CANOPEN_SDO_CS_DB_SS_BD_END:
                {
                    if (canopen_com_debug)
                        printf("DEBUG: EDB ack received. Finished.\n");

                    return 0;
                }
                case CANOPEN_SDO_CS_TX_ADT:
                {
                    fprintf(stderr, "SDO read error: %s [%s]\n",
                                    CANOPEN_SDO_CS_ADT_STR, 
                                    canopen_sdo_abort_code_lookup(&(canopen_frame.payload.sdo)));
                    return 1;
                }
                default:
                    if (canopen_com_debug)
                        printf("[unknown cs = 0x%.2X]\n", canopen_frame.payload.sdo.command);
            }           
        }
        // else: not our frame, read a new one
        frame_count++;
    }

    printf("%s: Warning: frame count overflow\n", __PRETTY_FUNCTION__);
    return 0;
}

