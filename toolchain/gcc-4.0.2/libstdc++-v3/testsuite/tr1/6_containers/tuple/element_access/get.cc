// 2004-09-23 Chris Jefferson <chris@bubblescope.net>

// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
// USA.

// Tuple

#include <tr1/tuple>
#include <testsuite_hooks.h>

using namespace std;
using namespace tr1;

int
main()
{
  int j=1;
  const int k=2;
  tuple<int,int &,const int&> a(0,j,k);
  const tuple<int,int &,const int&> b(1,j,k); 
  VERIFY(get<0>(a)==0 && get<1>(a)==1 && get<2>(a)==2);
  get<0>(a)=3;
  get<1>(a)=4;  
  VERIFY(get<0>(a)==3 && get<1>(a)==4);
  VERIFY(j==4);
  get<1>(b)=5;
  VERIFY(get<0>(b)==1 && get<1>(b)==5 && get<2>(b)==2);
  VERIFY(j==5);
}

