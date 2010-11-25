/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This is an example demonstrating how to control the VP8 encoder's
 * reference frame selection and update mechanism for video applications
 * that benefit from a scalable bitstream.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#define VPX_CODEC_DISABLE_COMPAT 1
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#define interface (&vpx_codec_vp8_cx_algo)
#define fourcc    0x30385056

#define IVF_FILE_HDR_SZ  (32)
#define IVF_FRAME_HDR_SZ (12)

static void mem_put_le16(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val>>8;
}

static void mem_put_le32(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val>>8;
    mem[2] = val>>16;
    mem[3] = val>>24;
}

static void die(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    if(fmt[strlen(fmt)-1] != '\n')
        printf("\n");
    exit(EXIT_FAILURE);
}

static void die_codec(vpx_codec_ctx_t *ctx, const char *s) {
    const char *detail = vpx_codec_error_detail(ctx);

    printf("%s: %s\n", s, vpx_codec_error(ctx));
    if(detail)
        printf("    %s\n",detail);
    exit(EXIT_FAILURE);
}

static int read_frame(FILE *f, vpx_image_t *img) {
    size_t nbytes, to_read;
    int    res = 1;

    to_read = img->w*img->h*3/2;
    nbytes = fread(img->planes[0], 1, to_read, f);
    if(nbytes != to_read) {
        res = 0;
        if(nbytes > 0)
            printf("Warning: Read partial frame. Check your width & height!\n");
    }
    return res;
}

