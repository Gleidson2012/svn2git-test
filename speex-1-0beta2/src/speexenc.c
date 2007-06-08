/* Copyright (C) 2002 Jean-Marc Valin 
   File: speexenc.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#if !defined WIN32 && !defined _WIN32
#include <unistd.h>
#include <getopt.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "speex.h"
#include <ogg/ogg.h>
#include "wav_io.h"
#include "speex_header.h"
#include "misc.h"

#if defined WIN32 || defined _WIN32
#include "getopt_win.h"
/* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif


void comment_init(char **comments, int* length, char *vendor_string);
void comment_add(char **comments, int* length, char *tag, char *val);


/*Write an Ogg page to a file pointer*/
int oe_write_page(ogg_page *page, FILE *fp)
{
   int written;
   written = fwrite(page->header,1,page->header_len, fp);
   written += fwrite(page->body,1,page->body_len, fp);
   
   return written;
}

#define MAX_FRAME_SIZE 2000
#define MAX_FRAME_BYTES 2000

/* Convert input audio bits, endians and channels */
int read_samples(FILE *fin,int frame_size, int bits, int channels, int lsb, float * input)
{   
   unsigned char in[MAX_FRAME_BYTES*2];
   int i,d;
   short *s;

   /*Read input audio*/
   fread(in,bits/8*channels, frame_size, fin);
   if (feof(fin))
      return 1;
   s=(short*)in;
   if(bits==8)
   {
      /* Convert 8->16 bits */
      for(i=frame_size*channels-1;i>=0;i--)
      {
         s[i]=(in[i]<<8)^0x8000;
      }
   } else
   {
      /* convert to our endian format */
      for(i=0;i<frame_size*channels;i++)
      {
         if(lsb) 
            s[i]=le_short(s[i]); 
         else
            s[i]=be_short(s[i]);
      }
   }

   if(channels==2)
   {
      /* downmix to mono */
      for(i=0;i<frame_size;i++)
      {
         d=s[i*2]+s[i*2+1];
         s[i]=d>>1;
      }
   }

   /* copy to float input buffer */
   for (i=0;i<frame_size;i++)
   {
      input[i]=(short)s[i];
   }

   return 0;
}

void usage()
{
   /*printf ("Speex encoder version " VERSION " (compiled " __DATE__ ")\n");
   printf ("\n");*/
   printf ("Usage: speexenc [options] input_file output_file\n");
   printf ("\n");
   printf ("Encodes input_file using Speex. It can read the WAV or raw files.\n");
   printf ("\n");
   printf ("input_file can be:\n");
   printf ("  filename.wav          wav file\n");
   printf ("  filename.*            raw PCM file (any extension other than .wav)\n");
   printf ("  -                     stdin\n");
   printf ("\n");  
   printf ("output_file can be:\n");
   printf ("  filename.spx          Speex file\n");
   printf ("  -                     stdout\n");
   printf ("\n");  
   printf ("Options:\n");
   printf (" -n, --narrowband   Narrowband (8 kHz) input file\n"); 
   printf (" -w, --wideband     Wideband (16 kHz) input file\n"); 
   printf (" --quality n        Encoding quality (0-10), default 3\n"); 
   printf (" --lbr              Low bit-rate mode (equivalent to --quality 3)\n"); 
   printf (" --vbr              Enable variable bit-rate (VBR)\n"); 
   printf (" --comp n           Set encoding complexity (0-10), default 3\n"); 
   printf (" --nframes n        Number of frames per Ogg packet (1-10), default 1\n"); 
   printf (" --comment          Add the given string as an extra comment. This may be\n                     used multiple times.\n");
   printf (" --author           Author of this track.\n");
   printf (" --title            Title for this track.\n");
   printf (" -h, --help         This help\n"); 
   printf (" -v, --version      Version information\n"); 
   printf (" -V                 Verbose mode (show bit-rate)\n"); 
   printf ("Raw input options:\n");
   printf (" --le               Raw input is little-endian\n"); 
   printf (" --be               Raw input is big-endian\n"); 
   printf (" --8bit             Raw input is 8-bit unsigned\n"); 
   printf (" --16bit            Raw input is 16-bit signed\n"); 
   printf ("\n");  
   printf ("Default Raw PCM input is 16-bit, little-endian, mono\n"); 
}

void version()
{
   printf ("Speex encoder version " VERSION " (compiled " __DATE__ ")\n");
}

