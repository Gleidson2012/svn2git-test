#spec060 -- new stuff
#spec061 -- finally -- bitwise equivalent to C code! (without loop filter)
#spec062 -- FINALLY -- bitwise equvalent, even for TKAL!  problem was dang page/packet stuff
#           this works sorta, but still needs to be revamped so Ogg stuff is not brittle
#           but at least it works with its own test data.  Now gotta add loop filter
#spec063 -- some tweaks
#spec064 -- loop filter -- not working yet, but reorged pixel maps & stuff
#spec065 -- loop filter working! (?) yes!
#spec066 -- cleanup time -- derf's comments, other stuff (see unk.txt)
#           firster -- fix up UV ala C code
#spec067 -- first -- fixing header parsing stuff
#spec068 -- decode_packet() -- now we can do a proper main loop
#spec069 -- ok, using correct logic for headers & frames
#spec070 -- cleaned up for release as 0.070
#spec071 -- take command line args, insist on info & table headers -- make it 0.071

##_Theora Bitstream Specification and Reference Decoder -- Theora 1.0
#Document version: 0.071

#(c) 2003 Xiph Foundation, Dan Miller

#special thanks to various people at On2 Technologies for donating VP3, and to the folks at Xiph.org Foundation for
#doing good things with it. Particular shout outs to Mike Melansen, Tim Teriberry, Ralph Giles, Christopher
#Montgomery, Mauricio Piacentini, and Jack Moffit (please let me know if I spelled your name wrong..)

#This documentation and software are distributed under the terms of the BSD-style Theora license, as outlined in the
#"COPYING" file in the root directory of this distribution. Please familiarize yourself with these terms before
#distributing any of the documents in this directory.

#Introduction

#Theora is a video codec. This is the spec. 

#State of this document

#This document is unfinished. It is a work in progress that is included in the Theora release for your edification.
#Once it is complete and verified, it will hopefully become the canonical reference for Theora compatibility.

#The most glaring deficiency is that presently the documentation and Python decoder only cover keyframes. We are
#actively looking for volunteers to help us complete the work of fully documenting the Theora format. If you are
#interested and believe you can help, please post a message to theora-dev@xiph.org, or contact jack@xiph.org or
#danbmil99@yahoo.com.

#In addition to support for predicted frames, the Python script needs to be heavily error-protected. Presently
#various illegal bitstreams will cause the program to go into an endless loop or crash in other unpredictable ways.
#The specification should eventually be usable as a syntax checker that is able to detect and report on any possible
#error condition within the bitstream. In addition to work on the code, we will need to generate a comprehensive test
#suite of encoded files that exercise every aspect of the bitstream. Illegal files will also need to be generated to
#test the error handling capacity of the script.
#About this document

#This document is The Theora Bitstream Specification and Reference Decoder. It is both an english language
#description of the Theora video bitstream, and an interpretable program in the Python programming language, which
#can be executed using a version 2.0 or later Python interpreter. More information about Python can be gleaned at
#www.python.org. When run as a Python script, this document will decode compliant Theora bitstreams, producing
#uncompressed YUV files. The YUV output format is identical to that used by
#theora/win32/experimental/dumpvid/dump_vid.exe, which is as follows:

#For each frame, Y pixel data is output as unsigned characters, scanning left to right for each row of pixels
#starting with the top row. The Y data is followed by interleaved V and U data, ie one scanline of V followed by one
#scanline of U. This format allows raw data to be inspected visually as a monochrome image, with the V and U planes
#situated directly below the Y plane.

#To invoke this document as a Python program: at the command prompt, type:
#[prompt]> python spec.py testspec.ogg out.raw
#The program will attempt to open and decode 'testspec.ogg'.  The raw decoded file will be saved as "out.raw"

#Formatting Conventions

#There are three versions of this document: text, HTML, and Python. The HTML and Python versions are created from the
#text file, spec.txt. While spec.txt is actually a legal Python script, the formatted version spec.py is more
#readable as the comments have been reformatted with linefeeds and formatting characters have been stripped. To
#create the HTML version spec.html, a simple hinting scheme is used to specify some rudimentary formatting (see
#txt2html.py for details). The following commands will regenerate both files from the source file:

#[prompt]> python txt2py.py spec.txt spec.py
#[prompt]> python txt2html.py spec.txt spec.html

#In the HTML version, non-code sections are generally formatted using a variable-width font to distinguish them from
#code. Code and non-code can be interspersed, even within Python routines. In general, non-code blocks of text within
#routines will be italicized.

#Overview

#This document represents part of the overall specification package. The specification package includes the following
#elements:

# * Theora Bitstream Specification and Reference Decoder (this document) 

# * Verification Bitstreams (compressed data) 

# * Verification Output Streams (uncompressed YUV data)

#(Note: the following paragraph will not be considered in effect until this document is completed and verified - Ed.)

