
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "wavelet.h"
#include "coder.h"
#include "yuv.h"
#include "rle.h"


#define  N_FRAMES  1


int read_ppm_info (char *fmt, int *w, int *h)
{
   int i;
   char fname [256];
   FILE *file;

   sprintf (fname, fmt, 0);
   file = fopen (fname, "r");

   if (!file) {
      fprintf(stderr, "Error: opening first frame '%s'\n", fname);
      return -1;
   }

   for (i=0; i<3; i++) {
      char ln [255];
      fgets(ln, 255, file);
      if(*ln == '#')
         i--;
      else {
         if (i == 0 && strncmp("P6", ln, 2)) {
            fprintf(stderr, "Error: Need PPM file for input\n");
            fclose (file);
            exit(-1);
         }
         if (i == 1) {
            ln[20] = 0;
            sscanf(ln, "%i %i", w, h);
         }
      }
   }

   fclose (file);
   return 0;
}


int read_ppm (char *fmt, int frame, uint8_t *buf, int w, int h)
{
   int i;
   long _w, _h;
   char fname [256];
   FILE *file;

   sprintf (fname, fmt, frame);
   file = fopen (fname, "r");

   if (!file)
      return -1;

   for (i=0; i<3; i++) {
      char ln [256];

      fgets(ln, 255, file);
      if(*ln == '#')
         i--;
      else {
         if (i == 0 && strncmp("P6", ln, 2)) {
            fprintf(stderr, "Error: Need PPM file for input\n");
            fclose (file);
            exit(-1);
         }
         if (i == 1) {
            ln[20] = 0;
            sscanf(ln, "%ld %ld", &_w, &_h);
         }
      }
   }

   if (w != _w || h != _h) {
      fprintf (stderr, "%s: image size inconsistent (w: %i <-> %ld, h: %i <-> %ld) !\n", __FUNCTION__, w, _w, h , _h);
      fclose (file);
      exit (-1);
   }

   fread (buf, 3, w*h, file);
   fclose (file);
   return 0;
}


void save_ppm (char *fmt, uint8_t *buf, int w, int h, int first_frame, int frames)
{
   int i;

   for (i=0; i<frames; i++)
   {
      char fname [256];
      FILE *outfile;
      uint8_t *img = buf + w * h * 3 * i;

      sprintf (fname, fmt, i + first_frame);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      fwrite (img, 3, w*h, outfile);
      fclose (outfile);
   }
}


void save_ppm16 (char *fmt, int16_t *buf, int w, int h, int first_frame, int frames)
{
   int i, j;

   for (i=0; i<frames; i++)
   {
      char fname [256];
      FILE *outfile;
      int16_t *img = buf + w * h * i;

      sprintf (fname, fmt, i + first_frame);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      for (j=0; j<w*h; j++) {
         uint8_t c [3] = { img[j], img[j], img[j] };
         fwrite (c, 1, 3, outfile);
      }
      fclose (outfile);
   }
}