static void write_ivf_file_header(FILE *outfile,
                                  const vpx_codec_enc_cfg_t *cfg,
                                  int frame_cnt) {
    char header[32];

    if(cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
        return;
    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    mem_put_le16(header+4,  0);                   /* version */
    mem_put_le16(header+6,  32);                  /* headersize */
    mem_put_le32(header+8,  fourcc);              /* headersize */
    mem_put_le16(header+12, cfg->g_w);            /* width */
    mem_put_le16(header+14, cfg->g_h);            /* height */
    mem_put_le32(header+16, cfg->g_timebase.den); /* rate */
    mem_put_le32(header+20, cfg->g_timebase.num); /* scale */
    mem_put_le32(header+24, frame_cnt);           /* length */
    mem_put_le32(header+28, 0);                   /* unused */

    if(fwrite(header, 1, 32, outfile));
}


static void write_ivf_frame_header(FILE *outfile,
                                   const vpx_codec_cx_pkt_t *pkt)
{
    char             header[12];
    vpx_codec_pts_t  pts;

    if(pkt->kind != VPX_CODEC_CX_FRAME_PKT)
        return;

    pts = pkt->data.frame.pts;
    mem_put_le32(header, pkt->data.frame.sz);
    mem_put_le32(header+4, pts&0xFFFFFFFF);
    mem_put_le32(header+8, pts >> 32);

    if(fwrite(header, 1, 12, outfile));
}

int main(int argc, char **argv) {
    FILE                *infile, *outfile;
    vpx_codec_ctx_t      codec;
    vpx_codec_enc_cfg_t  cfg;
    int                  frame_cnt = 0;
    unsigned char        file_hdr[IVF_FILE_HDR_SZ];
    unsigned char        frame_hdr[IVF_FRAME_HDR_SZ];
    vpx_image_t          raw;
    vpx_codec_err_t      res;
    long                 width;
    long                 height;
    int                  frame_avail;
    int                  got_data;
    int                  flags = 0;
    int                  num_streams = 5;                                     //

    /* Open files */
    if(argc!=5)
        die("Usage: %s <width> <height> <infile> <outfile>\n", argv[0]);
    width = strtol(argv[1], NULL, 0);
    height = strtol(argv[2], NULL, 0);
    if(width < 16 || width%2 || height <16 || height%2)
        die("Invalid resolution: %ldx%ld", width, height);
    if(!vpx_img_alloc(&raw, VPX_IMG_FMT_I420, width, height, 1))
        die("Faile to allocate image", width, height);
    if(!(outfile = fopen(argv[4], "wb")))
        die("Failed to open %s for writing", argv[4]);

    printf("Using %s\n",vpx_codec_iface_name(interface));

    /* Populate encoder configuration */
    res = vpx_codec_enc_config_default(interface, &cfg, 0);
    if(res) {
        printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));
        return EXIT_FAILURE;
    }

    /* Update the default configuration with our settings */
    cfg.rc_target_bitrate = width * height * cfg.rc_target_bitrate
                            / cfg.g_w / cfg.g_h;
    cfg.g_w = width;
    cfg.g_h = height;
                                                                              //
    /* Enable error resilient mode */                                         //
    cfg.g_error_resilient = 1;                                                //
    cfg.g_lag_in_frames   = 0;                                                //
    cfg.kf_mode           = VPX_KF_FIXED;                                     //
                                                                              //
    /* Disable automatic keyframe placement */                                //
    cfg.kf_min_dist = cfg.kf_max_dist = 1000;                                 //

    write_ivf_file_header(outfile, &cfg, 0);


        /* Open input file for this encoding pass */
        if(!(infile = fopen(argv[3], "rb")))
            die("Failed to open %s for reading", argv[3]);

        /* Initialize codec */
        if(vpx_codec_enc_init(&codec, interface, &cfg, 0))
            die_codec(&codec, "Failed to initialize encoder");

        frame_avail = 1;
        got_data = 0;
        while(frame_avail || got_data) {
            vpx_codec_iter_t iter = NULL;
            const vpx_codec_cx_pkt_t *pkt;

            flags = 0;                                                        //
            if(num_streams == 5)                                              //
            {                                                                 //
                switch(frame_cnt % 16) {                                      //
                    case 0:                                                   //
                        flags |= VPX_EFLAG_FORCE_KF;                          //
                        flags |= VP8_EFLAG_FORCE_GF;                          //
                        flags |= VP8_EFLAG_FORCE_ARF;                         //
                        break;                                                //
                    case 1:                                                   //
                    case 3:                                                   //
                    case 5:                                                   //
                    case 7:                                                   //
                    case 9:                                                   //
                    case 11:                                                  //
                    case 13:                                                  //
                    case 15:                                                  //
                        flags |= VP8_EFLAG_NO_UPD_LAST;                       //
                        flags |= VP8_EFLAG_NO_UPD_GF;                         //
                        flags |= VP8_EFLAG_NO_UPD_ARF;                        //
                        break;                                                //
                    case 2:                                                   //
                    case 6:                                                   //
                    case 10:                                                  //
                    case 14:                                                  //
                        break;                                                //
                    case 4:                                                   //
                        flags |= VP8_EFLAG_NO_REF_LAST;                       //
                        flags |= VP8_EFLAG_FORCE_GF;                          //
                        break;                                                //
                    case 8:                                                   //
                        flags |= VP8_EFLAG_NO_REF_LAST;                       //
                        flags |= VP8_EFLAG_NO_REF_GF;                         //
                        flags |= VP8_EFLAG_FORCE_GF;                          //
                        flags |= VP8_EFLAG_FORCE_ARF;                         //
                        break;                                                //
                    case 12:                                                  //
                        flags |= VP8_EFLAG_NO_REF_LAST;                       //
                        flags |= VP8_EFLAG_FORCE_GF;                          //
                        break;                                                //
                }                                                             //
            }                                                                 //
            else                                                              //
            {                                                                 //
                switch(frame_cnt % 9) {                                       //
                    case 0:                                                   //
                        if(frame_cnt==0)                                      //
                        {                                                     //
                            flags |= VPX_EFLAG_FORCE_KF;                      //
                        }                                                     //
                        else                                                  //
                        {                                                     //
                            cfg.rc_max_quantizer = 26;                        //
                            cfg.rc_min_quantizer = 0;                         //
                            cfg.rc_target_bitrate = 300;                      //
                            flags |= VP8_EFLAG_NO_REF_LAST;                   //
                            flags |= VP8_EFLAG_NO_REF_ARF;                    //
                        }                                                     //
                        flags |= VP8_EFLAG_FORCE_GF;                          //
                        flags |= VP8_EFLAG_FORCE_ARF;                         //
                        break;                                                //
                    case 1:                                                   //
                    case 2:                                                   //
                    case 4:                                                   //
                    case 5:                                                   //
                    case 7:                                                   //
                    case 8:                                                   //
                        cfg.rc_max_quantizer = 45;                            //
                        cfg.rc_min_quantizer = 0;                             //
                        cfg.rc_target_bitrate = 230;                          //
                        break;                                                //
                    case 3:                                                   //
                    case 6:                                                   //
                        cfg.rc_max_quantizer = 45;                            //
                        cfg.rc_min_quantizer = 0;                             //
                        cfg.rc_target_bitrate = 215;                          //
                        flags |= VP8_EFLAG_NO_REF_LAST;                       //
                        flags |= VP8_EFLAG_FORCE_ARF;                         //
                        break;                                                //
                }                                                             //
            }                                                                 //
            frame_avail = read_frame(infile, &raw);
            if(vpx_codec_encode(&codec, frame_avail? &raw : NULL, frame_cnt,
                                1, flags, VPX_DL_REALTIME))
                die_codec(&codec, "Failed to encode frame");
            got_data = 0;
            while( (pkt = vpx_codec_get_cx_data(&codec, &iter)) ) {
                got_data = 1;
                switch(pkt->kind) {
                case VPX_CODEC_CX_FRAME_PKT:
                    write_ivf_frame_header(outfile, pkt);
                    if(fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz,
                              outfile));
                    break;
                default:
                    break;
                }
                printf(pkt->kind == VPX_CODEC_CX_FRAME_PKT
                       && (pkt->data.frame.flags & VPX_FRAME_IS_KEY)? "K":".");
                fflush(stdout);
            }
            frame_cnt++;
        }
        printf("\n");
        fclose(infile);

    printf("Processed %d frames.\n",frame_cnt-1);
    if(vpx_codec_destroy(&codec))
        die_codec(&codec, "Failed to destroy codec");

    /* Try to rewrite the file header with the actual frame count */
    if(!fseek(outfile, 0, SEEK_SET))
        write_ivf_file_header(outfile, &cfg, frame_cnt-1);
    fclose(outfile);
    return EXIT_SUCCESS;
}
