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
//S rs-canopen-monitor
//S ------------------
//S 
//S This program connects to CAN bus and attempt to parse all incoming frames, and
//S maintains a list of recent frames on the standard output. Unlike rs-opencan-dump,
//S this applications clears the screen between each update.
//S 
//S The application is called as::
//S 
//S     $ rs-canopen-monitor CAN-DEVICE
//S 
//S where CAN-DEVICE is, e.g., can0 or can1, etc.
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
#include <stdlib.h>

#include <canopen/canopen.h>

//------------------------------------------------------------------------------
// Frame cache management
//

typedef struct _can_frame_cache {

    struct timeval tv;
    double period;
    canopen_frame_t frame;


    void *next;
} can_frame_cache_t;

static can_frame_cache_t *frame_cache = NULL;

double
timeval_diff(struct timeval *tv0, struct timeval *tv1)
{
    double t0, t1;

    if (tv0 == NULL || tv1 == NULL)
        return 0.0;

    t0 = tv0->tv_sec + tv0->tv_usec / 1.0e6;
    t1 = tv1->tv_sec + tv1->tv_usec / 1.0e6;
    
    return t1 - t0;
}

can_frame_cache_t *
can_frame_cache_new()
{   
    can_frame_cache_t *fc;

    if ((fc = (can_frame_cache_t *)malloc(sizeof(can_frame_cache_t))) == NULL)
    {
        fprintf(stderr, "Failed to allocate new frame cache item.\n");
        return NULL;
    }

    fc->period = 0.0;
    fc->next = NULL;
}

can_frame_cache_t *
can_frame_cache_add(canopen_frame_t *frame, struct timeval *tv)
{
    can_frame_cache_t *fc;

    fc = can_frame_cache_new();
    memcpy((void *)&(fc->frame), (void *)frame, sizeof(canopen_frame_t));
    fc->tv.tv_sec = tv->tv_sec;
    fc->tv.tv_usec = tv->tv_usec;

    if (frame_cache == NULL)
    {   
        frame_cache = fc;
        return;
    }

    fc->next = frame_cache;
    frame_cache = fc;

    return fc;
}

can_frame_cache_t *
can_frame_cache_update(canopen_frame_t *frame, struct timeval *tv)
{
    can_frame_cache_t *iter;

    for (iter = frame_cache; iter; iter = iter->next)
    {
        if (iter->frame.id == frame->id &&
            iter->frame.function_code == frame->function_code)
        {
            double delay = timeval_diff(&(iter->tv), tv);
            iter->tv.tv_sec = tv->tv_sec;
            iter->tv.tv_usec = tv->tv_usec;

            iter->period = (iter->period + delay) / 2.0;

            memcpy((void *)&(iter->frame), (void *)frame, sizeof(canopen_frame_t));

            return iter;
        }
    }
    
    return can_frame_cache_add(frame, tv);
}

void
can_frame_cache_print()
{
    can_frame_cache_t *iter;

    system("clear");

    for (iter = frame_cache; iter; iter = iter->next)
    {
        printf("%.6fs ", iter->period);
        canopen_frame_dump_short(&(iter->frame));
    }
}

//
//
//------------------------------------------------------------------------------



int
main(int argc, char **argv)
{
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame can_frame;
    struct timeval tv;
    canopen_frame_t canopen_frame;
    can_frame_cache_t *fc;
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

        ioctl(sock, SIOCGSTAMP, &tv);

        if (canopen_frame_parse(&canopen_frame, &can_frame) != 0)
        {
            fprintf(stderr, "CANopen failed to parse frame\n");
        }

        if ((fc = can_frame_cache_update(&canopen_frame, &tv)) == NULL)
        {
            fprintf(stderr, "Failed to lookup frame\n");        
            continue;
        }

        can_frame_cache_print();
    }

    return 0;
}
