/**
   @file sinusoids.h
   @brief Sinusoid extraction/synthesis
*/

/* Copyright (C) 2005

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _SINUSOIDS_H
#define _SINUSOIDS_H

void find_sinusoids(float *psd, float *w, int *N, int length);

void extract_sinusoids(float *x, float *w, float *window, float *ai, float *bi, float *y, int N, int len);

void extract_modulated_sinusoids(float *x, float *w, float *window, float *ai, float *bi, float *ci, float *di, float *y, int N, int len);

#endif