#A decoding application is Theora compliant IFF it can decode any Theora bitstream that is decodable with the
#Reference Decoder (this document), producing output that is bytewise equivalent. Several Verification Bitstreams and
#Verification Output Streams are included in the Specification Package for reference; the Verification Output Streams
#were produced using this document as a Python script. However, it is understood that a compliant decoder must be
#capable of decoding any legal Theora bitstream, not just those included in the Specification Package. A legal Theora
#bitstream is any bitstream that can be decoded by this document.

#Please note: the Python code herein is written solely to facilitate the definition and compliance testing of Theora
#bitstreams. It is by design an extremely inefficient and poorly structured piece of code. Do not use this as a
#template for real-world player applications. Instead, start with the C-based Theora decoder available for download
#at www.theora.org.

#The Theora Bitstream Specification does not cover encoders, except for the following sentence:

#An encoder is Theora compliant if it can produce a compressed bitstream that the Verification Decoder can decode.

#Note that this could be an application that produces nothing but black frames. The quality and scope of a Theora
#compliant encoder are entirely defined by the application domain for that product. If it produces a syntactically
#correct Theora bitstream, it is by definition a Theora encoder. The rest is up to you.


#Theora Bitstream Specification
#A Theora bitstream consists of header packet(s), followed by video packets. A decoder must receive all valid header
#packets before playing video. (note that this means if you wish to play a Theora stream from an arbitrary point, you
#need a mechanism to acquire the header information for that stream before commencing playback).

#Before we define our first routine, a little housekeeping for Python:

from array import array
from os import abort
import sys

#usage:

if len(sys.argv) < 3:
  print "usage: python spec.py infile outfile"
  abort()

#some globals & useful definitions:

#oggfile = file("testspec.ogg","rb")
oggfile = file(sys.argv[1],"rb")
oggstring = oggfile.read()                            #NOTE limited by memory constraints -- should use file I/O
oggindex = 0
pagebytes = 0
pagestart =0
oggbyte = 0
bitmask = 0
oggbuf = array('B',oggstring)                         #convert to an array of unsigned bytes
huffs = []                                            #this will contain list of huffman trees
infoflag = 0                                          #initialization flags
tableflag = 0

#Bitstream parsing routines

#Bit & byte ordering: typically, Ogg packs bits starting with the most significant bit to the least. For historical
#reasons, Theora packs bits least significant bit first. In cases where values are byte-aligned (8 bit boundaries),
#this only affects byte ordering. We do some Ogg parsing but it happens to be only on byte-aligned values, so we use
#readbits(8) but reverse the byte order (See for instance readOgg32())

#Note that in this specification we are assuming bytes are always 8 bit values. Future versions may support non-8-bit
#platforms.

#helpers:

def flushpacket():                                    #flush bits between packets
  global bitmask
  bitmask = 0

def flushpage():                                      #flush packet & disable paging for read_page_header()
  global bitmask, pagebytes
  bitmask = 0
  pagebytes = 999999                                  #kluge - yuk

#simple Ogg page header parsing routine.  Note we are not checking CRC's; we are assuming Ogg data is not corrupt.

def read_page_header():
  global oggindex, pagebytes, pagestart
  flushpage()
  oggs = readstring(4)                                #get the putated 4-byte Ogg identifier
  if oggs != "OggS":
    print "invalid page data -- OggS =", oggs
    abort()
  oggindex += 10                                      #serialnum at offset 14
  serialno = readOgg32()
  oggindex += 8                                       #segment count at offset 26
  segments = readbits(8)
  bytes = 0
  for i in range(segments):
    bytes += readbits(8)
  pagebytes = bytes
  pagestart = oggindex


#this routine just grabs a byte from the input stream:

def readbyte():                                       #note: this is a low-level function to read 
                                                      #a byte-aligned octet from the datastream.
                                                      #To read an arbitrarily aligned byte, use readbits(8)
  global oggindex,pagebytes
  if oggindex >= pagestart+pagebytes:
    read_page_header()
  byte = oggbuf[oggindex]
  oggindex += 1
  return byte

#These are used during the bulk of Theora stream parsing:

def readbit():
  global bitmask, oggbyte
  if bitmask == 0:
    oggbyte = readbyte()
    bitmask = 0x80
  if oggbyte & bitmask:
    bit = 1
  else:
    bit = 0
  bitmask >>= 1
  return bit

#readbits: our workhorse.  Gets up to 32 bits from the stream 
#(note -- hi bit is first bit read! use readOgg32() or build up values through sequential byte reads for Ogg
#purposes)

def readbits(x):
  ret = 0
  for i in range(x):
    ret <<= 1
    ret += readbit()
  return ret

#readstring reads a string of 8-bit unsigned chars:

def readstring(x):
  s = ''
  for i in range(x):
    s += chr(readbits(8))
  return s

