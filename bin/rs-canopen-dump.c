//------------------------------------------------------------------------------
// Copyright (C) 2012, Robert Johansson, Raditex AB
// All rights reserved.
//
// This file is part of rSCADA.
//
// rSCADA 
// http://www.rSCADA.se
// info@rscada.se
//------------------------------------------------------------------------------

// sphinx documentation
//
//S 
//S rs-canopen-dump
//S ----------------
//S 
//S This program connects to CAN bus and attempt to parse all incoming frames, and
//S print the frame in a human-readable format on the standard output.
//S 

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <string.h>
#include <stdio.h> 

#include <canopen/canopen.h>

int
main(int argc, char **argv)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame can_frame;
    canopen_frame_t canopen_frame;
    int sock, bytes_read;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s can-interface\n", argv[0]);
        return -1;
    }

    /* Create the socket */
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return -1;
    }
 
    /* Locate the interface you wish to use */
    strcpy(ifr.ifr_name, argv[1]);
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

    while (1)
    {

        bytes_read = read(sock, &can_frame, sizeof(struct can_frame));

        if (bytes_read < 0)
        {
            perror("read: can raw socket read");
            return 1;
        }

        if (bytes_read < (int)sizeof(struct can_frame))
        {
            fprintf(stderr, "read: incomplete CAN frame\n");
            return 1;
        }

        if (canopen_frame_parse(&canopen_frame, &can_frame) != 0)
        {
            fprintf(stderr, "CANopen failed to parse frame\n");
        }
    
        canopen_frame_dump_short(&canopen_frame);
    }

    return 0;
}