int main(int argc, char **argv)
{
   int c;
   int option_index = 0;
   int narrowband=0, wideband=0;
   char *inFile, *outFile;
   FILE *fin, *fout;
   float input[MAX_FRAME_SIZE];
   int frame_size;
   int vbr_enabled=0;
   int nbBytes;
   SpeexMode *mode=NULL;
   void *st;
   SpeexBits bits;
   char cbits[MAX_FRAME_BYTES];
   struct option long_options[] =
   {
      {"wideband", no_argument, NULL, 0},
      {"narrowband", no_argument, NULL, 0},
      {"lbr", no_argument, NULL, 0},
      {"vbr", no_argument, NULL, 0},
      {"quality", required_argument, NULL, 0},
      {"nframes", required_argument, NULL, 0},
      {"comp", required_argument, NULL, 0},
      {"help", no_argument, NULL, 0},
      {"le", no_argument, NULL, 0},
      {"be", no_argument, NULL, 0},
      {"lin8", no_argument, NULL, 0},
      {"lin16", no_argument, NULL, 0},
      {"version", no_argument, NULL, 0},
      {"comment", required_argument, NULL, 0},
      {"author", required_argument, NULL, 0},
      {"title", required_argument, NULL, 0},
      {0, 0, 0, 0}
   };
   int print_bitrate=0;
   int rate, size;
   int chan=1;
   int fmt=16;
   int quality=-1;
   int lbr=0;
   int lsb=1;
   ogg_stream_state os;
   ogg_page 		 og;
   ogg_packet 		 op;
   int bytes_written, ret, result;
   int id=-1;
   SpeexHeader header;
   int nframes=1;
   int complexity=3;
   char *vendor_string = "Encoded with Speex " VERSION;
   char *comments;
   int comments_length;
   int close_in=0, close_out=0;
   int eos=0;

   comment_init(&comments, &comments_length, vendor_string);

   /*Process command-line options*/
   while(1)
   {
      c = getopt_long (argc, argv, "nwhvV",
                       long_options, &option_index);
      if (c==-1)
         break;
      
      switch(c)
      {
      case 0:
         if (strcmp(long_options[option_index].name,"narrowband")==0)
            narrowband=1;
         else if (strcmp(long_options[option_index].name,"wideband")==0)
               wideband=1;
         else if (strcmp(long_options[option_index].name,"lbr")==0)
               lbr=1;
         else if (strcmp(long_options[option_index].name,"vbr")==0)
               vbr_enabled=1;
         else if (strcmp(long_options[option_index].name,"quality")==0)
         {
            quality = atoi (optarg);
         } else if (strcmp(long_options[option_index].name,"nframes")==0)
         {
            nframes = atoi (optarg);
            if (nframes<1)
               nframes=1;
            if (nframes>10)
               nframes=10;
         } else if (strcmp(long_options[option_index].name,"comp")==0)
         {
            complexity = atoi (optarg);
         } else if (strcmp(long_options[option_index].name,"help")==0)
         {
            usage();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"version")==0)
         {
            version();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"le")==0)
         {
            lsb=1;
         } else if (strcmp(long_options[option_index].name,"be")==0)
         {
            lsb=0;
         } else if (strcmp(long_options[option_index].name,"lin8")==0)
         {
            fmt=8;
         } else if (strcmp(long_options[option_index].name,"lin16")==0)
         {
            fmt=16;
         } else if (strcmp(long_options[option_index].name,"comment")==0)
         {
           comment_add(&comments, &comments_length, NULL, optarg); 
         } else if (strcmp(long_options[option_index].name,"author")==0)
         {
           comment_add(&comments, &comments_length, "author=", optarg); 
         } else if (strcmp(long_options[option_index].name,"title")==0)
         {
           comment_add(&comments, &comments_length, "title=", optarg); 
         }

         break;
      case 'n':
         narrowband=1;
         break;
      case 'h':
         usage();
         exit(0);
         break;
      case 'v':
         version();
         exit(0);
         break;
      case 'V':
         print_bitrate=1;
         break;
      case 'w':
         wideband=1;
         break;
      case '?':
         usage();
         exit(1);
         break;
      }
   }
   if (argc-optind!=2)
   {
      usage();
      exit(1);
   }
   inFile=argv[optind];
   outFile=argv[optind+1];

   if (wideband && narrowband)
   {
      fprintf (stderr,"Cannot specify both wideband and narrowband at the same time\n");
      exit(1);
   };

   /*Initialize Ogg stream struct*/
   srand(time(NULL));
   if (ogg_stream_init(&os, rand())==-1)
   {
      fprintf(stderr,"Stream init failed\n");
      exit(1);
   }

   if (strcmp(inFile, "-")==0)
   {
#if defined WIN32 || defined _WIN32
         _setmode(_fileno(stdout), _O_BINARY);
#endif
      fin=stdin;
   }
   else 
   {
#if defined WIN32 || defined _WIN32
      fin = fopen(inFile, "rb");
#else
      fin = fopen(inFile, "r");
#endif
      if (!fin)
      {
         perror(inFile);
         exit(1);
      }
      close_in=1;
   }

   rate=0;
   if (strcmp(inFile+strlen(inFile)-4,".wav")==0 || strcmp(inFile+strlen(inFile)-4,".WAV")==0)
      {
         if (read_wav_header(fin, &rate, &chan, &fmt, &size)==-1)
            exit(1);
	 lsb=1; /* CHECK: exists big-endian .wav ?? */
      }
   /*fprintf (stderr, "wave info: %d %d %d %d\n", rate, chan, fmt, size);*/

   if (rate==16000)
   {
      wideband=1;
      if (narrowband)
         fprintf (stderr,"Warning: encoding a wideband file in narrowband\n");
   } else if (rate==8000)
   {
      narrowband=1;
      if (wideband)
         fprintf (stderr,"Warning: encoding a narrowband file in wideband\n");
   } else if (rate==22050)
   {
      wideband=1;
      fprintf (stderr,"Warning: Speex is not optimized for 22.05 kHz sampling rate. Your mileage may vary\n");
      if (narrowband)
         fprintf (stderr,"Warning: encoding a wideband file in narrowband\n");
   } else if (rate==11025)
   {
      fprintf (stderr,"Warning: Speex is not optimized for 11.025 kHz sampling rate. Your mileage may vary\n");
      narrowband=1;
      if (wideband)
         fprintf (stderr,"Warning: encoding a narrowband file in wideband\n");
   }

   if (!wideband)
      narrowband=1;
   if (narrowband)
   {
      if (!rate)
         rate = 8000;
      mode=&speex_nb_mode;
   }
   if (wideband)
   {
      if (!rate)
         rate = 16000;
      mode=&speex_wb_mode;
   }

   speex_init_header(&header, rate, 1, mode);
   header.frames_per_packet=nframes;
   header.vbr=vbr_enabled;

   fprintf (stderr, "Encoding %d Hz audio using %s mode\n", 
            header.rate, mode->modeName);
   /*fprintf (stderr, "Encoding %d Hz audio at %d bps using %s mode\n", 
     header.rate, mode->bitrate, mode->modeName);*/

   /*Initialize Speex encoder*/
   st = speex_encoder_init(mode);

   if (strcmp(outFile,"-")==0)
   {
#if defined WIN32 || defined _WIN32
      _setmode(_fileno(stdout), _O_BINARY);
#endif
      fout=stdout;
   }
   else 
   {
#if defined WIN32 || defined _WIN32
      fout = fopen(outFile, "wb");
#else
      fout = fopen(outFile, "w");
#endif
      if (!fout)
      {
         perror(outFile);
         exit(1);
      }
      close_out=1;
   }


   /*Write header (format will change)*/
   {

      op.packet = (unsigned char *)speex_header_to_packet(&header, (int*)&(op.bytes));
      op.b_o_s = 1;
      op.e_o_s = 0;
      op.granulepos = 0;
      op.packetno = 0;
      ogg_stream_packetin(&os, &op);
      free(op.packet);

      op.packet = (unsigned char *)comments;
      op.bytes = comments_length;
      op.b_o_s = 0;
      op.e_o_s = 0;
      op.granulepos = 0;
      op.packetno = 1;
      ogg_stream_packetin(&os, &op);
      
      while((result = ogg_stream_flush(&os, &og)))
      {
         if(!result) break;
         ret = oe_write_page(&og, fout);
         if(ret != og.header_len + og.body_len)
         {
            fprintf (stderr,"Failed writing header to output stream\n");
            exit(1);
         }
         else
            bytes_written += ret;
      }
   }

   free(comments);

   speex_encoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);
   speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &complexity);
   if (vbr_enabled)
   {
      int tmp;
      tmp=1;
      speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
   }
   if (lbr || quality != -1)
   {
      int tmp=quality;
      if (quality==-1)
         tmp = 3;
      speex_encoder_ctl(st, SPEEX_SET_QUALITY, &tmp);
      if (vbr_enabled)
         speex_encoder_ctl(st, SPEEX_SET_VBR_QUALITY, &tmp);
   }

   speex_bits_init(&bits);

   if (read_samples(fin,frame_size,fmt,chan,lsb,input))
      eos=1;

   /*Main encoding loop (one frame per iteration)*/
   while (!eos)
   {
      id++;
      /*Encode current frame*/
      speex_encode(st, input, &bits);
      
      if (print_bitrate) {
         int tmp;
         char ch=13;
         speex_encoder_ctl(st, SPEEX_GET_BITRATE, &tmp);
         fputc (ch, stderr);
         fprintf (stderr, "Bitrate is use: %d bps     ", tmp);
      }

      if (read_samples(fin,frame_size,fmt,chan,lsb,input))
      {
         eos=1;
         op.e_o_s = 1;
      }

      if ((id+1)%nframes!=0)
         continue;
      nbBytes = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
      speex_bits_reset(&bits);
      op.packet = (unsigned char *)cbits;
      op.bytes = nbBytes;
      op.b_o_s = 0;
      if (eos)
         op.e_o_s = 1;
      else
         op.e_o_s = 0;
      op.granulepos = (id+nframes)*frame_size;
      op.packetno = 2+id/nframes;
      ogg_stream_packetin(&os, &op);

      /*Write all new pages (most likely 0 or 1)*/
      while (ogg_stream_pageout(&os,&og))
      {
         ret = oe_write_page(&og, fout);
         if(ret != og.header_len + og.body_len)
         {
            fprintf (stderr,"Failed writing header to output stream\n");
            exit(1);
         }
         else
            bytes_written += ret;
      }
   }
   if ((id+1)%nframes!=0)
   {
      while ((id+1)%nframes!=0)
      {
         id++;
         speex_bits_pack(&bits, 0, 7);
      }
      nbBytes = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
      op.packet = (unsigned char *)cbits;
      op.bytes = nbBytes;
      op.b_o_s = 0;
      op.e_o_s = 1;
      op.granulepos = (id+nframes)*frame_size;
      op.packetno = 2+id/nframes;
      ogg_stream_packetin(&os, &op);
   }
   /*Flush all pages left to be written*/
   while (ogg_stream_flush(&os, &og))
   {
      ret = oe_write_page(&og, fout);
      if(ret != og.header_len + og.body_len)
      {
         fprintf (stderr,"Failed writing header to output stream\n");
         exit(1);
      }
      else
         bytes_written += ret;
   }
   

   speex_encoder_destroy(st);
   speex_bits_destroy(&bits);
   ogg_stream_clear(&os);

   if (close_in)
      fclose(fin);
   if (close_out)
      fclose(fout);
   return 1;
}