#readOgg32 reads a longword Ogg style:

def readOgg32():                                      #different than readbits(32): byte order is reversed

  return readbits(8) + (readbits(8) << 8) + (readbits(8) << 16) + (readbits(8) << 24)

#entropy coding routines

#Certain values in Theora (such as DCT coefficients) are encoded using a context-sensitive Huffman scheme based on 32
#possible token values. Each token value has an associated set of extra bits that are bitpacked immediately following
#the primary huffman string. The binary decision trees (80 of them) necessary for decoding are in the table header.

#Set up the Huffman tables

#Huffman tables are encoded in compressed form using the following algorithm:

#Note how this function is called recursively for each possible branch in the tree until all branches have bottomed
#out with complete bitstrings:

hufftokens=0                                          #keep track of # of token strings -- 32 max

def read_hufftable(table):
  global hufftokens
  if readbit():                                       #if bit==1, this bitstring is complete
    table.append( readbits(5) )                       #next 5 bits = token number for this string
    hufftokens += 1
    if hufftokens > 32:
      print "illegal huffman table, > 32 tokens"
      abort()
  else:                                               #if bit was zero, we have two more entries defining
                                                      #the zero and one case for the next bit:
    table.append([])                                  #add another pair of tables
    table.append([])
    read_hufftable(table[0])                          #with an entry for zero
    read_hufftable(table[1])                          #and one for one

#read a token

#Again, we use recursion to parse the bits until we have a complete string:

def readtoken(huf):
  if type(huf[0]) == type(0):                         #integer means we have a value
    return huf[0]                                     #return token value
  else:
    if readbit():                                     #read a bit, recurse into subtable 0 or 1
      return readtoken(huf[1])                        #case for bit=1
    else:
      return readtoken(huf[0])                        #case for bit=0

#define an array of information tables for each token

#Each table contains the following five entries in this order:

##  base run length
##  number of extra run length bits (0 - 12)
##  base value
##  number of extra value bits (0 - 9)
##  number of extra sign bits (0 or 1)

token_array = [    
[1, 0, 'eob', 0, 0], 
[2, 0, 'eob', 0, 0], 
[3, 0, 'eob', 0, 0], 
[4, 2, 'eob', 0, 0], 
[8, 3, 'eob', 0, 0], 
[16, 4, 'eob', 0, 0], 
[0, 12, 'eob', 0, 0], 
[0, 3, 0, 0, 0], 

[0, 6, 0, 0, 0], 
[0, 0, 1, 0, 0], 
[0, 0, -1, 0, 0], 
[0, 0, 2, 0, 0], 
[0, 0, -2, 0, 0], 
[0, 0, 3, 0, 1], 
[0, 0, 4, 0, 1], 
[0, 0, 5, 0, 1], 

[0, 0, 6, 0, 1], 
[0, 0, 7, 1, 1], 
[0, 0, 9, 2, 1], 
[0, 0, 13, 3, 1], 
[0, 0, 21, 4, 1], 
[0, 0, 37, 5, 1], 
[0, 0, 69, 9, 1], 
[1, 0, 1, 0, 1], 

[2, 0, 1, 0, 1], 
[3, 0, 1, 0, 1], 
[4, 0, 1, 0, 1], 
[5, 0, 1, 0, 1], 
[6, 2, 1, 0, 1], 
[10, 3, 1, 0, 1], 
[1, 0, 2, 1, 1], 
[2, 1, 2, 1, 1] ]

#parse the tokens

#this function returns a run length & value based on the token & extended bits:

def parsetoken(huf):
  global token_array                                  #(not strictly necessary for read-only in Python)
  token = readtoken(huf)                              #read a token from the stream

  table = token_array[token]                          #get our table of parameters for this token
  run = table[0]                                      #base run length
  run_extra = table[1]                                #number of extra bits for run length
  value = table[2]                                    #actual value
  val_extra = table[3]                                #number of extra value bits
  sign_extra = table[4]                               #number of sign bits

  sign = 1
  if sign_extra:
    if readbit():
      sign = -1                                       #if there's a sign bit, get it.  1 means negative
                                                      #note that value may be negative to begin with, in
                                                      #which case there are no extra value or sign bits
  if val_extra:
    value += readbits(val_extra)                      #get extra value bits

  if run_extra:
    run += readbits(run_extra)                        #get extra run bits
  
  return [run, value * sign]                          #return run length and value
                                                      #note: string * 1 = string, so 'eob', 'zrl' are OK


#routines to read & parse codec headers

#routine to parse the Theora info header:

def read_info_header():
  global huffs, encoded_width, encoded_height, decode_width, decode_height, offset_x, offset_y
  global fps_numerator, fps_denominator, aspect_numerator, aspect_denominator, quality, bitrate
  global version_major, version_minor, version_subminor, colorspace, infoflag

  version_major = readbits(8)                         #major & minor version must be exact match
  if version_major != 3:
    print "incompatible major version#"
    abort()
  version_minor = readbits(8)
  if version_minor != 2:
    print "incompatible minor version#"
    abort()
  version_subminor = readbits(8)

  encoded_width = readbits(16) << 4                   #encoded width & height are in block units of 16x16
  encoded_height = readbits(16) << 4

  decode_width = readbits(24)                         #decode width & height are in actual pixels
  decode_height = readbits(24)
  offset_x = readbits(8)                              #offset for cropping if decode != full encoded frame
  offset_y = readbits(8)

  fps_numerator = readbits(32)                        #frames per second encoded as a fraction
  fps_denominator = readbits(32)
  aspect_numerator = readbits(24)                     #aspect not used now
  aspect_denominator = readbits(24)
  readbits(5)                                         #force keyframe frequency flag -- not used for decode
  colorspace = readbits(8)                            #colorspace flag defines YUV to RGB mapping
  bitrate = readbits(24)                              #target bitrate; not used for decode
  quality = readbits(6)                               #target quality also not used for decode
  infoflag = 1

#parse the comment header:

def read_comment_header():
  global vendor_string, vendor_string_len, comment_string, comment_string_len

  vendor_string_len = readOgg32()
  vendor_string = readstring(vendor_string_len)
  comment_string_len = readOgg32()
  comment_string = readstring(comment_string_len)


#read & parse the table header:

def read_table_header():
  global scale_table_AC, scale_table_DC, Y_quantizer, UV_quantizer, IF_quantizer
  global frequency_counts, hufftokens, tableflag

  scale_table_AC = []                                             #64 possible quantizer scalers for AC coeffs
  for x in range(64):    scale_table_AC.append(readbits(16))

  scale_table_DC = []                                             #64 possible quantizer scalers for DC coeffs
                                                                  #Note this is unrelated to 64 coeffs in an 8x8 block!
  for x in range(64):    scale_table_DC.append(readbits(16))

  Y_quantizer = []                                                #quantizers for intra Y coeff (this IS about 8x8 blocks!)
  for x in range(64):    Y_quantizer.append(readbits(8))

  UV_quantizer = []                                               #quantizers for intra U or V coeff
  for x in range(64):    UV_quantizer.append(readbits(8))

  IF_quantizer = []                                               #quantizers for interframe coeffs (Y, U, or V)
  for x in range(64):    IF_quantizer.append(readbits(8))

  for x in range(80):                                             #Read in huffman tables
    huffs.append([])
    hufftokens=0
    read_hufftable(huffs[x])

  tableflag = 1

def decode_header():

  header_type = readbits(7)

  cid = readstring(6)
  if cid != "theora":
    print "not a theora stream header", cid
    abort()

  if header_type == 0:
    read_info_header()
    flushpacket()
    return "info"

  elif header_type == 1:
    read_comment_header()
    flushpacket()
    return "comment"

  elif header_type == 2:
    read_table_header()
    flushpacket()
    return "table"

  else:
    print "unknown stream header type -- skipping"
    return "unknown"

# Routines that decode video

#[NOTE: for now, these routines only handle keyframes.  We may modify or add routines to support interframe data]
#[NOTE: each frame of video resides in a single Ogg page.]
#[NOTE: that's crap.  each frame of video resides in a logical Ogg packet.  pagination is irrelevant.]

#Hilbert ordering

#All data in Theora is organized into 8x8 blocks. When encoding the data, these blocks are further grouped into
#'super-blocks' of 16 blocks each, and encoded in 'Hilbert' order. Each super-block consists of up to 16 blocks
#encoded in the following order:

#      X -> X    X -> X
#           |    ^
#           v    |
#      X <- X    X <- X
#      |              ^
#      v              |
#      X    X -> X    X
#      |    ^    |    ^
#      v    |    v    |
#      X -> X    X -> X

#(thanks to Mike Melanson for the diagram)

#If any block is not coded (due to clipping, for instance -- encoded images can include partial super-blocks) the
#pattern continues until the next coded block is hit. Each pixel plane -- Y, V, and U -- are encoded using their own
#pattern of superblocks.

#By way of example: if a plane consisted of 32x16 pixels, only the top half of the Hilbert pattern would be used. If
#the 8 8x8 blocks in this example are labled in this way:

#    A B C D
#    E F G H

#then they will be encoded in the following order: A, B, F, E, H, G, C, D.


#the following array & function are used to 'de-hilbertize' the data. [note: this is a keyframe-only routine right
#now]

hilbert = [    
[0,0], [1,0], [1,1], [0,1], 
[0,2], [0,3], [1,3], [1,2], 
[2,2], [2,3], [3,3], [3,2], 
[3,1], [2,1], [2,0], [3,0] ]

