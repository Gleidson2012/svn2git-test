/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id$

 ********************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include "toplevel_lookup.h"
#include "../internal.h"
#include "dsp.h"
#include "codec_internal.h"

#ifdef _TH_DEBUG_
FILE *debugout=NULL;
long dframe=0;
#endif

static void EClearFragmentInfo(CP_INSTANCE * cpi){
  if(cpi->RunHuffIndices)
    _ogg_free(cpi->RunHuffIndices);
  if(cpi->ModeList)
    _ogg_free(cpi->ModeList);
  if(cpi->MVList)
    _ogg_free(cpi->MVList);
  if(cpi->PartiallyCodedFlags)
    _ogg_free(cpi->PartiallyCodedFlags);
  if(cpi->PartiallyCodedMbPatterns)
    _ogg_free(cpi->PartiallyCodedMbPatterns);
  if(cpi->BlockCodedFlags)
    _ogg_free(cpi->BlockCodedFlags);

  cpi->RunHuffIndices = 0;
  cpi->ModeList = 0;
  cpi->MVList = 0;
  cpi->PartiallyCodedFlags = 0;
  cpi->PartiallyCodedMbPatterns = 0;
  cpi->BlockCodedFlags = 0;
}

static void EInitFragmentInfo(CP_INSTANCE * cpi){

  /* clear any existing info */
  EClearFragmentInfo(cpi);

  /* A note to people reading and wondering why malloc returns aren't
     checked:

     lines like the following that implement a general strategy of
     'check the return of malloc; a zero pointer means we're out of
     memory!'...:

  if(!cpi->extra_fragments) { EDeleteFragmentInfo(cpi); return FALSE; }

     ...are not useful.  It's true that many platforms follow this
     malloc behavior, but many do not.  The more modern malloc
     strategy is only to allocate virtual pages, which are not mapped
     until the memory on that page is touched.  At *that* point, if
     the machine is out of heap, the page fails to be mapped and a
     SEGV is generated.

     That means that if we want to deal with out of memory conditions,
     we *must* be prepared to process a SEGV.  If we implement the
     SEGV handler, there's no reason to to check malloc return; it is
     a waste of code. */

  cpi->RunHuffIndices =
    _ogg_malloc(cpi->pb.UnitFragments*
                sizeof(*cpi->RunHuffIndices));
  cpi->BlockCodedFlags =
    _ogg_malloc(cpi->pb.UnitFragments*
                sizeof(*cpi->BlockCodedFlags));
  cpi->ModeList =
    _ogg_malloc(cpi->pb.UnitFragments*
                sizeof(*cpi->ModeList));
  cpi->MVList =
    _ogg_malloc(cpi->pb.UnitFragments*
                sizeof(*cpi->MVList));
  cpi->PartiallyCodedFlags =
    _ogg_malloc(cpi->pb.MacroBlocks*
                sizeof(*cpi->PartiallyCodedFlags));
  cpi->PartiallyCodedMbPatterns =
    _ogg_malloc(cpi->pb.MacroBlocks*
                sizeof(*cpi->PartiallyCodedMbPatterns));

}

static void EClearFrameInfo(CP_INSTANCE * cpi) {

  if(cpi->yuvptr)
    _ogg_free(cpi->yuvptr);
  cpi->yuvptr = 0;

  if(cpi->OptimisedTokenListEb )
    _ogg_free(cpi->OptimisedTokenListEb);
  cpi->OptimisedTokenListEb = 0;

  if(cpi->OptimisedTokenList )
    _ogg_free(cpi->OptimisedTokenList);
  cpi->OptimisedTokenList = 0;

  if(cpi->OptimisedTokenListHi )
    _ogg_free(cpi->OptimisedTokenListHi);
  cpi->OptimisedTokenListHi = 0;

  if(cpi->OptimisedTokenListPl )
    _ogg_free(cpi->OptimisedTokenListPl);
  cpi->OptimisedTokenListPl = 0;

  if(cpi->frag[0])_ogg_free(cpi->frag[0]);
  if(cpi->macro)_ogg_free(cpi->macro);
  if(cpi->super[0])_ogg_free(cpi->super[0]);
  cpi->frag[0] = 0;
  cpi->frag[1] = 0;
  cpi->frag[2] = 0;
  cpi->macro = 0;
  cpi->super[0] = 0;
  cpi->super[1] = 0;
  cpi->super[2] = 0;

}

