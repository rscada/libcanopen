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
//S rs-canopen-pdo-download
//S -----------------------
//S 
//S This program can be used for downloading PDOs to slave units.
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
#include <canopen/canopen-com.h>

int
main(int argc, char **argv)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame can_frame;
    canopen_frame_t canopen_frame;
    int sock, nbytes, n = 0;
    uint8_t node, subindex, len;
    uint16_t index;
    uint32_t data;

    if (argc != 6)
    {
        fprintf(stderr, "usage: %s can-interface NODE INDEX SUBINDEX DATA\n", argv[0]);
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
    index    = strtol(argv[3], NULL, 16);
    subindex = strtol(argv[4], NULL, 16);
    data     = strtol(argv[5], NULL, 16);
    len      = strlen(argv[5])/2;

    printf("DEBUG: Send SDO DOWNLOAD to Node=0x%.2X Index=0x%.4X SubIndex=0x%.2X Data=0x%.8X [%d]\n",
            node, index, subindex, data, len);

    // XXX: incomplete
    //canopen_frame_set_sdo_download(&canopen_frame, node, index, subindex, data, len);

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

        if (canopen_frame.id == node && canopen_frame.function_code == CANOPEN_FC_SDO_TX)
        {   
            printf("GOT REPLY [%d]\n", n);

            return 0;
        }

        n++;
    }

    return 0;
}