def de_hilbert(w, h, colist):                               #width, height, coefficient list
  sbw = int( (w+3) / 4)                                     #super-block width (width in sb's)
  sbh = int( (h+3) / 4)                                     #super-block height (height in sb's)
  ii = 0 
  comap = []

  for x in range(w):                                        #initialize coefficient map
    comap.append([])
    for y in range(h):
      comap[x].append([])

  for y in range(sbh):
    for x in range(sbw):
      for i in range(16):
        p, q = hilbert[i]                                   #nice Python syntax
        xx = x*4 + p
        yy = y*4 + q
        if (xx < w) & (yy < h):                             #skip stuff out of range
          comap[xx][yy] = colist[ii]                        #if in range, get a coeff
          ii += 1
  return comap

#decoding the DC coefficients

#The DC values are simply the zero-order coefficients of each 8x8 block. These values tend to have more entropy than
#most AC components, so in addition to quantization, it is desirable to use delta coding to reduce the data
#requirement of encoding them.

#Theora uses a scheme where each encoded DC value is in fact a difference between a predicted value and the actual
#value. Since the blocks are coded in raster order, the predicted value can be any combination of DC values of blocks
#to the left, up, upper left, and upper right.

#routine to do DC prediction on a single color plane:

def DCpredict(comap, w, h):

#first row is straight delta coding:

  l = 0                                                     #l = DC coeff to my left; init to zero
  for x in range(w):
    l += comap[x][0][0]
    comap[x][0][0] = l
      
#now the rest:

  for y in range(h):
    for x in range(w):
      if y>0:                                               #already got the first row
        if x == 0:                                          #left column?
          u = comap[0][y-1][0]                              #u = upper
          ur = comap[1][y-1][0]                             #ur = upper-right
          p = u
          comap[0][y][0] += p                               #add predictor to decoded value

        else:                                               #general case -- neither left column nor top row
          l = comap[x-1][y][0]                              #l = left
          ul = comap[x-1][y-1][0]                           #ul = upper-left
          u = comap[x][y-1][0]                              #u = upper
          p = (l*29 + u*29 - ul*26)                         #compute weighted predictor
          if p < 0:
            p += 31                                         #round towards zero
          p >>= 5                                           #shift by weight multiplier
          if abs(p-u) > 128:                                #range checking
            p = u;
          if abs(p-l) > 128:
            p = l;
          if abs(p-ul) > 128:
            p = ul;

          comap[x][y][0] += p                               #add predictor to decoded value


#Dequantization logic

#(thanks again to Mike Melanson for this exposition)

#Setting Up The Dequantizers

#Theora has three static tables for dequantizing fragments-- one for intra Y fragments, one for intra C fragments,
#and one for inter Y or C fragments. In the following code, these tables are loaded as Y_quantizer[], UV_quantizer[],
#and IF_quantizer[] (see definition for read_table_header below). However, these tables are adjusted according to the
#value of quality_index.

#quality_index is an index into 2 64-element tables: scale_table_DC[] and scale_table_AC[]. Each dequantizer from the
#three dequantization tables is adjusted by the appropriate scale factor according to this formula:

#    Scale dequantizers:
     
#        dequantizer * sf
#        ----------------
#              100
     
#    where sf = scale_table_DC[quality_index] for DC dequantizer
#               scale_table_AC[quality_index] for AC dequantizer

#(Note that the dequantize routine also multiplies each coefficient by 4.  This is to facilitate the iDCT later on.)

def dequant(data, scaleDC, scaleAC, dqtable):
  mul = int( (scaleDC * dqtable[0]) / 100 ) * 4
  for x in range(64):
    if x>0:
      mul = int( (scaleAC * dqtable[x]) / 100 ) * 4
    data[x] *= mul

#ZIG-ZAG order

#The coefficients in each 8x8 DCT coded block are layed out in 'zig-zag' order.  

#The following table shows the order in which the 64 coefficients are coded:

zigzag=[  
  0,  1,  5,  6,  14, 15, 27, 28,
  2,  4,  7,  13, 16, 26, 29, 42,
  3,  8,  12, 17, 25, 30, 41, 43,
  9,  11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63]

#this routine remaps the coefficients to their original order:

def unzig(data,un):
  for x in range(64):
    un[x] = data[zigzag[x]]

#inverse DCT (iDCT)

#Once coefficients are re-ordered and dequantized, the iDCT is performed on the 8x8 matrix to produce the actual
#pixel values (or differentials for predicted blocks).

#Theora's particular choice of iDCT computation involves intermediate values that are calculated using 32 bit,
#fixed-point arithmetic. Multiplication is defined as follows (assuming 32-bit integer parameters):

def Mul_fix(a, b):
  return (a * b) >> 16

#addition and subtraction can be performed normally.

#First we define a one-dimensional iDCT. Note that there are many implementations of iDCT available. Theora
#compatibility requires that the output of your iDCT routine be bitwise equivalent to the one outlined here:

