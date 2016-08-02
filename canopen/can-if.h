//------------------------------------------------------------------------------
// Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex Control AB
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

#ifndef _CAN_IF_H_
#define _CAN_IF_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string.h>
#include <stdint.h> 

int can_socket_open(char *interface);
int can_socket_open_timeout(char *interface, unsigned int timeout_sec);
int can_socket_close(int socket);

int can_filter_node_set(int socket, uint8_t node);
int can_filter_clear(int socket);

#endif /* _CAN_IF_H */
