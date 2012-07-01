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
Read CAN frame, parse into CANopen frame, and dump to STDOUT.
"""

from pyCANopen import *

canopen = CANopen()

while True:
    canopen_frame = canopen.read_frame()
    if canopen_frame:
        print canopen_frame
    else:
        print("CANopen Frame parse error")
            
#while True:
#    can_frame = canopen.read_can_frame()
#    if can_frame:
#        print can_frame
#    else:
#        print("CAN Frame parse error")
#        continue
#    canopen_frame = canopen.parse_can_frame(can_frame)
#    if canopen_frame:
#        print canopen_frame
#    else:
#        print("CANopen Frame parse error")
                        