def idct_1D(data, i, stride):

  A = Mul_fix(64277, data[i+stride]) + Mul_fix(12785, data[i+7*stride])
  B = Mul_fix(12785, data[i+stride]) - Mul_fix(64277, data[i+7*stride])
  C = Mul_fix(54491, data[i+3*stride]) + Mul_fix(36410, data[i+5*stride])
  D = Mul_fix(54491, data[i+5*stride]) - Mul_fix(36410, data[i+3*stride])
  A2 = Mul_fix(46341, A - C)
  B2 = Mul_fix(46341, B - D)
  C2 = A + C
  D2 = B + D
  E = Mul_fix(46341, data[i] + data[i+4*stride])
  F = Mul_fix(46341, data[i] - data[i+4*stride])
  G = Mul_fix(60547, data[i+2*stride]) + Mul_fix(25080, data[i+6*stride])
  H = Mul_fix(25080, data[i+2*stride]) - Mul_fix(60547, data[i+6*stride])
  E2 = E - G
  G2 = E + G
  A3 = F + A2
  B3 = B2 - H
  F2 = F - A2
  H2 = B2 + H

  data[i] = G2 + C2
  data[i+stride] = A3 + H2
  data[i+2*stride] = A3 - H2
  data[i+3*stride] = E2 + D2
  data[i+4*stride] = E2 - D2
  data[i+5*stride] = F2 + B3
  data[i+6*stride] = F2 - B3
  data[i+7*stride] = G2 - C2

#2D iDCT is performed first on rows, then columns (note order can affect lower bits!)

#(Note that we dequantized all coefficients to 4 times their real value. Since each coefficient is run through the
#iDCT twice (horizontal & vertical), the final values must be divided by 16.)

def idct(data):
  for y in range(8):
    idct_1D(data, y*8, 1);

  for x in range(8):
    idct_1D(data, x, 8);

  for x in range(64):
    data[x] = (data[x] + 8) >> 4                            #add for rounding; /16 (remember dequant was *4!)

#loop filter

#the Theora loop filter is run along every horizontal and vertical edge between blocks where one of the blocks is
#coded. In the keyframe case, this means every edge except the borders of the frame. For predicted frames, the only
#edges that are not filtered are those between two uncoded blocks (because they were filtered at some point
#previously, when the block was originally reconstructed)...

#clamping routine

def clampit(n, lo, hi):
  if n < lo:
    return lo
  if n > hi:
    return hi
  return n

#first, define an array of quality-dependent filter parameters:

loopfilter_array = [
  30, 25, 20, 20, 15, 15, 14, 14,
  13, 13, 12, 12, 11, 11, 10, 10,
  9,  9,  8,  8,  7,  7,  7,  7,
  6,  6,  6,  6,  5,  5,  5,  5,
  4,  4,  4,  4,  3,  3,  3,  3,
  2,  2,  2,  2,  2,  2,  2,  2,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0 ]

#next, we define a step function with five linear segments:

def loopfunc(n):
  global loopfilter_array
  global quality_index                                      #from the frame header
  K = loopfilter_array[quality_index]
  if (n <= -K*2) | (n >= K*2):
    return 0
  elif (n > -K*2) & (n < -K):
    return -2*K - n
  elif (n >= -K) & (n <= K):
    return n
  else:
    return 2*K - n

#now the 1D filter functions:

def filter_horiz(pixmap, x, y):
  for i in range(8):

    A = pixmap[x-2][y+i]
    B = pixmap[x-1][y+i]
    C = pixmap[x][y+i]
    D = pixmap[x+1][y+i]

    A = clampit(A, -128, 127)
    B = clampit(B, -128, 127)
    C = clampit(C, -128, 127)
    D = clampit(D, -128, 127)

    N = ( 4+(A - B*3 + C*3 - D)) >> 3
    delta = loopfunc(N)

    pixmap[x-1][y+i] = B + delta
    pixmap[x][y+i] = C - delta

def filter_vert(pixmap, x, y):

  for i in range(8):

    A = pixmap[x+i][y-2]
    B = pixmap[x+i][y-1]
    C = pixmap[x+i][y]
    D = pixmap[x+i][y+1]

    A = clampit(A, -128, 127)
    B = clampit(B, -128, 127)
    C = clampit(C, -128, 127)
    D = clampit(D, -128, 127)

    N = ( 4+(A - B*3 + C*3 - D)) >> 3
    delta = loopfunc(N)

    pixmap[x+i][y-1] = B + delta
    pixmap[x+i][y] = C - delta

#full filter (keyframe case only):

def loopfilter(pixmap, w, h):
  for y in range(h>>3):
    for x in range(w>>3):
      xx = x*8
      yy = y*8
      if xx > 0:                                      #vertical if not on left edge
        filter_horiz(pixmap, xx, yy)
      if yy > 0:                                      #horiz if not on top row
        filter_vert(pixmap, xx, yy)

