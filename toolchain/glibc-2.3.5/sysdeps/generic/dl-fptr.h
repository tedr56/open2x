/* Function descriptors. Generic version.
   Copyright (C) 1995, 1996, 1997, 2000, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef dl_fptr_h
#define dl_fptr_h 1

/* An FDESC is a function descriptor.  */

struct fdesc
  {
    ElfW(Addr) ip;	/* code entry point */
    ElfW(Addr) gp;	/* global pointer */
  };

struct fdesc_table
  {
    struct fdesc_table *next;
    unsigned int len;			/* # of entries in fdesc table */
    volatile unsigned int first_unused;	/* index of first available entry */
    struct fdesc fdesc[0];
  };

extern ElfW(Addr) _dl_boot_fptr_table [];

extern ElfW(Addr) _dl_make_fptr (struct link_map *, const ElfW(Sym) *,
				 ElfW(Addr));

#endif /* !dl_fptr_h */
