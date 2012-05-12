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
//S rs-canopen-pdo-upload
//S ---------------------
//S 
//S This program can be used for uploading SDOs from slave units.
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
    int sock, nbytes, n = 0, exp = 1;
    uint8_t node, subindex, data_array[256];
    uint16_t index;
    uint32_t data;

    bzero((void *)data_array, sizeof(data_array));

    if (argc != 5 && argc != 6)
    {
        fprintf(stderr, "usage: %s can-interface NODE INDEX SUBINDEX\n", argv[0]);
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

    if (argc == 6 && strcmp(argv[5], "SEG") == 0)
    {
        exp = 0;
    }

    if (exp)
    {
        // expediated upload
        if (canopen_sdo_upload_exp(sock, node, index, subindex, &data) == 0)
        {
            printf("%.8x\n", data);
        }
        else
        {
            printf("ERROR: expediated SDO upload failed\n");
        }
    }
    else
    {
        // segmented upload
        int len, i;
        if ((len = canopen_sdo_upload_seg(sock, node, index, subindex, data_array, sizeof(data_array))) >= 0)
        {
            //printf("RECEIVED: %d bytes of data: ", len);
            for (i = 0; i < len; i++)
                printf("%.2x", data_array[i]);
            printf("\n");

            //printf("RECEIVED: string = %s\n", (char *)data_array);

        }
        else
        {
            printf("ERROR: segmented SDO upload failed\n");
        }
    }
    return 0;
}
