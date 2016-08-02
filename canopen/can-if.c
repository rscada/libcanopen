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
 // Call the main function, with a default timeout
 // of 1s.
 return can_socket_open_timeout(interface, 1);
}

int
can_socket_open_timeout(char *interface, unsigned int timeout_sec)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct timeval tv= {timeout_sec,0};
    int sock, bytes_read;

    // create CAN socket
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return -1;
    }
 
    // set a timeoute for read
    if (timeout_sec != 0)
    {
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    }

    // bind socket to the given interface
    strcpy(ifr.ifr_name, interface);
    ioctl(sock, SIOCGIFINDEX, &ifr);// XXX add check
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // XXX Add check
 
    return sock;
}


int
can_socket_close(int socket)
{
    return close(socket);
}

int
can_filter_node_set(int sock, uint8_t node)
{
    struct can_filter rfilter[1];

    rfilter[0].can_id   = node;
    rfilter[0].can_mask = 0x00;

    // seems not to work to chance filters after bind
    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    return 0;
}


int
can_filter_clear(int sock)
{
    struct can_filter rfilter[1];

    rfilter[0].can_id   = 0x00;
    rfilter[0].can_mask = 0x00; // accept anything

    return setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
}
