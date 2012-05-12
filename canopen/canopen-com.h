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
// CANopen communication routines and data types.
//

#ifndef _OPENCAN_COM_H_
#define _OPENCAN_COM_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string.h>
#include <stdint.h> 

#include "canopen.h"

int canopen_sdo_upload_exp(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint32_t *data);
int canopen_sdo_download_exp(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint32_t data, uint16_t len);

int canopen_sdo_upload_seg(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint8_t *data, uint16_t len);
int canopen_sdo_download_seg(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint8_t *data, uint16_t len);

int canopen_sdo_upload_block(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint8_t *data, uint32_t len);
int canopen_sdo_download_block(int sock, uint8_t node, uint16_t index, uint8_t subindex, uint8_t *data, uint32_t len);

int canopen_frame_send(int sock, canopen_frame_t *canopen_frame);
int canopen_frame_recv(int sock, canopen_frame_t *canopen_frame);

#endif /* _OPENCAN_COM_H */