/*                 
 Comments will be stored in the Vorbis style.            
 It is describled in the "Structure" section of
    http://www.xiph.org/ogg/vorbis/doc/v-comment.html

The comment header is decoded as follows:
  1) [vendor_length] = read an unsigned integer of 32 bits
  2) [vendor_string] = read a UTF-8 vector as [vendor_length] octets
  3) [user_comment_list_length] = read an unsigned integer of 32 bits
  4) iterate [user_comment_list_length] times {
     5) [length] = read an unsigned integer of 32 bits
     6) this iteration's user comment = read a UTF-8 vector as [length] octets
     }
  7) [framing_bit] = read a single bit as boolean
  8) if ( [framing_bit]  unset or end of packet ) then ERROR
  9) done.

  If you have troubles, please write to ymnk@jcraft.com.
 */

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
  	           	    (buf[base]&0xff))
#define writeint(buf, base, val) do{ buf[base+3]=((val)>>24)&0xff; \
                                     buf[base+2]=((val)>>16)&0xff; \
                                     buf[base+1]=((val)>>8)&0xff; \
                                     buf[base]=(val)&0xff; \
                                 }while(0)

void comment_init(char **comments, int* length, char *vendor_string)
{
  int vendor_length=strlen(vendor_string);
  int user_comment_list_length=0;
  int len=4+vendor_length+4;
  char *p=(char*)malloc(len);
  if(p==NULL){
  }
  writeint(p, 0, vendor_length);
  memcpy(p+4, vendor_string, vendor_length);
  writeint(p, 4+vendor_length, user_comment_list_length);
  *length=len;
  *comments=p;
}
void comment_add(char **comments, int* length, char *tag, char *val)
{
  char* p=*comments;
  int vendor_length=readint(p, 0);
  int user_comment_list_length=readint(p, 4+vendor_length);
  int tag_len=(tag?strlen(tag):0);
  int val_len=strlen(val);
  int len=(*length)+4+tag_len+val_len;

  p=(char*)realloc(p, len);
  if(p==NULL){
  }

  writeint(p, *length, tag_len+val_len);      /* length of comment */
  if(tag) memcpy(p+*length+4, tag, tag_len);  /* comment */
  memcpy(p+*length+4+tag_len, val, val_len);  /* comment */
  writeint(p, 4+vendor_length, user_comment_list_length+1);

  *comments=p;
  *length=len;
}
#undef readint
#undef writeint