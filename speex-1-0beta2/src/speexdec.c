/* Copyright (C) 2002 Jean-Marc Valin 
   File: speexdec.c

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

#include "speex.h"
#include "ogg/ogg.h"

#if defined WIN32 || defined _WIN32
#include <windows.h>
#include "getopt_win.h"
#include "wave_out.h"
/* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

#include <string.h>
#include "wav_io.h"
#include "speex_header.h"
#include "misc.h"

#define MAX_FRAME_SIZE 2000

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
  	           	    (buf[base]&0xff))

static void print_comments(char *comments, int length)
{
   char *c=comments;
   int len, i, nb_fields;

   len=readint(c, 0);
   c+=4;
   fwrite(c, 1, len, stderr);
   c+=len;
   fprintf (stderr, "\n");
   nb_fields=readint(c, 0);
   c+=4;
   for (i=0;i<nb_fields;i++)
   {
      len=readint(c, 0);
      c+=4;
      fwrite(c, 1, len, stderr);
      c+=len;
      fprintf (stderr, "\n");
   }
}

FILE *out_file_open(char *outFile, int rate)
{
   FILE *fout;
   /*Open output file*/
   if (strlen(outFile)==0)
   {
#if defined HAVE_SYS_SOUNDCARD_H
      int audio_fd, format, stereo;
      audio_fd=open("/dev/dsp", O_WRONLY);
      
      format=AFMT_S16_LE;
      if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format)==-1)
      {
         perror("SNDCTL_DSP_SETFMT");
         close(audio_fd);
         exit(1);
      }
      
      stereo=0;
      if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo)==-1)
      {
         perror("SNDCTL_DSP_STEREO");
         close(audio_fd);
         exit(1);
      }
      if (stereo!=0)
      {
         fprintf (stderr, "Cannot set mono mode\n");
         exit(1);
      }

      if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &rate)==-1)
      {
         perror("SNDCTL_DSP_SPEED");
         close(audio_fd);
         exit(1);
      }
      fout = fdopen(audio_fd, "w");
#elif defined WIN32 || defined _WIN32
      if (Set_WIN_Params (INVALID_FILEDESC, rate, SAMPLE_SIZE, 1))
      {
         fprintf (stderr, "Can't access %s\n", "WAVE OUT");
         exit(1);
      }
#else
      fprintf (stderr, "No soundcard support\n");
      exit(1);
#endif
   } else {
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
         if (strcmp(outFile+strlen(outFile)-4,".wav")==0 || strcmp(outFile+strlen(outFile)-4,".WAV")==0)
            write_wav_header(fout, rate, 1, 0, 0);
      }
   }
   return fout;
}

void usage()
{
   /*printf ("Speex decoder version " VERSION " (compiled " __DATE__ ")\n");
   printf ("\n");*/
   printf ("Usage: speexdec [options] input_file.spx\n");
   printf ("       speexdec [options] input_file.spx output_file.wav\n");
   printf ("\n");
   printf ("Decodes a Speex file and produce a WAV file or raw file\n");
   printf ("\n");
   printf ("input_file can be:\n");
   printf ("  filename.spx          regular Speex file\n");
   printf ("  -                     stdin\n");
   printf ("\n");  
   printf ("output_file can be:\n");
   printf ("  filename.wav          wav file\n");
   printf ("  filename.*            raw PCM file (any extension other that .wav)\n");
   printf ("  -                     stdout\n");
   printf ("  (nothing)             will be played to soundcard\n");
   printf ("\n");  
   printf ("Options:\n");
   printf (" --enh                 Enable perceptual enhancement\n");
   printf (" --no-enh              Disable perceptual enhancement (default FOR NOW)\n");
   printf (" --force-nb            Force decoding in narrowband, even for wideband\n");
   printf (" --force-wb            Force decoding in wideband, even for narrowband\n");
   printf (" --packet-loss n       Simulate n %% random packet loss\n");
   printf (" -V                    Verbose mode (show bit-rate)\n"); 
   printf (" -h, --help            This help\n");
   printf (" -v, --version         Version information\n");
   printf (" --pf                  Deprecated, use --pf instead\n");
   printf (" --no-pf               Deprecated, use --no-pf instead\n");
}

void version()
{
   printf ("Speex decoder version " VERSION " (compiled " __DATE__ ")\n");
}

