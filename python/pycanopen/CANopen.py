#!/usr/bin/python
# ------------------------------------------------------------------------------
# Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex Control AB
# All rights reserved.
#
# This file is part of the rSCADA system.
#
# rSCADA 
# http://www.rSCADA.se
# info@rscada.se
# ------------------------------------------------------------------------------

"""
Python bindings for rSCADA libCANopen.
"""

from ctypes import *

libcanopen = cdll.LoadLibrary('libcanopen.so')
libc = cdll.LoadLibrary('libc.so.6')
    
class CANFrame(Structure):
    _fields_ = [("can_id",  c_uint32),
                ("can_dlc", c_uint8),
                ("align",   c_uint8 * 3), # hardcoded alignment to match C struct
                ("data",    c_uint8 * 8)]

    def __str__(self):
        data_str = " ".join(["%.2x" % (x,) for x in self.data])
        return "CAN Frame: ID=%.2x DLC=%.2x DATA=[%s]" % (self.can_id, self.can_dlc, data_str)
        
class CANopenFrame(Structure):
    _fields_ = [("rtr",           c_uint8),
                ("function_code", c_uint8),
                ("type",          c_uint8),
                ("id",            c_uint32),
                ("data",          c_uint8 * 8), # should be a union...
                ("data_len",      c_uint8)]

    def __str__(self):
        data_str = " ".join(["%.2x" % (x,) for x in self.data])    
        return "CANopen Frame: RTR=%d FC=0x%.2x ID=0x%.2x [len=%d] %s" % (self.rtr, self.function_code, self.id, self.data_len, data_str)

class CANopen:

    def __init__(self, interface="can0"):
        """
        Constructor for CANopen class. Optionally takes an interface 
        name for which to bind a socket to. Defaults to interface "can0"
        """
        self.sock = libcanopen.can_socket_open(interface)
        
    def open(self, interface):
        """
        Open a new socket. If open socket already exist, close it first.
        """        
        if self.sock:
            self.close()
        self.sock = libcanopen.can_socket_open(interface)
        
    def close(self):
        """
        Close the socket associated with this class instance.
        """
        if self.sock:
            libcanopen.can_socket_close(self.sock)
            self.sock = None
            
    def read_can_frame(self):
        """
        Low-level function: Read a CAN frame from socket.
        """
        if self.sock:
            can_frame = CANFrame()
            if libc.read(self.sock, byref(can_frame), c_int(16)) != 16:
                raise "CAN frame read error"
            return can_frame
        else:
            raise "CAN fram read error: socket not connected"
            
    def parse_can_frame(self, can_frame):
        """
        Low level function: Parse a given CAN frame into CANopen frame
        """
        canopen_frame = CANopenFrame()        
        if libcanopen.canopen_frame_parse(byref(canopen_frame), byref(can_frame)) == 0:
            return canopen_frame
        else:
            raise "CANopen Frame parse error"
                        
    def read_frame(self):
        """
        Read a CANopen frame from socket. First read a CAN frame, then parse
        into a CANopen frame and return it.
        """
        can_frame = self.read_can_frame()
        if not can_frame:
            raise "CAN Frame read error"

        canopen_frame = self.parse_can_frame(can_frame)
        if not canopen_frame:
            raise "CANopen Frame parse error"
        
        return canopen_frame
            

    #---------------------------------------------------------------------------
    # SDO related functions
    #

    #
    # EXPEDIATED
    #

    def SDOUploadExp(self, node, index, subindex):
        """
        Expediated SDO upload
        """
        res = c_uint32()
        ret = libcanopen.canopen_sdo_upload_exp(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), byref(res))

        if ret != 0:
            raise Exception("CANopen Expediated SDO upload error")

        return res.value
        

       
    def SDODownloadExp(self, node, index, subindex, data, size):
        """
        Expediated SDO download
        """

        ret = libcanopen.canopen_sdo_download_exp(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), c_uint32(data), c_uint16(size))

        if ret != 0:
            raise Exception("CANopen Expediated SDO download error")

    #
    # SEGMENTED
    #      
        
    def SDOUploadSeg(self, node, index, subindex, size):
        """
        Segmented SDO upload
        """
        data = create_string_buffer(size)
        ret = libcanopen.canopen_sdo_upload_seg(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), data, c_uint16(size));

        if ret < 0:
            raise Exception("CANopen Segmented SDO upload error: ret = %d" % ret)

        hex_str = "".join(["%.2x" % ord(data[i]) for i in range(ret)])
        #[0:-2]

        return hex_str

       
    def SDODownloadSeg(self, node, index, subindex, str_data, size):
        """
        Segmented SDO download
        """
        n = len(str_data)/2
        data = create_string_buffer(''.join([chr(int(str_data[2*n:2*n+2],16)) for n in range(n)]))

        ret = libcanopen.canopen_sdo_download_seg(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), data, c_uint16(n));

        if ret != 0:
            raise Exception("CANopen Segmented SDO download error")

    #
    # BLOCK
    #

    def SDOUploadBlock(self, node, index, subindex, size):
        """
        Block SDO upload: not implemented.
        """
        data = create_string_buffer(size)
        ret = libcanopen.canopen_sdo_upload_block(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), data, c_uint16(size));

        if ret != 0:
            raise Exception("CANopen Block SDO upload error")

        hex_str = "".join(["%.2x" % ord(d) for d in data])[0:-2]

        return hex_str
        
        
    def SDODownloadBlock(self, node, index, subindex, str_data, size):
        """
        Block SDO download
        """
        n = len(str_data)/2
        data = create_string_buffer(''.join([chr(int(str_data[2*n:2*n+2],16)) for n in range(n)]))

        ret = libcanopen.canopen_sdo_download_block(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), data, c_uint16(n));

        if ret != 0:
            raise Exception("CANopen Block SDO download error")
