#
# (C) Copyright 2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(CPU).a

START	= start.o
ASOBJS	= io.o firmware_sc_task_bestcomm.impl.o firmware_sc_task.impl.o
OBJS	= i2c.o traps.o cpu.o cpu_init.o speed.o interrupts.o serial.o \
	  loadtask.o fec.o pci_mpc5200.o

all:	.depend $(START) $(ASOBJS) $(LIB)

$(LIB):	$(OBJS)
	$(AR) crv $@ $(ASOBJS) $(OBJS)

#########################################################################

.depend:	Makefile $(START:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(START:.o=.S) $(ASOBJS:.o=.S) $(OBJS:.o=.c) > $@

sinclude .depend

#########################################################################
