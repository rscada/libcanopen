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

#include <canopen/can-if.h>

int
can_socket_open(char *interface)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sock, bytes_read;

    /* Create the socket */
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return -1;
    }
 
    /* Locate the interface you wish to use */
    strcpy(ifr.ifr_name, interface);
    ioctl(sock, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled 
                                      * with that device's index */
                                     // XXX add check
 
    /* Select that CAN interface, and bind the socket to it. */
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // XXX Add check
 
    /* Send a message to the CAN bus * /
    frame.can_id = 0x123;
    strcpy(frame.data, "foo");
    frame.can_dlc = strlen(frame.data);
    int bytes_sent = write(sock, &frame, sizeof(frame));
    */
    
    return sock;
}


int
can_socket_close(int socket)
{
    return close(socket);
}