int main (int argc, char **argv)
{
   char *fmt = "%i.ppm";
   uint8_t *rgb, *rgb2;
   char *bitstream [3];
   int i, ycount, ucount, vcount;
   int ylimit, ulimit, vlimit;
   int width = -1, height = -1, frames = 0, frame = 0;

   if (argc == 5)
      fmt = argv[4];
   else if (argc != 4) {
      printf ("\n"
        " usage: %s <ylimit> <ulimit> <vlimit> <input filename format string>\n"
        "\n"
        "   ylimit, ulimit, vlimit:     cut Y/U/V bitstream after limit bytes/frame\n"
        "   input ppm filename format:  optional, \"%%i.ppm\" by default\n"
        "\n", argv[0]);
      exit (-1);
   }

   ylimit = strtol (argv[1], 0, 0);
   ulimit = strtol (argv[2], 0, 0);
   vlimit = strtol (argv[3], 0, 0);

   if (read_ppm_info (fmt, &width, &height) < 0)
      exit (-1);

   rgb  = malloc (width * height * 3 * N_FRAMES);
   rgb2 = malloc (width * height * 3 * N_FRAMES);
   bitstream[0] = malloc (width * height * N_FRAMES);
   bitstream[1] = malloc (width * height * N_FRAMES);
   bitstream[2] = malloc (width * height * N_FRAMES);

   if (!rgb || !rgb2 || !bitstream[0] || !bitstream[1] || !bitstream[2]) {
      printf ("memory allocation failed.\n");
      return (-1);
   }


   do {
      Wavelet3DBuf *y, *u, *v, *y2, *u2, *v2;

      for (frames=0; frames<N_FRAMES; frames++) {
         printf ("read '");
         printf (fmt, frame + frames);
         printf ("'");
         if (read_ppm (fmt, frame + frames,
                       rgb + width * height * 3 * frames, width, height) < 0)
         {
            printf (" failed.\n");
            break;
         }
         printf ("\n");
      }

      y = wavelet_3d_buf_new (width, height, frames);
      u = wavelet_3d_buf_new (width, height, frames);
      v = wavelet_3d_buf_new (width, height, frames);
      y2 = wavelet_3d_buf_new (width, height, frames);
      u2 = wavelet_3d_buf_new (width, height, frames);
      v2 = wavelet_3d_buf_new (width, height, frames);

      if (!y || !u || !v || !y2 || !u2 || !v2) {
         printf ("memory allocation failed.\n");
         return (-1);
      }

      save_ppm ("orig%i.ppm", rgb, width, height, frame, frames);

      rgb2yuv (rgb, y->data, u->data, v->data, width * height * frames, 3);
/*
      save_ppm16 ("y%i.ppm", y->data, width, height, frame, frames);
      save_ppm16 ("u%i.ppm", u->data, width, height, frame, frames);
      save_ppm16 ("v%i.ppm", v->data, width, height, frame, frames);
*/
      wavelet_3d_buf_fwd_xform (y);
      wavelet_3d_buf_fwd_xform (u);
      wavelet_3d_buf_fwd_xform (v);

      save_ppm16 ("y.coeff%i.ppm", y->data, width, height, frame, frames);
      save_ppm16 ("u.coeff%i.ppm", u->data, width, height, frame, frames);
      save_ppm16 ("v.coeff%i.ppm", v->data, width, height, frame, frames);

      ycount = width * height * frames;
      ucount = width * height * frames;
      vcount = width * height * frames;

      if (ycount > frames * ylimit) ycount = frames * ylimit;
      if (ucount > frames * ulimit) ucount = frames * ulimit;
      if (vcount > frames * vlimit) vcount = frames * vlimit;

      ycount = encode_coeff3d (y, bitstream [0], ycount);
      ucount = encode_coeff3d (u, bitstream [1], ucount);
      vcount = encode_coeff3d (v, bitstream [2], vcount);

      decode_coeff3d (y2, bitstream [0], ycount);
      decode_coeff3d (u2, bitstream [1], ucount);
      decode_coeff3d (v2, bitstream [2], vcount);

      for (i=0; i<width*height*frames; i++) {
         rgb [3*i]   = (y->data[i] == y2->data [i]) ? 0 : ~0;
         rgb [3*i+1] = (u->data[i] == u2->data [i]) ? 0 : ~0;
         rgb [3*i+2] = (v->data[i] == v2->data [i]) ? 0 : ~0;
/*if (y->data[i] != y2->data [i]) {
   printf ("%i: %i <-> %i\n", i, y->data[i], y2->data[i]);
bit_print (y->data[i]);
bit_print (y2->data[i]);
}*/
      }

      save_ppm ("coeffdiff%i.ppm", rgb, width, height, frame, frames);

      save_ppm16 ("y.rcoeff%i.ppm", y2->data, width, height, frame, frames);
      save_ppm16 ("u.rcoeff%i.ppm", u2->data, width, height, frame, frames);
      save_ppm16 ("v.rcoeff%i.ppm", v2->data, width, height, frame, frames);

      wavelet_3d_buf_inv_xform (y2);
      wavelet_3d_buf_inv_xform (u2);
      wavelet_3d_buf_inv_xform (v2);

      save_ppm16 ("yr%i.ppm", y2->data, width, height, frame, frames);
      save_ppm16 ("ur%i.ppm", u2->data, width, height, frame, frames);
      save_ppm16 ("vr%i.ppm", v2->data, width, height, frame, frames);

      yuv2rgb (y2->data, u2->data, v2->data, rgb2, width * height * frames, 3);

      save_ppm ("out%03d.ppm", rgb2, width, height, frame, frames);

      wavelet_3d_buf_destroy (y);
      wavelet_3d_buf_destroy (u);
      wavelet_3d_buf_destroy (v);
      wavelet_3d_buf_destroy (y2);
      wavelet_3d_buf_destroy (u2);
      wavelet_3d_buf_destroy (v2);

      frame += frames;

   } while (frames == N_FRAMES);

   free (rgb);
   free (rgb2);
   free (bitstream[0]);
   free (bitstream[1]);
   free (bitstream[2]);
   return 0;
}

