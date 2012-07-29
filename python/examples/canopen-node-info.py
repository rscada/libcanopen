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
Read node info from a CANopen node
"""

import sys
from pyCANopen import *

canopen = CANopen()

if len(sys.argv) == 1:
    # get the node address from the first command-line argument
    node = int(sys.argv[1])
else:
    print("usage: %s NODE" % sys.argv[0])
    exit(1)

value = canopen.SDOUploadExp(node, 0x1018, 0x01)
print("Vendor  ID   = 0x%.8X\n" % value)

value = canopen.SDOUploadExp(node, 0x1018, 0x02)
print("Product ID   = 0x%.8X\n" % value)

value = canopen.SDOUploadExp(node, 0x1018, 0x03)
print("Software Ver = 0x%.8X\n" % value)