static void EInitFrameInfo(CP_INSTANCE * cpi){
  int FrameSize = cpi->pb.ReconYPlaneSize + 2 * cpi->pb.ReconUVPlaneSize;

  /* clear any existing info */
  EClearFrameInfo(cpi);

  /* allocate frames */
  cpi->yuvptr =
    _ogg_malloc(FrameSize*
                sizeof(*cpi->yuvptr));
  cpi->OptimisedTokenListEb =
    _ogg_malloc(FrameSize*
                sizeof(*cpi->OptimisedTokenListEb));
  cpi->OptimisedTokenList =
    _ogg_malloc(FrameSize*
                sizeof(*cpi->OptimisedTokenList));
  cpi->OptimisedTokenListHi =
    _ogg_malloc(FrameSize*
                sizeof(*cpi->OptimisedTokenListHi));
  cpi->OptimisedTokenListPl =
    _ogg_malloc(FrameSize*
                sizeof(*cpi->OptimisedTokenListPl));

  /* new block abstraction setup... babysteps... */
  cpi->frag_h[0] = (cpi->info.width >> 3);
  cpi->frag_v[0] = (cpi->info.height >> 3);
  cpi->frag_n[0] = cpi->frag_h[0] * cpi->frag_v[0];
  cpi->frag_h[1] = (cpi->info.width >> 4);
  cpi->frag_v[1] = (cpi->info.height >> 4);
  cpi->frag_n[1] = cpi->frag_h[1] * cpi->frag_v[1];
  cpi->frag_h[2] = (cpi->info.width >> 4);
  cpi->frag_v[2] = (cpi->info.height >> 4);
  cpi->frag_n[2] = cpi->frag_h[2] * cpi->frag_v[2];
  cpi->frag_total = cpi->frag_n[0] + cpi->frag_n[1] + cpi->frag_n[2];

  cpi->macro_h = (cpi->frag_h[0] >> 1);
  cpi->macro_v = (cpi->frag_v[0] >> 1);
  cpi->macro_total = cpi->macro_h * cpi->macro_v;

  cpi->super_h[0] = (cpi->info.width >> 5) + ((cpi->info.width & 0x1f) ? 1 : 0);
  cpi->super_v[0] = (cpi->info.height >> 5) + ((cpi->info.height & 0x1f) ? 1 : 0);
  cpi->super_n[0] = cpi->super_h[0] * cpi->super_v[0];
  cpi->super_h[1] = (cpi->info.width >> 6) + ((cpi->info.width & 0x3f) ? 1 : 0);
  cpi->super_v[1] = (cpi->info.height >> 6) + ((cpi->info.height & 0x3f) ? 1 : 0);
  cpi->super_n[1] = cpi->super_h[1] * cpi->super_v[1];
  cpi->super_h[2] = (cpi->info.width >> 6) + ((cpi->info.width & 0x3f) ? 1 : 0);
  cpi->super_v[2] = (cpi->info.height >> 6) + ((cpi->info.height & 0x3f) ? 1 : 0);
  cpi->super_n[2] = cpi->super_h[2] * cpi->super_v[2];
  cpi->super_total = cpi->super_n[0] + cpi->super_n[1] + cpi->super_n[2];

  cpi->frag[0] = calloc(cpi->frag_total, sizeof(**cpi->frag));
  cpi->frag[1] = cpi->frag[0] + cpi->frag_n[0];
  cpi->frag[2] = cpi->frag[1] + cpi->frag_n[1];

  cpi->macro = calloc(cpi->macro_total, sizeof(*cpi->macro));

  cpi->super[0] = calloc(cpi->super_total, sizeof(**cpi->super));
  cpi->super[1] = cpi->super[0] + cpi->super_n[0];
  cpi->super[2] = cpi->super[1] + cpi->super_n[1];

  /* fill in superblock fragment pointers; hilbert order */
  {
    int row,col,frag,mb;
    int fhilbertx[16] = {0,1,1,0,0,0,1,1,2,2,3,3,3,2,2,3};
    int fhilberty[16] = {0,0,1,1,2,3,3,2,2,3,3,2,1,1,0,0};
    int mhilbertx[4] = {0,0,1,1};
    int mhilberty[4] = {0,1,1,0};
    int plane;

    for(plane=0;plane<3;plane++){

      for(row=0;row<cpi->super_v[plane];row++){
	for(col=0;col<cpi->super_h[plane];col++){
	  int superindex = row*cpi->super_h[plane] + col;
	  for(frag=0;frag<16;frag++){
	    /* translate to fragment index */
	    int frow = row*4 + fhilberty[frag];
	    int fcol = col*4 + fhilbertx[frag];
	    if(frow<cpi->frag_v[plane] && fcol<cpi->frag_h[plane]){
	      int fragindex = frow*cpi->frag_h[plane] + fcol;
	      cpi->super[plane][superindex].f[frag] = &cpi->frag[plane][fragindex];
	    }
	  }
	}
      }
    }

    for(row=0;row<cpi->super_v[0];row++){
      for(col=0;col<cpi->super_h[0];col++){
	int superindex = row*cpi->super_h[0] + col;
	for(mb=0;mb<4;mb++){
	  /* translate to macroblock index */
	  int mrow = row*2 + mhilberty[mb];
	  int mcol = col*2 + mhilbertx[mb];
	  if(mrow<cpi->macro_v && mcol<cpi->macro_h){
	    int macroindex = mrow*cpi->macro_h + mcol;
	    cpi->super[0][superindex].m[mb] = &cpi->macro[macroindex];
	  }
	}
      }
    }
  }

  /* fill in macroblock fragment pointers; raster (MV coding) order */
  {
    int row,col,frag;
    int scanx[4] = {0,1,0,1};
    int scany[4] = {0,1,1,0};

    for(row=0;row<cpi->macro_v;row++){
      int baserow = row*2;
      for(col=0;col<cpi->macro_h;col++){
	int basecol = col*2;
	int macroindex = row*cpi->macro_h + col;
	for(frag=0;frag<4;frag++){
	  /* translate to fragment index */
	  int frow = baserow + scany[frag];
	  int fcol = basecol + scanx[frag];
	  if(frow<cpi->frag_v[0] && fcol<cpi->frag_h[0]){
	    int fragindex = frow*cpi->frag_h[0] + fcol;
	    cpi->macro[macroindex].y[frag] = &cpi->frag[0][fragindex];
	  }
	}

	if(row<cpi->frag_v[1] && col<cpi->frag_h[1])
	  cpi->macro[macroindex].u = &cpi->frag[1][macroindex];
	if(row<cpi->frag_v[2] && col<cpi->frag_h[2])
	  cpi->macro[macroindex].v = &cpi->frag[2][macroindex];

      }
    }
  }
}

