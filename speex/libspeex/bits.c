/* Copyright (C) 2002 Jean-Marc Valin 
   File: bits.c

   Handles bit packing/unpacking

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "bits.h"
#include <stdio.h>

void frame_bits_init(FrameBits *bits)
{
   bits->nbBits=0;
   bits->bytePtr=0;
   bits->bitPtr=0;
}

void frame_bits_destroy(FrameBits *bits)
{
   /* Will do something once the allocation is dynamic */
}

void frame_bits_init_from(FrameBits *bits, char *bytes, int len)
{
   int i;
   if (len > MAX_BYTES_PER_FRAME)
   {
      fprintf (stderr, "Trying to init frame with too many bits");
      exit(1);
   }
   for (i=0;i<len;i++)
      bits->bytes[i]=bytes[i];
   bits->nbBits=len<<3;
   bits->bytePtr=0;
   bits->bitPtr=0;
}

void frame_bits_write(FrameBits *bits, char *bytes, int max_len)
{
   int i;
   if (max_len > ((bits->nbBits+7)>>3))
      max_len = ((bits->nbBits+7)>>3);
   for (i=0;i<max_len;i++)
      bytes[i]=bits->bytes[i];
}

void frame_bits_pack(FrameBits *bits, int data, int nbBits)
{
}

int frame_bits_unpack_signed(FrameBits *bits, int nbBits)
{
}

unsigned int frame_bits_unpack_unsigned(FrameBits *bits, int nbBits)
{
}
