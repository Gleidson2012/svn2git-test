/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file speex_bits.h
   @brief Handles bit packing/unpacking
*/
/*
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

#ifndef BITS_H
#define BITS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum size of the bit-stream (for fixed-size allocation) */
#define MAX_BYTES_PER_FRAME 1000

/** Bit-packing data structure representing (part of) a bit-stream. */
typedef struct SpeexBits {
   char *bytes; /**< "raw" data */
   int  nbBits; /**< Total number of bits stored in the stream*/
   int  bytePtr; /**< Position of the byte "cursor" */
   int  bitPtr;  /**< Position of the bit "cursor" within the current byte */
   int  owner; /**< Does the struct "own" the "raw" buffer (member "bytes") */
} SpeexBits;

/** Initializes and allocates resources for a SpeexBits struct */
void speex_bits_init(SpeexBits *bits);

/** Initializes SpeexBits struct using a pre-allocated buffer*/
void speex_bits_init_buffer(SpeexBits *bits, void *buff);

/** Frees all resources assiociated to a SpeexBits struct. Right now this does nothing since no resources are allocated, but this could change in the future.*/
void speex_bits_destroy(SpeexBits *bits);

/** Resets bits to initial value (just after initialization, erasing content)*/
void speex_bits_reset(SpeexBits *bits);

/** Rewind the bit-stream to the beginning (ready for read) without erasing the content */
void speex_bits_rewind(SpeexBits *bits);

/** Initializes the bit-stream from the data in an area of memory */
void speex_bits_read_from(SpeexBits *bits, char *bytes, int len);

/** Append bytes to the bit-stream
 * @param bits Bit-stream to operate on
 * @param bytes pointer to the bytes what will be appended
 * @param len Number of bytes of append
 */
void speex_bits_read_whole_bytes(SpeexBits *bits, char *bytes, int len);

/** Write the content of a bit-stream to an area of memory */
int speex_bits_write(SpeexBits *bits, char *bytes, int max_len);

/** Like speex_bits_write, but writes only the complete bytes in the stream. Also removes the written bytes from the stream */
int speex_bits_write_whole_bytes(SpeexBits *bits, char *bytes, int max_len);

/** Append bits to the bit-stream
 * @param bits Bit-stream to operate on
 * @param data Value to append as integer
 * @param nbBits number of bits to consider in "data"
 */
void speex_bits_pack(SpeexBits *bits, int data, int nbBits);

/** Interpret the next bits in the bit-stream as a signed integer
 *
 * @param bits Bit-stream to operate on
 * @param nbBits Number of bits to interpret
 * @return A signed integer represented by the bits read
 */
int speex_bits_unpack_signed(SpeexBits *bits, int nbBits);

/** Interpret the next bits in the bit-stream as an unsigned integer
 *
 * @param bits Bit-stream to operate on
 * @param nbBits Number of bits to interpret
 * @return An unsigned integer represented by the bits read
 */
unsigned int speex_bits_unpack_unsigned(SpeexBits *bits, int nbBits);

/** Returns the number of bytes in the bit-stream, including the last one even if it is not "full"
 *
 * @param bits Bit-stream to operate on
 * @return Number of bytes in the stream
 */
int speex_bits_nbytes(SpeexBits *bits);

/** Same as speex_bits_unpack_unsigned, but without modifying the cursor position */
unsigned int speex_bits_peek_unsigned(SpeexBits *bits, int nbBits);

/** Get the value of the next bit in the stream, without modifying the
 * "cursor" position 
 * 
 * @param bits Bit-stream to operate on
 */
int speex_bits_peek(SpeexBits *bits);

/** Advances the position of the "bit cursor" in the stream 
 *
 * @param bits Bit-stream to operate on
 * @param n Number of bits to advance
 * */
void speex_bits_advance(SpeexBits *bits, int n);

#ifdef __cplusplus
}
#endif

#endif