static void SetupKeyFrame(CP_INSTANCE *cpi) {
  int i,j;

  /* code all blocks */
  for(i=0;i<3;i++)
    for(j=0;j<cpi->frag_n[i];j++)
      cpi->frag[i][j].coded=1;
  
  /* Set up for a KEY FRAME */
  cpi->pb.FrameType = KEY_FRAME;
}

static void AdjustKeyFrameContext(CP_INSTANCE *cpi) {

  /* Update the frame carry over. */
  cpi->TotKeyFrameBytes += oggpackB_bytes(cpi->oggbuffer);

  cpi->LastKeyFrame = 1;
}

static void UpdateFrame(CP_INSTANCE *cpi){

  /* Initialise bit packing mechanism. */
#ifndef LIBOGG2
  oggpackB_reset(cpi->oggbuffer);
#else
  oggpackB_writeinit(cpi->oggbuffer, cpi->oggbufferstate);
#endif

  /* mark as video frame */
  oggpackB_write(cpi->oggbuffer,0,1);

  /* Write out the frame header information including size. */
  WriteFrameHeader(cpi);

  /* Encode the data.  */
  EncodeData(cpi);

  if ( cpi->pb.FrameType == KEY_FRAME ) 
    AdjustKeyFrameContext(cpi);

}

static void CompressFirstFrame(CP_INSTANCE *cpi) {

  /* Keep track of the total number of Key Frames Coded. */
  cpi->KeyFrameCount = 1;
  cpi->LastKeyFrame = 1;
  cpi->TotKeyFrameBytes = 0;

  SetupKeyFrame(cpi);

  /* Compress and output the frist frame. */
  PickIntra(cpi);
  UpdateFrame(cpi);

}

