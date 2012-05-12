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

//S 
//S rs-canopen-node-info
//S --------------------
//S 
//S Print information for a CANopen node, including manufacturer ID, product
//S ID, software version, etc. These values are read from the device using the
//S expediated SDO protocol, and should be available for all CANopen compatible
//S devices. 
//S 
//S The application is called as::
//S 
//S     $ rs-canopen-information CAN-DEVICE NODE
//S 
//S where CAN-DEVICE is, e.g., can0 or can1, etc., NODE is the 7-bit or 29-bit
//S CANopen node ID.
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

    int sock;
    uint8_t node;
    uint32_t value;

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

    printf("Node = 0x%.2X\n", node);

    // Vendor ID
    canopen_sdo_upload_exp(sock, node, 0x1018, 0x01, &value);
    printf("Vendor  ID    = 0x%.8X\n", value);

    // Product ID
    canopen_sdo_upload_exp(sock, node, 0x1018, 0x02, &value);
    printf("Product ID    = 0x%.8X\n", value);

    // Software Ver
    canopen_sdo_upload_exp(sock, node, 0x1018, 0x03, &value);
    printf("Software Ver. = 0x%.8X\n", value);

    return 0;
}
