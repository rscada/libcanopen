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

from pyCANopen import *

canopen = CANopen()

node = 5

value = canopen.SDOUploadExp(node, 0x1018, 0x01)
print("Vendor  ID   = 0x%.8X\n" % value)

value = canopen.SDOUploadExp(node, 0x1018, 0x02)
print("Product ID   = 0x%.8X\n" % value)

value = canopen.SDOUploadExp(node, 0x1018, 0x03)
print("Software Ver = 0x%.8X\n" % value)