static void CompressKeyFrame(CP_INSTANCE *cpi){

  /* Keep track of the total number of Key Frames Coded */
  cpi->KeyFrameCount += 1;

  SetupKeyFrame(cpi);

  /* Compress and output the frist frame. */
  PickIntra(cpi);
  UpdateFrame(cpi);

}

static int CompressFrame( CP_INSTANCE *cpi ) {
  ogg_uint32_t  i,j;
  ogg_uint32_t  KFIndicator = 0;

  /* Clear down the macro block level mode and MV arrays. */
  for ( i = 0; i < cpi->pb.UnitFragments; i++ ) {
    cpi->frag[0][i].mode = CODE_INTER_NO_MV;  /* Default coding mode */
    cpi->pb.FragMVect[i].x = 0;
    cpi->pb.FragMVect[i].y = 0;
  }

  /* Default to delta frames. */
  cpi->pb.FrameType = DELTA_FRAME;

  /* Clear down the difference arrays for the current frame. */
  for(i=0;i<3;i++)
    for(j=0;j<cpi->frag_n[i];j++)
      cpi->frag[i][j].coded=0;

  {
    /*  pick all the macroblock modes and motion vectors */
    ogg_uint32_t InterError;
    ogg_uint32_t IntraError;
    
    /* for now, mark all blocks to be coded... */
    /* TEMPORARY */
    for(i=0;i<3;i++)
      for(j=0;j<cpi->frag_n[i];j++)
	cpi->frag[i][j].coded=1;
    
    /* Select modes and motion vectors for each of the blocks : return
       an error score for inter and intra */
    PickModes( cpi, cpi->pb.YSBRows, cpi->pb.YSBCols,
               cpi->info.width,
               &InterError, &IntraError );

    /* decide whether we really should have made this frame a key frame */
    /* forcing out a keyframe if the max interval is up is done at a higher level */
    if( cpi->info.keyframe_auto_p){
      if( ( 2* IntraError < 5 * InterError )
          && ( KFIndicator >= (ogg_uint32_t)
               cpi->info.keyframe_auto_threshold)
          && ( cpi->LastKeyFrame > cpi->info.keyframe_mindistance)
          ){
        CompressKeyFrame(cpi);  /* Code a key frame */
        return 0;
      }

    }

    /* Increment the frames since last key frame count */
    cpi->LastKeyFrame++;

    /* Proceed with the frame update. */
    UpdateFrame(cpi);

  }

  return 0;
}

/********************** The toplevel: encode ***********************/

static int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static void theora_encode_dispatch_init(CP_INSTANCE *cpi);