static void *process_header(ogg_packet *op, int enh_enabled, int *frame_size, int *rate, int *nframes, int forceMode)
{
   void *st;
   SpeexMode *mode;
   SpeexHeader *header;
   int modeID;
   
   header = speex_packet_to_header((char*)op->packet, op->bytes);
   if (!header)
   {
      fprintf (stderr, "Cannot read header\n");
      return NULL;
   }
   if (header->mode >= SPEEX_NB_MODES)
   {
      fprintf (stderr, "Mode number %d does not (any longer) exist in this version\n", 
               header->mode);
      return NULL;
   }
      
   modeID = header->mode;
   if (forceMode!=-1)
      modeID = forceMode;
   mode = speex_mode_list[modeID];
   
   if (mode->bitstream_version < header->mode_bitstream_version)
   {
      fprintf (stderr, "The file was encoded with a newer version of Speex. You need to upgrade in order to play it.\n");
      return NULL;
   }
   if (mode->bitstream_version > header->mode_bitstream_version) 
   {
      fprintf (stderr, "The file was encoded with an older version of Speex. You would need to downgrade the version in order to play it.\n");
      return NULL;
   }
   
   st = speex_decoder_init(mode);
   speex_decoder_ctl(st, SPEEX_SET_ENH, &enh_enabled);
   speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);
   
   /* FIXME: need to adjust in case the forceMode option is set */
   *rate = header->rate;
   if (header->mode==1 && forceMode==0)
      *rate/=2;
   if (header->mode==0 && forceMode==1)
      *rate*=2;
   *nframes = header->frames_per_packet;
   
   fprintf (stderr, "Decoding %d Hz audio using %s mode", 
            *rate, mode->modeName);

   if (header->vbr)
      fprintf (stderr, " (VBR)\n");
   else
      fprintf(stderr, "\n");
   /*fprintf (stderr, "Decoding %d Hz audio at %d bps using %s mode\n", 
    *rate, mode->bitrate, mode->modeName);*/

   free(header);
   return st;
}

