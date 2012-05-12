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
//S rs-canopen-sdo-download
//S -----------------------
//S 
//S This program can be used for downloading SDOs to slave units.
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
    int sock, nbytes, n = 0, mode;
    uint8_t node, subindex;
    uint16_t index, len;
    uint32_t data = 0;
    uint8_t data_array[256];

    if (argc != 6 && argc != 7)
    {
        fprintf(stderr, "usage: %s can-interface NODE INDEX SUBINDEX DATA [MODE]\n", argv[0]);
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
    len      = strlen(argv[5])/2;

    if (argc == 7 && strcmp(argv[6], "SEG") == 0)
    {
        char *data_str = argv[5];
        int i = 0, j = 0;

        mode = 1;
        for (i = 0; i < len * 2; i++)
        {
            char tmp[3];
            tmp[0] = data_str[j++];
            tmp[1] = data_str[j++];
            tmp[2] = 0;

            data_array[i] = strtol(tmp, NULL, 16);
        }
    }
    if (argc == 7 && strcmp(argv[6], "BLOCK") == 0)
    {
        char *data_str = argv[5];
        int i = 0, j = 0;

        mode = 2;
        for (i = 0; i < len * 2; i++)
        {
            char tmp[3];
            tmp[0] = data_str[j++];
            tmp[1] = data_str[j++];
            tmp[2] = 0;

            data_array[i] = strtol(tmp, NULL, 16);
        }
    }
    else
    {
        mode = 0;        
        data = strtol(argv[5], NULL, 16);
    }

    if (mode == 0)
    {
        // expediated download
        if (canopen_sdo_download_exp(sock, node, index, subindex, data, len) == 0)
        {
            ;//printf("Download OK\n");
        }
        else
        {
            printf("ERROR: Expediated SDO download failed.\n");
        }
    }
    else if (mode == 1)
    {
        // segmented download
        if (canopen_sdo_download_seg(sock, node, index, subindex, data_array, len) == 0)
        {
            ;//printf("Download OK\n");
        }
        else
        {
            printf("ERROR: Segmented SDO download failed.\n");
        }
    }
    else if (mode == 2)
    {
        // block download
        if (canopen_sdo_download_block(sock, node, index, subindex, data_array, len) == 0)
        {
            ;//printf("Download OK\n");
        }
        else
        {
            printf("ERROR: Block SDO download failed.\n");
        }
    }

    return 0;
}