int theora_encode_init(theora_state *th, theora_info *c){
  CP_INSTANCE *cpi;

#ifdef _TH_DEBUG_
  debugout=fopen("theoraenc-debugout.txt","w");
#endif

  memset(th, 0, sizeof(*th));
  /*Currently only the 4:2:0 format is supported.*/
  if(c->pixelformat!=OC_PF_420)return OC_IMPL;
  th->internal_encode=cpi=_ogg_calloc(1,sizeof(*cpi));
  theora_encode_dispatch_init(cpi);

  dsp_static_init (&cpi->dsp);
  memcpy (&cpi->pb.dsp, &cpi->dsp, sizeof(DspFunctions));

  c->version_major=TH_VERSION_MAJOR;
  c->version_minor=TH_VERSION_MINOR;
  c->version_subminor=TH_VERSION_SUB;

  if(c->quality>63)c->quality=63;
  if(c->quality<0)c->quality=32;
  if(c->target_bitrate<0)c->target_bitrate=0;
  cpi->BaseQ = c->quality;

  cpi->MVChangeFactor    =    14;
  cpi->FourMvChangeFactor =   8;
  cpi->MinImprovementForNewMV = 25;
  cpi->ExhaustiveSearchThresh = 2500;
  cpi->MinImprovementForFourMV = 100;
  cpi->FourMVThreshold = 10000;
  cpi->InterTripOutThresh = 5000;
  cpi->MVEnabled = 1;
  cpi->GoldenFrameEnabled = 1;
  cpi->InterPrediction = 1;
  cpi->MotionCompensation = 1;

  /* Set encoder flags. */
  /* if not AutoKeyframing cpi->ForceKeyFrameEvery = is frequency */
  if(!c->keyframe_auto_p)
    c->keyframe_frequency_force = c->keyframe_frequency;

  /* Set the frame rate variables. */
  if ( c->fps_numerator < 1 )
    c->fps_numerator = 1;
  if ( c->fps_denominator < 1 )
    c->fps_denominator = 1;

  /* don't go too nuts on keyframe spacing; impose a high limit to
     make certain the granulepos encoding strategy works */
  if(c->keyframe_frequency_force>32768)c->keyframe_frequency_force=32768;
  if(c->keyframe_mindistance>32768)c->keyframe_mindistance=32768;
  if(c->keyframe_mindistance>c->keyframe_frequency_force)
    c->keyframe_mindistance=c->keyframe_frequency_force;
  cpi->keyframe_granule_shift=_ilog(c->keyframe_frequency_force-1);

  /* clamp the target_bitrate to a maximum of 24 bits so we get a
     more meaningful value when we write this out in the header. */
  if(c->target_bitrate>(1<<24)-1)c->target_bitrate=(1<<24)-1;

  /* copy in config */
  memcpy(&cpi->info,c,sizeof(*c));
  th->i=&cpi->info;
  th->granulepos=-1;

  /* Set up an encode buffer */
#ifndef LIBOGG2
  cpi->oggbuffer = _ogg_malloc(sizeof(oggpack_buffer));
  oggpackB_writeinit(cpi->oggbuffer);
#else
  cpi->oggbuffer = _ogg_malloc(oggpack_buffersize());
  cpi->oggbufferstate = ogg_buffer_create();
  oggpackB_writeinit(cpi->oggbuffer, cpi->oggbufferstate);
#endif 

  InitFrameDetails(cpi);
  EInitFragmentInfo(cpi);
  EInitFrameInfo(cpi);

  /* Initialise Motion compensation */
  InitMotionCompensation(cpi);

  /* Initialise the compression process. */
  /* We always start at frame 1 */
  cpi->CurrentFrame = 1;

  InitHuffmanSet(&cpi->pb);

  /* This makes sure encoder version specific tables are initialised */
  memcpy(&cpi->pb.quant_info, &TH_VP31_QUANT_INFO, sizeof(th_quant_info));
  InitQTables(&cpi->pb);

  /* Indicate that the next frame to be compressed is the first in the
     current clip. */
  cpi->ThisIsFirstFrame = 1;
  cpi->readyflag = 1;
  
  cpi->HeadersWritten = 0;

  return 0;
}

int theora_encode_YUVin(theora_state *t,
                         yuv_buffer *yuv){
  int dropped = 0;
  ogg_int32_t i;
  unsigned char *LocalDataPtr;
  unsigned char *InputDataPtr;
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);

  if(!cpi->readyflag)return OC_EINVAL;
  if(cpi->doneflag)return OC_EINVAL;

  /* If frame size has changed, abort out for now */
  if (yuv->y_height != (int)cpi->info.height ||
      yuv->y_width != (int)cpi->info.width )
    return(-1);

  /* Copy over input YUV to internal YUV buffers. */
  /* we invert the image for backward compatibility with VP3 */
  /* First copy over the Y data */
  LocalDataPtr = cpi->yuvptr + yuv->y_width*(yuv->y_height - 1);
  InputDataPtr = yuv->y;
  for ( i = 0; i < yuv->y_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->y_width );
    LocalDataPtr -= yuv->y_width;
    InputDataPtr += yuv->y_stride;
  }

  /* Now copy over the U data */
  LocalDataPtr = &cpi->yuvptr[(yuv->y_height * yuv->y_width)];
  LocalDataPtr += yuv->uv_width*(yuv->uv_height - 1);
  InputDataPtr = yuv->u;
  for ( i = 0; i < yuv->uv_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->uv_width );
    LocalDataPtr -= yuv->uv_width;
    InputDataPtr += yuv->uv_stride;
  }

  /* Now copy over the V data */
  LocalDataPtr =
    &cpi->yuvptr[((yuv->y_height*yuv->y_width)*5)/4];
  LocalDataPtr += yuv->uv_width*(yuv->uv_height - 1);
  InputDataPtr = yuv->v;
  for ( i = 0; i < yuv->uv_height; i++ ){
    memcpy( LocalDataPtr, InputDataPtr, yuv->uv_width );
    LocalDataPtr -= yuv->uv_width;
    InputDataPtr += yuv->uv_stride;
  }

  /* Special case for first frame */
  if ( cpi->ThisIsFirstFrame ){
    CompressFirstFrame(cpi);
    cpi->ThisIsFirstFrame = 0;
    cpi->ThisIsKeyFrame = 0;
  } else {

    /* don't allow generating invalid files that overflow the p-frame
       shift, even if keyframe_auto_p is turned off */
    if(cpi->LastKeyFrame >= (ogg_uint32_t)
       cpi->info.keyframe_frequency_force)
      cpi->ThisIsKeyFrame = 1;
    
    if ( cpi->ThisIsKeyFrame ) {
      CompressKeyFrame(cpi);
      cpi->ThisIsKeyFrame = 0;
    } else  {
      /* Compress the frame. */
      dropped = CompressFrame(cpi);
    }

  }

  /* Update stats variables. */
  cpi->CurrentFrame++;
  cpi->packetflag=1;

  t->granulepos=
    ((cpi->CurrentFrame - cpi->LastKeyFrame)<<cpi->keyframe_granule_shift)+
    cpi->LastKeyFrame - 1;

