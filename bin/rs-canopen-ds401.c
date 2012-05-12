//------------------------------------------------------------------------------
// Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex AB
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
//S rs-canopen-ds401
//S ---------------------
//S 
//S Command line tool for reading and writing the state of CANopen DS401 nodes.
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
usage()
{
    fprintf(stderr, "usage: rs-canopen-ds401 INTERFACE NODE COMMAND DEVICE ADDRESS [VALUE]\n");
    fprintf(stderr, "\tINTERFACE = can0 | can1 | ...\n");
    fprintf(stderr, "\tNODE\t     = 0 .. 127\n");
    fprintf(stderr, "\tCOMMAND\t  = read | write\n");    
    fprintf(stderr, "\tDEVICE\t  = di | do\n");
    fprintf(stderr, "\tADDRESS\t  = 0 .. 3\n");
    fprintf(stderr, "\tVALUE\t  = 0 | 1    [only for COMMAND = write]\n");
}

int
main(int argc, char **argv)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame can_frame;
    canopen_frame_t canopen_frame;
    int sock, nbytes, n = 0, exp = 1;
    uint8_t node, address, subindex, data_array[256], value;
    uint16_t index;
    uint32_t data;

    bzero((void *)data_array, sizeof(data_array));

    if (argc != 6 && argc != 7)
    {
        usage();
        return 1;
    }

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return 1;
    }
 
    strcpy(ifr.ifr_name, argv[1]);
    ioctl(sock, SIOCGIFINDEX, &ifr); // XXX add check
 
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sock, (struct sockaddr*)&addr, sizeof(addr)); // XXX Add check
   
    node = strtol(argv[2], NULL, 16);
    address = atoi(argv[5]);

    if (address > 8)
    {
        fprintf(stderr, "ERROR: address out of range [%d]\n", address);
        return 1;
    }

    if (strcmp(argv[4], "do") == 0)
    {
        index    = 0x6200;
    }
    else if (strcmp(argv[4], "di") == 0)
    {
        index    = 0x6000;
    }
    else
    {
        usage();
        return;
    }

    if (strcmp(argv[3], "read") == 0 && argc == 6)
    {    
        subindex = 0x01;

        if (canopen_sdo_upload_exp(sock, node, index, subindex, &data) != 0)
        {
            fprintf(stderr, "ERROR: expediated SDO upload failed\n");
            return 1;
        }

        printf("%d\n", (data & (1<<address)) ? 1 : 0);

    }
    else if (strcmp(argv[3], "write") == 0 && index == 0x6200 && argc == 7)
    {
        subindex = 0x01;

        value = strcmp(argv[6], "1") == 0 ? 1 : 0;

        if (canopen_sdo_upload_exp(sock, node, index, subindex, &data) != 0)
        {
            fprintf(stderr, "ERROR: expediated SDO upload failed\n");
            return 1;
        }

        if (value)
        {
            data |= (1 << address) & 0xFF;
        }
        else
        {
            data &= (~(1 << address)) & 0xFF;
        }

        if (canopen_sdo_download_exp(sock, node, index, subindex, data, 1) != 0)
        {
            printf("ERROR: Expediated SDO download failed.\n");
            return 1;
        }
    }
    else
    {
        usage();
        return 1;
    }

    return 0;
}
