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

    def SDOUploadExp(self, node, index, subindex):
        """
        Expediated SDO upload
        """
        res = c_uint32()
        libcanopen.canopen_sdo_upload_exp(self.sock, c_uint8(node), c_uint16(index), c_uint8(subindex), byref(res))
        return res.value
        
        
        