#ifdef _TH_DEBUG_
  dframe++;
#endif  

  return 0;
}

int theora_encode_packetout( theora_state *t, int last_p, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);
  long bytes=oggpackB_bytes(cpi->oggbuffer);

  if(!bytes)return(0);
  if(!cpi->packetflag)return(0);
  if(cpi->doneflag)return(-1);

#ifndef LIBOGG2
  op->packet=oggpackB_get_buffer(cpi->oggbuffer);
#else
  op->packet=oggpackB_writebuffer(cpi->oggbuffer);
#endif
  op->bytes=bytes;
  op->b_o_s=0;
  op->e_o_s=last_p;

  op->packetno=cpi->CurrentFrame;
  op->granulepos=t->granulepos;

  cpi->packetflag=0;
  if(last_p)cpi->doneflag=1;

  return 1;
}

static void _tp_writebuffer(oggpack_buffer *opb, const char *buf, const long len)
{
  long i;

  for (i = 0; i < len; i++)
    oggpackB_write(opb, *buf++, 8);
}

static void _tp_writelsbint(oggpack_buffer *opb, long value)
{
  oggpackB_write(opb, value&0xFF, 8); 
  oggpackB_write(opb, value>>8&0xFF, 8);
  oggpackB_write(opb, value>>16&0xFF, 8);
  oggpackB_write(opb, value>>24&0xFF, 8);
}

/* build the initial short header for stream recognition and format */
int theora_encode_header(theora_state *t, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);
  int offset_y;

#ifndef LIBOGG2
  oggpackB_reset(cpi->oggbuffer);
#else
  oggpackB_writeinit(cpi->oggbuffer, cpi->oggbufferstate);
#endif
  oggpackB_write(cpi->oggbuffer,0x80,8);
  _tp_writebuffer(cpi->oggbuffer, "theora", 6);

  oggpackB_write(cpi->oggbuffer,TH_VERSION_MAJOR,8);
  oggpackB_write(cpi->oggbuffer,TH_VERSION_MINOR,8);
  oggpackB_write(cpi->oggbuffer,TH_VERSION_SUB,8);

  oggpackB_write(cpi->oggbuffer,cpi->info.width>>4,16);
  oggpackB_write(cpi->oggbuffer,cpi->info.height>>4,16);
  oggpackB_write(cpi->oggbuffer,cpi->info.frame_width,24);
  oggpackB_write(cpi->oggbuffer,cpi->info.frame_height,24);
  oggpackB_write(cpi->oggbuffer,cpi->info.offset_x,8);
  /* Applications use offset_y to mean offset from the top of the image; the
   * meaning in the bitstream is the opposite (from the bottom). Transform.
   */
  offset_y = cpi->info.height - cpi->info.frame_height - 
    cpi->info.offset_y;
  oggpackB_write(cpi->oggbuffer,offset_y,8);

  oggpackB_write(cpi->oggbuffer,cpi->info.fps_numerator,32);
  oggpackB_write(cpi->oggbuffer,cpi->info.fps_denominator,32);
  oggpackB_write(cpi->oggbuffer,cpi->info.aspect_numerator,24);
  oggpackB_write(cpi->oggbuffer,cpi->info.aspect_denominator,24);

  oggpackB_write(cpi->oggbuffer,cpi->info.colorspace,8);
  oggpackB_write(cpi->oggbuffer,cpi->info.target_bitrate,24);
  oggpackB_write(cpi->oggbuffer,cpi->info.quality,6);

  oggpackB_write(cpi->oggbuffer,cpi->keyframe_granule_shift,5);

  oggpackB_write(cpi->oggbuffer,cpi->info.pixelformat,2);

  oggpackB_write(cpi->oggbuffer,0,3); /* spare config bits */

