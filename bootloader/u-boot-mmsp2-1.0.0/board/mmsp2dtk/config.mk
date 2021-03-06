#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
# David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
#
# Magic Eyes MMSP2 DTK board with MP2520F(ARM920T) cpu
# 
# (C) Copyright 2004
# DIGNSYS Inc. < www.dignsys.com >
# Kane Ahn < hbahn@dignsys.com >
#
# MMSP2 DTK has 1 bank of 64 MB DRAM
#
# 0000'0000 to 0400'0000
#
# Linux-Kernel is expected to be at 0000'8000, entry 0000'8000
# optionally with a ramdisk at 0080'0000
# we load ourself to 03F8'0000
# download area is 0300'0000
#

TEXT_BASE = 0x03F80000
