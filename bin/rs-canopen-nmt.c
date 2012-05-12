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
//S rs-canopen-nmt
//S --------------
//S 
//S CANopen network management: this application can read and set the state
//S of CANopen devices. Possible state commands are *start*, *stop*, *pre-op*,
//S *reset-app*, *reset-com*. 
//S 
//S The application is called as::
//S 
//S     $ rs-canopen-monitor CAN-DEVICE COMMAND NODE
//S 
//S where CAN-DEVICE is, e.g., can0 or can1, etc., and COMMAND is one state 
//S commands listed above (or *poll* for only reading out the state), and 
//S NODE is the 7-bit or 29-bit CANopen node ID.
//S
//S    COMMAND options:
//S        
//S    * start: Put the node in the started state.
//S    * stop:  Put the node in the stopped.
//S    * pre-op: Put the node in the pre-operational mode.
//S    * reset-app: Reset the application.
//S    * reset-com: Reset the communicatin.
//S    
//S Examples::
//S    
//S    $ rs-canopen-monitor can0 stop 07
//S    $ rs-canopen-monitor can0 start 07
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
    int sock, bytes_sent, cs, node;

    if (argc != 4)
    {
        fprintf(stderr, "usage: %s can-interface COMMAND NODE\n", argv[0]);
        fprintf(stderr, "    COMMAND = start, stop, pre-op, reset-app, reset-com, poll\n");
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
 
    cs = 0;

    if (strcmp(argv[2], "start") == 0)
    {
        cs = CANOPEN_NMT_MC_CS_START;
    }
    else if (strcmp(argv[2], "stop") == 0)
    {
        cs = CANOPEN_NMT_MC_CS_STOP;
    }
    else if (strcmp(argv[2], "pre-op") == 0)
    {
        cs = CANOPEN_NMT_MC_CS_PREOP;
    }
    else if (strcmp(argv[2], "reset-app") == 0)
    {
        cs = CANOPEN_NMT_MC_CS_RESET_APP;
    }
    else if (strcmp(argv[2], "reset-com") == 0)
    {
        cs = CANOPEN_NMT_MC_CS_RESET_APP;
    }
    // else -> "poll", i.e., a NG frame
    
    node = atoi(argv[3]);

    if (cs == 0)
    {
        printf("DEBUG: Send NMT NB to Node=0x%.2X\n", node);
        canopen_frame_set_nmt_ng(&canopen_frame, node);
    }
    else
    {
        printf("DEBUG: Send NMT MC with CS=0x%.2X to Node=0x%.2X\n", cs, node);
        canopen_frame_set_nmt_mc(&canopen_frame, cs, node);
    }

    if (canopen_frame_pack(&canopen_frame, &can_frame) != 0)
    {
        fprintf(stderr, "CANopen failed to parse frame\n");
        return 1;
    }

    // send the frame to the CAN bus
    bytes_sent = write(sock, &can_frame, sizeof(can_frame));

    if (bytes_sent < 0)
    {
        perror("read: can raw socket read");
        return 1;
    }

    if (bytes_sent < (int)sizeof(struct can_frame))
    {
        fprintf(stderr, "read: incomplete CAN frame\n");
        return 1;
    }

    return 0;
}