#ifndef LIBOGG2
  op->packet=oggpackB_get_buffer(cpi->oggbuffer);
#else
  op->packet=oggpackB_writebuffer(cpi->oggbuffer);
#endif
  op->bytes=oggpackB_bytes(cpi->oggbuffer);

  op->b_o_s=1;
  op->e_o_s=0;

  op->packetno=0;

  op->granulepos=0;
  cpi->packetflag=0;

  return(0);
}

/* build the comment header packet from the passed metadata */
int theora_encode_comment(theora_comment *tc, ogg_packet *op)
{
  const char *vendor = theora_version_string();
  const int vendor_length = strlen(vendor);
  oggpack_buffer *opb;

#ifndef LIBOGG2
  opb = _ogg_malloc(sizeof(oggpack_buffer));
  oggpackB_writeinit(opb);
#else
  opb = _ogg_malloc(oggpack_buffersize());
  oggpackB_writeinit(opb, ogg_buffer_create());
#endif 
  oggpackB_write(opb, 0x81, 8);
  _tp_writebuffer(opb, "theora", 6);

  _tp_writelsbint(opb, vendor_length);
  _tp_writebuffer(opb, vendor, vendor_length);

  _tp_writelsbint(opb, tc->comments);
  if(tc->comments){
    int i;
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        _tp_writelsbint(opb,tc->comment_lengths[i]);
        _tp_writebuffer(opb,tc->user_comments[i],tc->comment_lengths[i]);
      }else{
        oggpackB_write(opb,0,32);
      }
    }
  }
  op->bytes=oggpack_bytes(opb);

#ifndef LIBOGG2
  /* So we're expecting the application will free this? */
  op->packet=_ogg_malloc(oggpack_bytes(opb));
  memcpy(op->packet, oggpack_get_buffer(opb), oggpack_bytes(opb));
  oggpack_writeclear(opb);
#else
  op->packet = oggpack_writebuffer(opb);
  /* When the application puts op->packet into a stream_state object,
     it becomes the property of libogg2's internal memory management. */
#endif

  _ogg_free(opb);

  op->b_o_s=0;
  op->e_o_s=0;

  op->packetno=0;
  op->granulepos=0;

  return (0);
}

/* build the final header packet with the tables required
   for decode */
int theora_encode_tables(theora_state *t, ogg_packet *op){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(t->internal_encode);

#ifndef LIBOGG2
  oggpackB_reset(cpi->oggbuffer);
#else
  oggpackB_writeinit(cpi->oggbuffer, cpi->oggbufferstate);
#endif
  oggpackB_write(cpi->oggbuffer,0x82,8);
  _tp_writebuffer(cpi->oggbuffer,"theora",6);

  WriteQTables(&cpi->pb,cpi->oggbuffer);
  WriteHuffmanTrees(cpi->pb.HuffRoot_VP3x,cpi->oggbuffer);

#ifndef LIBOGG2
  op->packet=oggpackB_get_buffer(cpi->oggbuffer);
#else
  op->packet=oggpackB_writebuffer(cpi->oggbuffer);
#endif
  op->bytes=oggpackB_bytes(cpi->oggbuffer);

  op->b_o_s=0;
  op->e_o_s=0;

  op->packetno=0;

  op->granulepos=0;
  cpi->packetflag=0;

  cpi->HeadersWritten = 1;

  return(0);
}