#Pixel management routines

#this routine converts an array of 8x8 blocks of data into a pixel array

def blocks2pixels(data, w, h, dx, dy):
  pix = [ [0 for y in range(h)] for x in range(w) ]         #initialize 2D array, pix[w][h]

  for y in range(h):
    for x in range(w):
      xx = x+dx
      yy = y+dy
      bx = xx >> 3
      by = yy >> 3
      ix = xx % 8
      iy = yy % 8
      p = data[bx][by][ix + iy*8]
      pix[x][y] = p
  return pix

#one last helper -- turns a color map (integers with x, y coordinates) into a straight block array, clamped to 0-255:

def pixels2chars(pixels):
  h = len(pixels[0])
  w = len(pixels)

  data = []                                                 #initialze linear array width * height long

  for y in range(h):
    for x in range(w):
      p = pixels[x][y]
      p += 128
      p = clampit(p, 0, 255)
      data.append(p)                                        #add it to the list
  return data

# *** DECODING THE FRAME ***

#ok, let's do it!

def decode_frame():
  print
  print "DECODING FRAME"
  global quality_index, infoflag, tableflag

  if (infoflag == 0) | (tableflag == 0):                    #if info & table not initialized
    print "stream parameters not initialized -- missing info or table headers?"
    abort()

#First, we decode the frame header:

  is_predicted = readbit()
  print "is_predicted:", is_predicted
  quality_index = readbits(6)
  print "quality_index =", quality_index
  scalefactor_AC = scale_table_AC[quality_index]            #(ThisFrameQualityValue in C)
  print "scalefactor_AC =", scalefactor_AC
  scalefactor_DC = scale_table_DC[quality_index]
  print "scalefactor_DC =", scalefactor_DC

  if is_predicted == 0:                                     #0 = keyframe, 1 = interframe

#OK, this is a keyframe.  That means we just have 'intra' coded blocks.
    print "decoding keyframe"
    keyframe_type = readbit()                               #keyframe type always == 1 (DCT) (for now)
    readbits(2)                                             #2 unused bits

#compute some values based on width & height:
    blocks_Y = int( (encoded_height*encoded_width)/64 )     #Y blocks coded
    blocks_UV = int(blocks_Y / 2)
    blocks_U = int(blocks_UV / 2)
    blocks_V = blocks_U
    blocks_tot = int(blocks_Y * 1.5)                        #Y and UV blocks coded

#initialize a map of coefficients.  For each coded block, we will eventually have 64:
    global coeffs
    coeffs = [ [] for x in range(blocks_tot) ]

#Theora encodes each coefficient for every block in sequence. IE first we decode all the DC coefficients; then
#coefficient #1, then #2, and so on.
#Also, as we go higher into the coefficient index, we will use different huffman tables:

    for i in range(64):                                     #For each coefficient index,
      if i == 0:                                            #get DC huffman tables for coeff=0
        huff_Y = readbits(4)
        huff_UV = readbits(4)
      elif i == 1:
        huff_Y = readbits(4)+16                             #get AC huff tables at 1, 6, 15, and 28
        huff_UV = readbits(4)+16
      elif i == 6:
        huff_Y += 16
        huff_UV += 16
      elif i == 15:
        huff_Y += 16
        huff_UV += 16
      elif i == 28:
        huff_Y += 16
        huff_UV += 16

#now, for every block, decode our coefficient:

      for x in range(blocks_tot):
        if x < blocks_Y:                                    #if we're in the Y plane,
          huff = huff_Y
        else:
          huff = huff_UV

#first check whether this coefficient was already decoded (Because of an end-of-block or other run):

        if len(coeffs[x]) <= i:                             #if this coeff has not been set

#if not, get a token:

          run, val = parsetoken(huffs[huff])

#if this is an end-of-block token, that means we have a run of blocks to mark as fully decoded:

          if val == 'eob':                                  #eob = End Of Block run
            xx = x                                          #temporary block index starts with x
            for r in range(run):                            #clear (run) blocks
              done = len(coeffs[xx])                        #this many coeffs are set in this block
              remain = 64 - done                            #this many remain
              for j in range (remain):                      #for all remaining coeffs
                coeffs[xx].append(0)                        #set to zero
              ii = i                                        #temporary coeff index starts with i
              while (len(coeffs[xx]) >ii) and ii<64:        #find next candidate block for eob treatment
                xx += 1                                     #next block
                if xx == blocks_tot:                        #if we wrapped around,
                  xx = 0                                    #back to block zero
                  ii += 1                                   #and next coeff

#otherwise the token represents a run of zeros followed by a value:
          else:                                             #zero run + value
            for r in range (run):                           #a run of zeros
              coeffs[x].append(0)
            coeffs[x].append(val)                           #followed by a val

