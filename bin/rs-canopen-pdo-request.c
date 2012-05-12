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
//S rs-canopen-pdo-request
//S ---------------------
//S 
//S This program can be used for sending a PDO request to a device.
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
    int sock, nbytes, n = 0;
    uint8_t node, subindex;
    uint16_t index;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s can-interface NODE\n", argv[0]);
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
    ioctl(sock, SIOCGIFINDEX, &ifr); // XXX add check
 
    /* Select that CAN interface, and bind the socket to it. */
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // XXX Add check
   
    node     = strtol(argv[2], NULL, 16);

    printf("DEBUG: Send PDO REQUEST to Node=0x%.2X\n", node);

    canopen_frame_set_pdo_request(&canopen_frame, node);

    if (canopen_frame_pack(&canopen_frame, &can_frame) != 0)
    {
        fprintf(stderr, "CANopen failed to parse frame\n");
        return 1;
    }

    // send the frame to the CAN bus
    nbytes = write(sock, &can_frame, sizeof(can_frame));

    if (nbytes < 0)
    {
        perror("read: can raw socket read");
        return 1;
    }

    if (nbytes < (int)sizeof(struct can_frame))
    {
        fprintf(stderr, "read: incomplete CAN frame\n");
        return 1;
    }

    while (1)
    {
        // set filters for our node?

        nbytes = read(sock, &can_frame, sizeof(struct can_frame));

        if (nbytes < 0)
        {
            perror("read: can raw socket read");
            return 1;
        }

        if (nbytes < (int)sizeof(struct can_frame))
        {
            fprintf(stderr, "read: incomplete CAN frame\n");
            return 1;
        }

        if (canopen_frame_parse(&canopen_frame, &can_frame) != 0)
        {
            fprintf(stderr, "CANopen failed to parse frame\n");
        }

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_PDO1_TX)
        {   
            printf("REPLY [%d]: 0x%.2X 0x%.2X 0x%.2X 0x%.2X \n", 
                   n, canopen_frame.payload.sdo.data[0], canopen_frame.payload.sdo.data[1],
                      canopen_frame.payload.sdo.data[2], canopen_frame.payload.sdo.data[3]);

            return 0;
        }

        n++;
    }

    return 0;
}