int main(int argc, char **argv)
{
   int c;
   int option_index = 0;
   char *inFile, *outFile;
   FILE *fin, *fout=NULL;
   short out[MAX_FRAME_SIZE];
   float output[MAX_FRAME_SIZE];
   int frame_size=0;
   void *st=NULL;
   SpeexBits bits;
   int packet_count=0;
   int stream_init = 0;
   struct option long_options[] =
   {
      {"help", no_argument, NULL, 0},
      {"version", no_argument, NULL, 0},
      {"enh", no_argument, NULL, 0},
      {"no-enh", no_argument, NULL, 0},
      {"pf", no_argument, NULL, 0},
      {"no-pf", no_argument, NULL, 0},
      {"force-nb", no_argument, NULL, 0},
      {"force-wb", no_argument, NULL, 0},
      {"packet-loss", required_argument, NULL, 0},
      {0, 0, 0, 0}
   };
   ogg_sync_state oy;
   ogg_page       og;
   ogg_packet     op;
   ogg_stream_state os;
   int enh_enabled;
   int nframes=2;
   int print_bitrate=0;
   int close_in=0;
   int eos=0;
   int forceMode=-1;
   int audio_size=0;
   float loss_percent=-1;

   enh_enabled = 0;

   /*Process options*/
   while(1)
   {
      c = getopt_long (argc, argv, "hvV",
                       long_options, &option_index);
      if (c==-1)
         break;
      
      switch(c)
      {
      case 0:
         if (strcmp(long_options[option_index].name,"help")==0)
         {
            usage();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"version")==0)
         {
            version();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"enh")==0)
         {
            enh_enabled=1;
         } else if (strcmp(long_options[option_index].name,"no-enh")==0)
         {
            enh_enabled=0;
         } else if (strcmp(long_options[option_index].name,"pf")==0)
         {
            fprintf (stderr, "--pf is deprecated, use --enh instead\n");
            enh_enabled=1;
         } else if (strcmp(long_options[option_index].name,"no-pf")==0)
         {
            fprintf (stderr, "--no-pf is deprecated, use --no-enh instead\n");
            enh_enabled=0;
         } else if (strcmp(long_options[option_index].name,"force-nb")==0)
         {
            forceMode=0;
         } else if (strcmp(long_options[option_index].name,"force-wb")==0)
         {
            forceMode=1;
         } else if (strcmp(long_options[option_index].name,"packet-loss")==0)
         {
            loss_percent = atof(optarg);
         }
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
      case '?':
         usage();
         exit(1);
         break;
      }
   }
   if (argc-optind!=2 && argc-optind!=1)
   {
      usage();
      exit(1);
   }
   inFile=argv[optind];

   if (argc-optind==2)
      outFile=argv[optind+1];
   else
      outFile = "";
   /*Open input file*/
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


   /*Init Ogg data struct*/
   ogg_sync_init(&oy);
   
   speex_bits_init(&bits);
   /*Main decoding loop*/
   while (1)
   {
      char *data;
      int i, j, nb_read;
      /*Get the ogg buffer for writing*/
      data = ogg_sync_buffer(&oy, 200);
      /*Read bitstream from input file*/
      nb_read = fread(data, sizeof(char), 200, fin);      
      ogg_sync_wrote(&oy, nb_read);

      /*Loop for all complete pages we got (most likely only one)*/
      while (ogg_sync_pageout(&oy, &og)==1)
      {
         if (stream_init == 0) {
            ogg_stream_init(&os, ogg_page_serialno(&og));
            stream_init = 1;
         }
         /*Add page to the bitstream*/
         ogg_stream_pagein(&os, &og);
         /*Extract all available packets*/
         while (!eos && ogg_stream_packetout(&os, &op)==1)
         {
            /*If first packet, process as Speex header*/
            if (packet_count==0)
            {
               int rate;
               st = process_header(&op, enh_enabled, &frame_size, &rate, &nframes, forceMode);
               if (!nframes)
                  nframes=1;
               if (!st)
                  exit(1);
               fout = out_file_open(outFile, rate);

            } else if (packet_count==1){
               print_comments((char*)op.packet, op.bytes);
               /*
               fprintf (stderr, "File comments: ");
               fwrite(op.packet, 1, op.bytes, stderr);
               fprintf (stderr, "\n");
               */
            } else {
               
               int lost=0;
               if (loss_percent>0 && 100*((float)rand())/RAND_MAX<loss_percent)
                  lost=1;

               /*End of stream condition*/
               if (op.e_o_s)
                  eos=1;

               /*Copy Ogg packet to Speex bitstream*/
               speex_bits_read_from(&bits, (char*)op.packet, op.bytes);
               for (j=0;j<nframes;j++)
               {
                  /*Decode frame*/
                  if (!lost)
                     speex_decode(st, &bits, output);
                  else
                     speex_decode(st, NULL, output);

                  if (print_bitrate) {
                     int tmp;
                     char ch=13;
                     speex_decoder_ctl(st, SPEEX_GET_BITRATE, &tmp);
                     fputc (ch, stderr);
                     fprintf (stderr, "Bitrate is use: %d bps     ", tmp);
                  }
                  /*PCM saturation (just in case)*/
                  for (i=0;i<frame_size;i++)
                  {
                     if (output[i]>32000.0)
                        output[i]=32000.0;
                     else if (output[i]<-32000.0)
                        output[i]=-32000.0;
                  }
                  /*Convert to short and save to output file*/
                  for (i=0;i<frame_size;i++)
                     out[i]=(short)le_short((short)output[i]);
#if defined WIN32 || defined _WIN32
                  if (strlen(outFile)==0)
                      WIN_Play_Samples (out, sizeof(short) * frame_size);
                  else
#endif
                  fwrite(out, sizeof(short), frame_size, fout);
                  audio_size+=sizeof(short)*frame_size;
               }
            }
            packet_count++;
         }
      }
      if (feof(fin))
         break;

   }

   if (strcmp(outFile+strlen(outFile)-4,".wav")==0 || strcmp(inFile+strlen(inFile)-4,".WAV")==0)
   {
      if (fseek(fout,4,SEEK_SET)==0)
      {
         int tmp;
         tmp = le_int(audio_size+36);
         fwrite(&tmp,4,1,fout);
         if (fseek(fout,32,SEEK_CUR)==0)
         {
            tmp = le_int(audio_size);
            fwrite(&tmp,4,1,fout);
         } else
         {
            fprintf (stderr, "First seek worked, second didn't\n");
         }
      } else {
         fprintf (stderr, "Cannot seek on wave file, size will be incorrect\n");
      }
   }

   if (st)
      speex_decoder_destroy(st);
   speex_bits_destroy(&bits);
   ogg_sync_clear(&oy);
   ogg_stream_clear(&os);

   if (close_in)
      fclose(fin);
   fclose(fout);
   return 1;
}