#now 'de-hilbertize' coefficient blocks:

    Yheight = int(encoded_height/8)
    Ywidth = int(encoded_width/8)
    UVheight = int(Yheight/2)
    UVwidth = int(Ywidth/2)
    global comapY, comapU, comapV
    comapY = de_hilbert(Ywidth, Yheight, coeffs)
    comapU = de_hilbert(UVwidth, UVheight, coeffs[blocks_Y:])
    comapV = de_hilbert(UVwidth, UVheight, coeffs[(blocks_Y+blocks_U):])

#next, we need to reverse the DC prediction-based delta coding:

    DCpredict(comapY, Ywidth, Yheight)
    DCpredict(comapV, UVwidth, UVheight)
    DCpredict(comapU, UVwidth, UVheight)

#finally reorder, dequantize, and iDCT all the coefficients, for each color plane:

    temp = [0 for x in range(64)]                           #temporary array

    for y in range(Yheight):
      for x in range(Ywidth):
        unzig (comapY[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, Y_quantizer)
        idct(temp)
        comapY[x][y] = temp + []                            #Python quirk -- force assignment by copy (not reference)

    for y in range(UVheight):
      for x in range(UVwidth):
        unzig (comapU[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, UV_quantizer)
        idct(temp)
        comapU[x][y] = temp + []

    for y in range(UVheight):
      for x in range(UVwidth):
        unzig (comapV[x][y], temp)
        dequant(temp, scalefactor_DC, scalefactor_AC, UV_quantizer)
        idct(temp)
        comapV[x][y] = temp + []

#convert the image into a raw byte array of planar Y, V, U:

    pixY = blocks2pixels(comapY, decode_width, decode_height, offset_x, offset_y)
    pixU = blocks2pixels(comapU, decode_width>>1, decode_height>>1, offset_x>>1, offset_y>>1)
    pixV = blocks2pixels(comapV, decode_width>>1, decode_height>>1, offset_x>>1, offset_y>>1)

#run loop filter:

    loopfilter(pixY, decode_width, decode_height)
    loopfilter(pixU, decode_width>>1, decode_height>>1)
    loopfilter(pixV, decode_width>>1, decode_height>>1)

#return the three color planes:

    return pixels2chars(pixY), pixels2chars(pixU), pixels2chars(pixV)

#Decode Predicted Frame:                THIS SECTION UNFINISHED
  else: 
    print "decoding interframe (NOT!)"
    abort()
    coding_scheme = readbits(3)
    if coding_scheme == 0:
      mode_alphabet = []                                    #define a list (think of it as an array)
      for x in range(8):
        mode_alphabet.append(readbits(3))                   #add another mode to the list

#--end of definition for decode_frame()


#Parse Theora packets

#Define a function to parse the packet type & call appropriate functions. Returns either a string for header packets,
#or a tuple of Y, U, and V data for frames:

def decode_packet():
  packet_type = readbit()
  if packet_type == 0:
    return decode_frame()
  else:
    return decode_header()

#MAIN TEST SEQUENCE

#let's test our routines by parsing the stream headers and the first frame.

print
print "THEORA SPEC PYTHON SCRIPT"
print "Test: decoding first frame of", sys.argv[1]
ret = ""

while type(ret) == type(""):                          #string means stream header parsed
  ret = decode_packet()
  if type(ret) == type(""):                           #if it's a header packet,
    print "header packet type:", ret                  #print the type (info, comment, tables)
    if ret == "info":
      print "  version:", version_major, version_minor, version_subminor 
      print "  encoded width:", encoded_width
      print "  encoded height:", encoded_height
      print "  decode width:", decode_width
      print "  decode height:", decode_height
      print "  X offset:", offset_x
      print "  Y offset:", offset_y
      print "  fps:", fps_numerator, "/", fps_denominator
      print "  aspect:", aspect_numerator, "/", aspect_denominator
      print "  colorspace:",
      if colorspace == 0:
        print "  not specified"
      elif colorspace == 1:
        print "  ITU 601"
      elif colorspace == 2:
        print "  CIE 709"
      else:
        print "  colorspace type not recognized"
      print "  target bitrate:", bitrate
      print "  target quality:", quality

    elif ret == "comment":
      print "  vendor string:", vendor_string
      print "  comment length:", comment_string_len
    elif ret == "table":
      print "tables loaded"
  else:
    print "frame decoded"

#'ret' should now have the first frame:

Y, U, V = ret

#define a little routine to fix up the UV buffer the way we like:

def interleave(U, V, w, h):
  buf = []
  for y in range(h):
    for x in range(w):
      buf.append( V[y*w + x] )
    for x in range(w):
      buf.append( U[y*w + x] )
  return buf

#write data to disk:

buf = array('B', Y + interleave(U, V, decode_width/2, decode_height/2) )
outfile = file(sys.argv[2],"wb")
outfile.write(buf)

#that's all for now.
print "done"

 