static void theora_encode_clear (theora_state  *th){
  CP_INSTANCE *cpi;
  cpi=(CP_INSTANCE *)th->internal_encode;
  if(cpi){

    ClearHuffmanSet(&cpi->pb);
    EClearFragmentInfo(cpi);
    EClearFrameInfo(cpi);

    oggpackB_writeclear(cpi->oggbuffer);
    _ogg_free(cpi->oggbuffer);
    _ogg_free(cpi);
  }

#ifdef _TH_DEBUG_
  fclose(debugout);
  debugout=NULL;
#endif

  memset(th,0,sizeof(*th));
}


/* returns, in seconds, absolute time of current packet in given
   logical stream */
static double theora_encode_granule_time(theora_state *th,
 ogg_int64_t granulepos){
#ifndef THEORA_DISABLE_FLOAT
  CP_INSTANCE *cpi=(CP_INSTANCE *)(th->internal_encode);
  PB_INSTANCE *pbi=(PB_INSTANCE *)(th->internal_decode);

  if(cpi)pbi=&cpi->pb;

  if(granulepos>=0){
    ogg_int64_t iframe=granulepos>>cpi->keyframe_granule_shift;
    ogg_int64_t pframe=granulepos-(iframe<<cpi->keyframe_granule_shift);

    return (iframe+pframe)*
      ((double)cpi->info.fps_denominator/cpi->info.fps_numerator);

  }
#endif

  return(-1); /* negative granulepos or float calculations disabled */
}

/* returns frame number of current packet in given logical stream */
static ogg_int64_t theora_encode_granule_frame(theora_state *th,
 ogg_int64_t granulepos){
  CP_INSTANCE *cpi=(CP_INSTANCE *)(th->internal_encode);
  PB_INSTANCE *pbi=(PB_INSTANCE *)(th->internal_decode);

  if(cpi)pbi=&cpi->pb;

  if(granulepos>=0){
    ogg_int64_t iframe=granulepos>>cpi->keyframe_granule_shift;
    ogg_int64_t pframe=granulepos-(iframe<<cpi->keyframe_granule_shift);

    return (iframe+pframe);
  }

  return(-1);
}


static int theora_encode_control(theora_state *th,int req,
 void *buf,size_t buf_sz) {
  CP_INSTANCE *cpi;
  PB_INSTANCE *pbi;
  int value;
  
  if(th == NULL)
    return TH_EFAULT;

  cpi = th->internal_encode;
  pbi = &cpi->pb;
  
  switch(req) {
    case TH_ENCCTL_SET_QUANT_PARAMS:
      if( ( buf==NULL&&buf_sz!=0 )
  	   || ( buf!=NULL&&buf_sz!=sizeof(th_quant_info) )
  	   || cpi->HeadersWritten ){
        return TH_EINVAL;
      }
      
      memcpy(&pbi->quant_info, buf, sizeof(th_quant_info));
      InitQTables(pbi);
      
      return 0;
    case TH_ENCCTL_SET_VP3_COMPATIBLE:
      if(cpi->HeadersWritten)
        return TH_EINVAL;
      
      memcpy(&pbi->quant_info, &TH_VP31_QUANT_INFO, sizeof(th_quant_info));
      InitQTables(pbi);
      
      return 0;
    case TH_ENCCTL_SET_SPLEVEL:
      if(buf == NULL || buf_sz != sizeof(int))
        return TH_EINVAL;
      
      memcpy(&value, buf, sizeof(int));
            
      switch(value) {
        case 0:
          cpi->MotionCompensation = 1;
          cpi->info.quick_p = 0;
        break;
        
        case 1:
          cpi->MotionCompensation = 1;
          cpi->info.quick_p = 1;
        break;
        
        case 2:
          cpi->MotionCompensation = 0;
          cpi->info.quick_p = 1;
        break;
        
        default:
          return TH_EINVAL;    
      }
      
      return 0;
    case TH_ENCCTL_GET_SPLEVEL_MAX:
      value = 2;
      memcpy(buf, &value, sizeof(int));
      return 0;
    default:
      return TH_EIMPL;
  }
}

static void theora_encode_dispatch_init(CP_INSTANCE *cpi){
  cpi->dispatch_vtbl.clear=theora_encode_clear;
  cpi->dispatch_vtbl.control=theora_encode_control;
  cpi->dispatch_vtbl.granule_frame=theora_encode_granule_frame;
  cpi->dispatch_vtbl.granule_time=theora_encode_granule_time;
}
