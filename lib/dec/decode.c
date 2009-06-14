/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2008                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "decint.h"
#if defined(OC_DUMP_IMAGES)
# include <stdio.h>
# include "png.h"
#endif
#if defined(HAVE_CAIRO)
# include <cairo.h>
#endif


/*No post-processing.*/
#define OC_PP_LEVEL_DISABLED  (0)
/*Keep track of DC qi for each block only.*/
#define OC_PP_LEVEL_TRACKDCQI (1)
/*Deblock the luma plane.*/
#define OC_PP_LEVEL_DEBLOCKY  (2)
/*Dering the luma plane.*/
#define OC_PP_LEVEL_DERINGY   (3)
/*Stronger luma plane deringing.*/
#define OC_PP_LEVEL_SDERINGY  (4)
/*Deblock the chroma planes.*/
#define OC_PP_LEVEL_DEBLOCKC  (5)
/*Dering the chroma planes.*/
#define OC_PP_LEVEL_DERINGC   (6)
/*Stronger chroma plane deringing.*/
#define OC_PP_LEVEL_SDERINGC  (7)
/*Maximum valid post-processing level.*/
#define OC_PP_LEVEL_MAX       (7)



/*The mode alphabets for the various mode coding schemes.
  Scheme 0 uses a custom alphabet, which is not stored in this table.*/
static const unsigned char OC_MODE_ALPHABETS[7][OC_NMODES]={
  /*Last MV dominates */
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_MV,
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_NOMV,
    OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV,OC_MODE_INTER_MV_LAST2,
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV,OC_MODE_INTER_NOMV,
    OC_MODE_INTER_MV_LAST2,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,
    OC_MODE_GOLDEN_MV,OC_MODE_INTER_MV_FOUR
  },
  /*No MV dominates.*/
  {
    OC_MODE_INTER_NOMV,OC_MODE_INTER_MV_LAST,OC_MODE_INTER_MV_LAST2,
    OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  {
    OC_MODE_INTER_NOMV,OC_MODE_GOLDEN_NOMV,OC_MODE_INTER_MV_LAST,
    OC_MODE_INTER_MV_LAST2,OC_MODE_INTER_MV,OC_MODE_INTRA,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  },
  /*Default ordering.*/
  {
    OC_MODE_INTER_NOMV,OC_MODE_INTRA,OC_MODE_INTER_MV,OC_MODE_INTER_MV_LAST,
    OC_MODE_INTER_MV_LAST2,OC_MODE_GOLDEN_NOMV,OC_MODE_GOLDEN_MV,
    OC_MODE_INTER_MV_FOUR
  }
};


static int oc_sb_run_unpack(oggpack_buffer *_opb){
  long bits;
  int ret;
  /*Coding scheme:
       Codeword            Run Length
     0                       1
     10x                     2-3
     110x                    4-5
     1110xx                  6-9
     11110xxx                10-17
     111110xxxx              18-33
     111111xxxxxxxxxxxx      34-4129*/
  theorapackB_read1(_opb,&bits);
  if(bits==0)return 1;
  theorapackB_read(_opb,2,&bits);
  if((bits&2)==0)return 2+(int)bits;
  else if((bits&1)==0){
    theorapackB_read1(_opb,&bits);
    return 4+(int)bits;
  }
  theorapackB_read(_opb,3,&bits);
  if((bits&4)==0)return 6+(int)bits;
  else if((bits&2)==0){
    ret=10+((bits&1)<<2);
    theorapackB_read(_opb,2,&bits);
    return ret+(int)bits;
  }
  else if((bits&1)==0){
    theorapackB_read(_opb,4,&bits);
    return 18+(int)bits;
  }
  theorapackB_read(_opb,12,&bits);
  return 34+(int)bits;
}

static int oc_block_run_unpack(oggpack_buffer *_opb){
  long bits;
  long bits2;
  /*Coding scheme:
     Codeword             Run Length
     0x                      1-2
     10x                     3-4
     110x                    5-6
     1110xx                  7-10
     11110xx                 11-14
     11111xxxx               15-30*/
  theorapackB_read(_opb,2,&bits);
  if((bits&2)==0)return 1+(int)bits;
  else if((bits&1)==0){
    theorapackB_read1(_opb,&bits);
    return 3+(int)bits;
  }
  theorapackB_read(_opb,2,&bits);
  if((bits&2)==0)return 5+(int)bits;
  else if((bits&1)==0){
    theorapackB_read(_opb,2,&bits);
    return 7+(int)bits;
  }
  theorapackB_read(_opb,3,&bits);
  if((bits&4)==0)return 11+bits;
  theorapackB_read(_opb,2,&bits2);
  return 15+((bits&3)<<2)+bits2;
}



static int oc_dec_init(oc_dec_ctx *_dec,const th_info *_info,
 const th_setup_info *_setup){
  int qti;
  int pli;
  int qi;
  int ret;
  ret=oc_state_init(&_dec->state,_info,3);
  if(ret<0)return ret;
  oc_huff_trees_copy(_dec->huff_tables,
   (const oc_huff_node *const *)_setup->huff_tables);
  for(qi=0;qi<64;qi++)for(pli=0;pli<3;pli++)for(qti=0;qti<2;qti++){
    _dec->state.dequant_tables[qi][pli][qti]=
     _dec->state.dequant_table_data[qi][pli][qti];
  }
  oc_dequant_tables_init(_dec->state.dequant_tables,_dec->pp_dc_scale,
   &_setup->qinfo);
  for(qi=0;qi<64;qi++){
    int qsum;
    qsum=0;
    for(qti=0;qti<2;qti++)for(pli=0;pli<3;pli++){
      qsum+=_dec->state.dequant_tables[qti][pli][qi][12]+
       _dec->state.dequant_tables[qti][pli][qi][17]+
       _dec->state.dequant_tables[qti][pli][qi][18]+
       _dec->state.dequant_tables[qti][pli][qi][24]<<(pli==0);
    }
    _dec->pp_sharp_mod[qi]=-(qsum>>11);
  }
  _dec->dct_tokens=(unsigned char **)oc_calloc_2d(64,
   _dec->state.nfrags,sizeof(_dec->dct_tokens[0][0]));
  _dec->extra_bits=(ogg_uint16_t **)oc_calloc_2d(64,
   _dec->state.nfrags,sizeof(_dec->extra_bits[0][0]));
  memcpy(_dec->state.loop_filter_limits,_setup->qinfo.loop_filter_limits,
   sizeof(_dec->state.loop_filter_limits));
  _dec->pp_level=OC_PP_LEVEL_DISABLED;
  _dec->dc_qis=NULL;
  _dec->variances=NULL;
  _dec->pp_frame_data=NULL;
  _dec->stripe_cb.ctx=NULL;
  _dec->stripe_cb.stripe_decoded=NULL;
#if defined(HAVE_CAIRO)
  _dec->telemetry=0;
  _dec->telemetry_mbmode=0;
  _dec->telemetry_mv=0;
  _dec->telemetry_frame_data=NULL;
#endif
  return 0;
}

static void oc_dec_clear(oc_dec_ctx *_dec){
#if defined(HAVE_CAIRO)
  _ogg_free(_dec->telemetry_frame_data);
#endif
  _ogg_free(_dec->pp_frame_data);
  _ogg_free(_dec->variances);
  _ogg_free(_dec->dc_qis);
  oc_free_2d(_dec->extra_bits);
  oc_free_2d(_dec->dct_tokens);
  oc_huff_trees_clear(_dec->huff_tables);
  oc_state_clear(&_dec->state);
}


static int oc_dec_frame_header_unpack(oc_dec_ctx *_dec){
  long val;
  /*Check to make sure this is a data packet.*/
  theorapackB_read1(&_dec->opb,&val);
  if(val!=0)return TH_EBADPACKET;
  /*Read in the frame type (I or P).*/
  theorapackB_read1(&_dec->opb,&val);
  _dec->state.frame_type=(int)val;
  /*Read in the qi list.*/
  theorapackB_read(&_dec->opb,6,&val);
  _dec->state.qis[0]=(unsigned char)val;
  theorapackB_read1(&_dec->opb,&val);
  if(!val)_dec->state.nqis=1;
  else{
    theorapackB_read(&_dec->opb,6,&val);
    _dec->state.qis[1]=(unsigned char)val;
    theorapackB_read1(&_dec->opb,&val);
    if(!val)_dec->state.nqis=2;
    else{
      theorapackB_read(&_dec->opb,6,&val);
      _dec->state.qis[2]=(unsigned char)val;
      _dec->state.nqis=3;
    }
  }
  if(_dec->state.frame_type==OC_INTRA_FRAME){
    /*Keyframes have 3 unused configuration bits, holdovers from VP3 days.
      Most of the other unused bits in the VP3 headers were eliminated.
      I don't know why these remain.*/
    /*I wanted to eliminate wasted bits, but not all config wiggle room
       --Monty.*/
    theorapackB_read(&_dec->opb,3,&val);
    if(val!=0)return TH_EIMPL;
  }
  return 0;
}

/*Mark all fragments as coded and in OC_MODE_INTRA.
  This also builds up the coded fragment list (in coded order), and clears the
   uncoded fragment list.
  It does not update the coded macro block list nor the super block flags, as
   those are not used when decoding INTRA frames.*/
static void oc_dec_mark_all_intra(oc_dec_ctx *_dec){
  const oc_sb_map   *sb_maps;
  const oc_sb_flags *sb_flags;
  oc_fragment       *frags;
  ptrdiff_t         *coded_fragis;
  ptrdiff_t          ncoded_fragis;
  ptrdiff_t          prev_ncoded_fragis;
  unsigned           nsbs;
  unsigned           sbi;
  int                pli;
  coded_fragis=_dec->state.coded_fragis;
  prev_ncoded_fragis=ncoded_fragis=0;
  sb_maps=(const oc_sb_map *)_dec->state.sb_maps;
  sb_flags=_dec->state.sb_flags;
  frags=_dec->state.frags;
  sbi=nsbs=0;
  for(pli=0;pli<3;pli++){
    nsbs+=_dec->state.fplanes[pli].nsbs;
    for(;sbi<nsbs;sbi++){
      int quadi;
      for(quadi=0;quadi<4;quadi++)if(sb_flags[sbi].quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++){
          ptrdiff_t fragi;
          fragi=sb_maps[sbi][quadi][bi];
          if(fragi>=0){
            frags[fragi].coded=1;
            frags[fragi].mb_mode=OC_MODE_INTRA;
            coded_fragis[ncoded_fragis++]=fragi;
          }
        }
      }
    }
    _dec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
  }
}

/*Decodes the bit flags indicating whether each super block is partially coded
   or not.
  Return: The number of partially coded super blocks.*/
static unsigned oc_dec_partial_sb_flags_unpack(oc_dec_ctx *_dec){
  oc_sb_flags *sb_flags;
  unsigned     nsbs;
  unsigned     sbi;
  unsigned     npartial;
  unsigned     run_count;
  long         val;
  int          flag;
  theorapackB_read1(&_dec->opb,&val);
  flag=(int)val;
  sb_flags=_dec->state.sb_flags;
  nsbs=_dec->state.nsbs;
  sbi=run_count=npartial=0;
  while(sbi<nsbs){
    int full_run;
    run_count=oc_sb_run_unpack(&_dec->opb);
    full_run=run_count>=4129;
    do{
      sb_flags[sbi].coded_partially=flag;
      sb_flags[sbi].coded_fully=0;
      npartial+=flag;
      sbi++;
    }
    while(--run_count>0&&sbi<nsbs);
    if(full_run&&sbi<nsbs){
      theorapackB_read1(&_dec->opb,&val);
      flag=(int)val;
    }
    else flag=!flag;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
  return npartial;
}

/*Decodes the bit flags for whether or not each non-partially-coded super
   block is fully coded or not.
  This function should only be called if there is at least one
   non-partially-coded super block.
  Return: The number of partially coded super blocks.*/
static void oc_dec_coded_sb_flags_unpack(oc_dec_ctx *_dec){
  oc_sb_flags *sb_flags;
  unsigned     nsbs;
  unsigned     sbi;
  unsigned     run_count;
  long         val;
  int          flag;
  sb_flags=_dec->state.sb_flags;
  nsbs=_dec->state.nsbs;
  /*Skip partially coded super blocks.*/
  for(sbi=0;sb_flags[sbi].coded_partially;sbi++);
  theorapackB_read1(&_dec->opb,&val);
  flag=(int)val;
  do{
    int full_run;
    run_count=oc_sb_run_unpack(&_dec->opb);
    full_run=run_count>=4129;
    for(;sbi<nsbs;sbi++){
      if(sb_flags[sbi].coded_partially)continue;
      if(run_count--<=0)break;
      sb_flags[sbi].coded_fully=flag;
    }
    if(full_run&&sbi<nsbs){
      theorapackB_read1(&_dec->opb,&val);
      flag=(int)val;
    }
    else flag=!flag;
  }
  while(sbi<nsbs);
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}

static void oc_dec_coded_flags_unpack(oc_dec_ctx *_dec){
  const oc_sb_map   *sb_maps;
  const oc_sb_flags *sb_flags;
  oc_fragment       *frags;
  unsigned           nsbs;
  unsigned           sbi;
  unsigned           npartial;
  long               val;
  int                pli;
  int                flag;
  int                run_count;
  ptrdiff_t         *coded_fragis;
  ptrdiff_t         *uncoded_fragis;
  ptrdiff_t          ncoded_fragis;
  ptrdiff_t          nuncoded_fragis;
  ptrdiff_t          prev_ncoded_fragis;
  npartial=oc_dec_partial_sb_flags_unpack(_dec);
  if(npartial<_dec->state.nsbs)oc_dec_coded_sb_flags_unpack(_dec);
  if(npartial>0){
    theorapackB_read1(&_dec->opb,&val);
    flag=!(int)val;
  }
  else flag=0;
  sb_maps=(const oc_sb_map *)_dec->state.sb_maps;
  sb_flags=_dec->state.sb_flags;
  frags=_dec->state.frags;
  sbi=nsbs=run_count=0;
  coded_fragis=_dec->state.coded_fragis;
  uncoded_fragis=coded_fragis+_dec->state.nfrags;
  prev_ncoded_fragis=ncoded_fragis=nuncoded_fragis=0;
  for(pli=0;pli<3;pli++){
    nsbs+=_dec->state.fplanes[pli].nsbs;
    for(;sbi<nsbs;sbi++){
      int quadi;
      for(quadi=0;quadi<4;quadi++)if(sb_flags[sbi].quad_valid&1<<quadi){
        int bi;
        for(bi=0;bi<4;bi++){
          ptrdiff_t fragi;
          fragi=sb_maps[sbi][quadi][bi];
          if(fragi>=0){
            int coded;
            if(sb_flags[sbi].coded_fully)coded=1;
            else if(!sb_flags[sbi].coded_partially)coded=0;
            else{
              if(run_count<=0){
                run_count=oc_block_run_unpack(&_dec->opb);
                flag=!flag;
              }
              run_count--;
              coded=flag;
            }
            if(coded)coded_fragis[ncoded_fragis++]=fragi;
            else *(uncoded_fragis-++nuncoded_fragis)=fragi;
            frags[fragi].coded=coded;
          }
        }
      }
    }
    _dec->state.ncoded_fragis[pli]=ncoded_fragis-prev_ncoded_fragis;
    prev_ncoded_fragis=ncoded_fragis;
  }
  /*TODO: run_count should be 0 here.
    If it's not, we should issue a warning of some kind.*/
}



typedef int (*oc_mode_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mode_unpack(oggpack_buffer *_opb){
  long val;
  int  i;
  for(i=0;i<7;i++){
    theorapackB_read1(_opb,&val);
    if(!val)break;
  }
  return i;
}

static int oc_clc_mode_unpack(oggpack_buffer *_opb){
  long val;
  theorapackB_read(_opb,3,&val);
  return (int)val;
}

/*Unpacks the list of macro block modes for INTER frames.*/
static void oc_dec_mb_modes_unpack(oc_dec_ctx *_dec){
  const oc_mb_map     *mb_maps;
  signed char         *mb_modes;
  const oc_fragment   *frags;
  const unsigned char *alphabet;
  unsigned char        scheme0_alphabet[8];
  oc_mode_unpack_func  mode_unpack;
  size_t               nmbs;
  size_t               mbi;
  long                 val;
  int                  mode_scheme;
  theorapackB_read(&_dec->opb,3,&val);
  mode_scheme=(int)val;
  if(mode_scheme==0){
    int mi;
    /*Just in case, initialize the modes to something.
      If the bitstream doesn't contain each index exactly once, it's likely
       corrupt and the rest of the packet is garbage anyway, but this way we
       won't crash, and we'll decode SOMETHING.*/
    /*LOOP VECTORIZES*/
    for(mi=0;mi<OC_NMODES;mi++)scheme0_alphabet[mi]=OC_MODE_INTER_NOMV;
    for(mi=0;mi<OC_NMODES;mi++){
      theorapackB_read(&_dec->opb,3,&val);
      scheme0_alphabet[val]=OC_MODE_ALPHABETS[6][mi];
    }
    alphabet=scheme0_alphabet;
  }
  else alphabet=OC_MODE_ALPHABETS[mode_scheme-1];
  if(mode_scheme==7)mode_unpack=oc_clc_mode_unpack;
  else mode_unpack=oc_vlc_mode_unpack;
  mb_modes=_dec->state.mb_modes;
  mb_maps=(const oc_mb_map *)_dec->state.mb_maps;
  nmbs=_dec->state.nmbs;
  frags=_dec->state.frags;
  for(mbi=0;mbi<nmbs;mbi++){
    if(mb_modes[mbi]!=OC_MODE_INVALID){
      int bi;
      /*Check for a coded luma block in this macro block.*/
      for(bi=0;bi<4&&!frags[mb_maps[mbi][0][bi]].coded;bi++);
      /*We found one, decode a mode.*/
      if(bi<4)mb_modes[mbi]=alphabet[(*mode_unpack)(&_dec->opb)];
      /*There were none: INTER_NOMV is forced.*/
      else mb_modes[mbi]=OC_MODE_INTER_NOMV;
    }
  }
}



typedef int (*oc_mv_comp_unpack_func)(oggpack_buffer *_opb);

static int oc_vlc_mv_comp_unpack(oggpack_buffer *_opb){
  long bits;
  int  mask;
  int  mv;
  theorapackB_read(_opb,3,&bits);
  switch(bits){
    case  0:return 0;
    case  1:return 1;
    case  2:return -1;
    case  3:
    case  4:{
      mv=(int)(bits-1);
      theorapackB_read1(_opb,&bits);
    }break;
    /*case  5:
    case  6:
    case  7:*/
    default:{
      mv=1<<bits-3;
      theorapackB_read(_opb,bits-2,&bits);
      mv+=(int)(bits>>1);
      bits&=1;
    }break;
  }
  mask=-(int)bits;
  return mv+mask^mask;
}

static int oc_clc_mv_comp_unpack(oggpack_buffer *_opb){
  long bits;
  int  mask;
  int  mv;
  theorapackB_read(_opb,6,&bits);
  mv=(int)bits>>1;
  mask=-((int)bits&1);
  return mv+mask^mask;
}

/*Unpacks the list of motion vectors for INTER frames, and propagtes the macro
   block modes and motion vectors to the individual fragments.*/
static void oc_dec_mv_unpack_and_frag_modes_fill(oc_dec_ctx *_dec){
  const oc_mb_map        *mb_maps;
  const signed char      *mb_modes;
  oc_set_chroma_mvs_func  set_chroma_mvs;
  oc_mv_comp_unpack_func  mv_comp_unpack;
  oc_fragment            *frags;
  oc_mv                  *frag_mvs;
  const unsigned char    *map_idxs;
  int                     map_nidxs;
  oc_mv                   last_mv[2];
  oc_mv                   cbmvs[4];
  size_t                  nmbs;
  size_t                  mbi;
  long                    val;
  set_chroma_mvs=OC_SET_CHROMA_MVS_TABLE[_dec->state.info.pixel_fmt];
  theorapackB_read1(&_dec->opb,&val);
  mv_comp_unpack=val?oc_clc_mv_comp_unpack:oc_vlc_mv_comp_unpack;
  map_idxs=OC_MB_MAP_IDXS[_dec->state.info.pixel_fmt];
  map_nidxs=OC_MB_MAP_NIDXS[_dec->state.info.pixel_fmt];
  memset(last_mv,0,sizeof(last_mv));
  frags=_dec->state.frags;
  frag_mvs=_dec->state.frag_mvs;
  mb_maps=(const oc_mb_map *)_dec->state.mb_maps;
  mb_modes=_dec->state.mb_modes;
  nmbs=_dec->state.nmbs;
  for(mbi=0;mbi<nmbs;mbi++){
    int          mb_mode;
    mb_mode=mb_modes[mbi];
    if(mb_mode!=OC_MODE_INVALID){
      oc_mv        mbmv;
      ptrdiff_t    fragi;
      int          coded[13];
      int          codedi;
      int          ncoded;
      int          mapi;
      int          mapii;
      /*Search for at least one coded fragment.*/
      ncoded=mapii=0;
      do{
        mapi=map_idxs[mapii];
        fragi=mb_maps[mbi][mapi>>2][mapi&3];
        if(frags[fragi].coded)coded[ncoded++]=mapi;
      }
      while(++mapii<map_nidxs);
      if(ncoded<=0)continue;
      switch(mb_mode){
        case OC_MODE_INTER_MV_FOUR:{
          oc_mv       lbmvs[4];
          int         bi;
          /*Mark the tail of the list, so we don't accidentally go past it.*/
          coded[ncoded]=-1;
          for(bi=codedi=0;bi<4;bi++){
            if(coded[codedi]==bi){
              codedi++;
              fragi=mb_maps[mbi][0][bi];
              frags[fragi].mb_mode=mb_mode;
              lbmvs[bi][0]=(signed char)(*mv_comp_unpack)(&_dec->opb);
              lbmvs[bi][1]=(signed char)(*mv_comp_unpack)(&_dec->opb);
              memcpy(frag_mvs[fragi],lbmvs[bi],sizeof(lbmvs[bi]));
            }
            else lbmvs[bi][0]=lbmvs[bi][1]=0;
          }
          if(codedi>0){
            memcpy(last_mv[1],last_mv[0],sizeof(last_mv[1]));
            memcpy(last_mv[0],lbmvs[coded[codedi-1]],sizeof(last_mv[0]));
          }
          if(codedi<ncoded){
            (*set_chroma_mvs)(cbmvs,(const oc_mv *)lbmvs);
            for(;codedi<ncoded;codedi++){
              mapi=coded[codedi];
              bi=mapi&3;
              fragi=mb_maps[mbi][mapi>>2][bi];
              frags[fragi].mb_mode=mb_mode;
              memcpy(frag_mvs[fragi],cbmvs[bi],sizeof(cbmvs[bi]));
            }
          }
        }break;
        case OC_MODE_INTER_MV:{
          memcpy(last_mv[1],last_mv[0],sizeof(last_mv[1]));
          mbmv[0]=last_mv[0][0]=(signed char)(*mv_comp_unpack)(&_dec->opb);
          mbmv[1]=last_mv[0][1]=(signed char)(*mv_comp_unpack)(&_dec->opb);
        }break;
        case OC_MODE_INTER_MV_LAST:memcpy(mbmv,last_mv[0],sizeof(mbmv));break;
        case OC_MODE_INTER_MV_LAST2:{
          memcpy(mbmv,last_mv[1],sizeof(mbmv));
          memcpy(last_mv[1],last_mv[0],sizeof(last_mv[1]));
          memcpy(last_mv[0],mbmv,sizeof(last_mv[0]));
        }break;
        case OC_MODE_GOLDEN_MV:{
          mbmv[0]=(signed char)(*mv_comp_unpack)(&_dec->opb);
          mbmv[1]=(signed char)(*mv_comp_unpack)(&_dec->opb);
        }break;
        default:memset(mbmv,0,sizeof(mbmv));break;
      }
      /*4MV mode fills in the fragments itself.
        For all other modes we can use this common code.*/
      if(mb_mode!=OC_MODE_INTER_MV_FOUR){
        for(codedi=0;codedi<ncoded;codedi++){
          mapi=coded[codedi];
          fragi=mb_maps[mbi][mapi>>2][mapi&3];
          frags[fragi].mb_mode=mb_mode;
          memcpy(frag_mvs[fragi],mbmv,sizeof(mbmv));
        }
      }
    }
  }
}

static void oc_dec_block_qis_unpack(oc_dec_ctx *_dec){
  oc_fragment     *frags;
  const ptrdiff_t *coded_fragis;
  ptrdiff_t        ncoded_fragis;
  ptrdiff_t        fragii;
  ptrdiff_t        fragi;
  ncoded_fragis=_dec->state.ntotal_coded_fragis;
  if(ncoded_fragis<=0)return;
  frags=_dec->state.frags;
  coded_fragis=_dec->state.coded_fragis;
  if(_dec->state.nqis==1){
    /*If this frame has only a single qi value, then just use it for all coded
       fragments.*/
    for(fragii=0;fragii<ncoded_fragis;fragii++){
      frags[coded_fragis[fragii]].qii=0;
    }
  }
  else{
    long val;
    int  flag;
    int  nqi1;
    int  run_count;
    /*Otherwise, we decode a qi index for each fragment, using two passes of
      the same binary RLE scheme used for super-block coded bits.
     The first pass marks each fragment as having a qii of 0 or greater than
      0, and the second pass (if necessary), distinguishes between a qii of
      1 and 2.
     At first we just store the qii in the fragment.
     After all the qii's are decoded, we make a final pass to replace them
      with the corresponding qi's for this frame.*/
    theorapackB_read1(&_dec->opb,&val);
    flag=(int)val;
    run_count=nqi1=0;
    fragii=0;
    while(fragii<ncoded_fragis){
      int full_run;
      run_count=oc_sb_run_unpack(&_dec->opb);
      full_run=run_count>=4129;
      do{
        frags[coded_fragis[fragii++]].qii=flag;
        nqi1+=flag;
      }
      while(--run_count>0&&fragii<ncoded_fragis);
      if(full_run&&fragii<ncoded_fragis){
        theorapackB_read1(&_dec->opb,&val);
        flag=(int)val;
      }
      else flag=!flag;
    }
    /*TODO: run_count should be 0 here.
      If it's not, we should issue a warning of some kind.*/
    /*If we have 3 different qi's for this frame, and there was at least one
       fragment with a non-zero qi, make the second pass.*/
    if(_dec->state.nqis==3&&nqi1>0){
      /*Skip qii==0 fragments.*/
      for(fragii=0;frags[coded_fragis[fragii]].qii==0;fragii++);
      theorapackB_read1(&_dec->opb,&val);
      flag=(int)val;
      do{
        int full_run;
        run_count=oc_sb_run_unpack(&_dec->opb);
        full_run=run_count>=4129;
        for(;fragii<ncoded_fragis;fragii++){
          fragi=coded_fragis[fragii];
          if(frags[fragi].qii==0)continue;
          if(run_count--<=0)break;
          frags[fragi].qii+=flag;
        }
        if(full_run&&fragii<ncoded_fragis){
          theorapackB_read1(&_dec->opb,&val);
          flag=(int)val;
        }
        else flag=!flag;
      }
      while(fragii<ncoded_fragis);
      /*TODO: run_count should be 0 here.
        If it's not, we should issue a warning of some kind.*/
    }
  }
}



/*Returns the decoded value of the first coefficient produced by the given
   token.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: The decoded coefficient value.*/
typedef int (*oc_token_dec1val_func)(int _token,int _extra_bits);

/*We want to avoid accessing arrays of constants in these functions, because
   we take the address of them, which means that when compiling with -fPIC,
   an expensive prolog is added to set up the PIC register in any functions
   which access a global symbol (even if it has file scope or smaller).
  Thus a lot of what would be tables are packed into 32-bit constants.*/

/*Handles zero run tokens.*/
static int oc_token_dec1val_zrl(void){
  return 0;
}

/*Handles 1, -1, 2 and -2 tokens.*/
static int oc_token_dec1val_const(int _token){
  return OC_BYTE_TABLE32(1,-1,2,-2,_token-OC_NDCT_ZRL_TOKEN_MAX);
}

/*Handles DCT value tokens category 2.*/
static int oc_token_dec1val_cat2(int _token,int _extra_bits){
  int mask;
  mask=-_extra_bits;
  return _token-OC_DCT_VAL_CAT2+3+mask^mask;
}

/*Handles DCT value tokens categories 3 through 6.*/
static int oc_token_dec1val_cat3_6(int _token,int _extra_bits){
  int cati;
  int mask;
  int val_cat_offs;
  int val_cat_shift;
  cati=_token-OC_DCT_VAL_CAT3;
  val_cat_shift=cati+1;
  mask=-(_extra_bits>>val_cat_shift);
  _extra_bits&=(1<<val_cat_shift)-1;
  val_cat_offs=OC_BYTE_TABLE32(7,9,13,21,cati);
  return val_cat_offs+_extra_bits+mask^mask;
}

/*Handles DCT value tokens categories 7 through 8.*/
static int oc_token_dec1val_cat7_8(int _token,int _extra_bits){
  int cati;
  int mask;
  int val_cat_offs;
  int val_cat_shift;
  cati=_token-OC_DCT_VAL_CAT7;
  val_cat_shift=5+(cati<<2);
  mask=-(_extra_bits>>val_cat_shift);
  _extra_bits&=(1<<val_cat_shift)-1;
  val_cat_offs=37+(cati<<5);
  return val_cat_offs+_extra_bits+mask^mask;
}

/*A jump table for computing the first coefficient value the given token value
   represents.*/
static const oc_token_dec1val_func OC_TOKEN_DEC1VAL_TABLE[TH_NDCT_TOKENS-
 OC_NDCT_EOB_TOKEN_MAX]={
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  (oc_token_dec1val_func)oc_token_dec1val_const,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat2,
  oc_token_dec1val_cat3_6,
  oc_token_dec1val_cat3_6,
  oc_token_dec1val_cat3_6,
  oc_token_dec1val_cat3_6,
  oc_token_dec1val_cat7_8,
  oc_token_dec1val_cat7_8,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl,
  (oc_token_dec1val_func)oc_token_dec1val_zrl
};

/*Returns the decoded value of the first coefficient produced by the given
   token.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to skip.
  _extra_bits: The extra bits attached to this token.
  Return: The decoded coefficient value.*/
static int oc_dct_token_dec1val(int _token,int _extra_bits){
  return (*OC_TOKEN_DEC1VAL_TABLE[_token-OC_NDCT_EOB_TOKEN_MAX])(_token,
   _extra_bits);
}

/*Unpacks the DC coefficient tokens.
  Unlike when unpacking the AC coefficient tokens, we actually need to decode
   the DC coefficient values now so that we can do DC prediction.
  _huff_idx:   The index of the Huffman table to use for each color plane.
  _ntoks_left: The number of tokens left to be decoded in each color plane for
                each coefficient.
               This is updated as EOB tokens and zero run tokens are decoded.
  Return: The length of any outstanding EOB run.*/
static ptrdiff_t oc_dec_dc_coeff_unpack(oc_dec_ctx *_dec,int _huff_idxs[2],
 ptrdiff_t _ntoks_left[3][64]){
  unsigned char   *dct_tokens;
  ogg_uint16_t    *extra_bits;
  oc_fragment     *frags;
  const ptrdiff_t *coded_fragis;
  ptrdiff_t        ncoded_fragis;
  ptrdiff_t        fragii;
  ptrdiff_t        eobs;
  ptrdiff_t        ti;
  ptrdiff_t        ebi;
  int              pli;
  dct_tokens=_dec->dct_tokens[0];
  extra_bits=_dec->extra_bits[0];
  frags=_dec->state.frags;
  coded_fragis=_dec->state.coded_fragis;
  ncoded_fragis=fragii=eobs=ti=ebi=0;
  for(pli=0;pli<3;pli++){
    ptrdiff_t run_counts[64];
    ptrdiff_t eob_count;
    ptrdiff_t eobi;
    int       rli;
    ncoded_fragis+=_dec->state.ncoded_fragis[pli];
    memset(run_counts,0,sizeof(run_counts));
    _dec->eob_runs[pli][0]=eobs;
    /*Continue any previous EOB run, if there was one.*/
    eobi=eobs;
    if(ncoded_fragis-fragii<eobi)eobi=ncoded_fragis-fragii;
    eob_count=eobi;
    eobs-=eobi;
    while(eobi-->0)frags[coded_fragis[fragii++]].dc=0;
    while(fragii<ncoded_fragis){
      int token;
      int neb;
      int eb;
      int skip;
      token=oc_huff_token_decode(&_dec->opb,
       _dec->huff_tables[_huff_idxs[pli+1>>1]]);
      dct_tokens[ti++]=(unsigned char)token;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        long val;
        theorapackB_read(&_dec->opb,neb,&val);
        eb=(int)val;
        extra_bits[ebi++]=(ogg_uint16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0){
        eobs=eobi=-skip;
        if(ncoded_fragis-fragii<eobi)eobi=ncoded_fragis-fragii;
        eob_count+=eobi;
        eobs-=eobi;
        while(eobi-->0)frags[coded_fragis[fragii++]].dc=0;
      }
      else{
        run_counts[skip-1]++;
        eobs=0;
        frags[coded_fragis[fragii++]].dc=oc_dct_token_dec1val(token,eb);
      }
    }
    _dec->ti0[pli][0]=ti;
    _dec->ebi0[pli][0]=ebi;
    /*Add the total EOB count to the longest run length.*/
    run_counts[63]+=eob_count;
    /*And convert the run_counts array to a moment table.*/
    for(rli=63;rli-->0;)run_counts[rli]+=run_counts[rli+1];
    /*Finally, subtract off the number of coefficients that have been
       accounted for by runs started in this coefficient.*/
    for(rli=64;rli-->0;)_ntoks_left[pli][rli]-=run_counts[rli];
  }
  return eobs;
}

/*Unpacks the AC coefficient tokens.
  This can completely discard coefficient values while unpacking, and so is
   somewhat simpler than unpacking the DC coefficient tokens.
  _huff_idx:   The index of the Huffman table to use for each color plane.
  _ntoks_left: The number of tokens left to be decoded in each color plane for
                each coefficient.
               This is updated as EOB tokens and zero run tokens are decoded.
  _eobs:       The length of any outstanding EOB run from previous
                coefficients.
  Return: The length of any outstanding EOB run.*/
static int oc_dec_ac_coeff_unpack(oc_dec_ctx *_dec,int _zzi,int _huff_idxs[2],
 ptrdiff_t _ntoks_left[3][64],ptrdiff_t _eobs){
  unsigned char *dct_tokens;
  ogg_uint16_t  *extra_bits;
  ptrdiff_t      ti;
  ptrdiff_t      ebi;
  int            pli;
  dct_tokens=_dec->dct_tokens[_zzi];
  extra_bits=_dec->extra_bits[_zzi];
  ti=ebi=0;
  for(pli=0;pli<3;pli++){
    ptrdiff_t run_counts[64];
    ptrdiff_t ntoks_left;
    ptrdiff_t eob_count;
    ptrdiff_t ntoks;
    int       rli;
    _dec->eob_runs[pli][_zzi]=_eobs;
    ntoks_left=_ntoks_left[pli][_zzi];
    memset(run_counts,0,sizeof(run_counts));
    eob_count=0;
    ntoks=0;
    while(ntoks+_eobs<ntoks_left){
      int token;
      int neb;
      int eb;
      int skip;
      ntoks+=_eobs;
      eob_count+=_eobs;
      token=oc_huff_token_decode(&_dec->opb,
       _dec->huff_tables[_huff_idxs[pli+1>>1]]);
      dct_tokens[ti++]=(unsigned char)token;
      neb=OC_DCT_TOKEN_EXTRA_BITS[token];
      if(neb){
        long val;
        theorapackB_read(&_dec->opb,neb,&val);
        eb=(int)val;
        extra_bits[ebi++]=(ogg_uint16_t)eb;
      }
      else eb=0;
      skip=oc_dct_token_skip(token,eb);
      if(skip<0)_eobs=-skip;
      else{
        run_counts[skip-1]++;
        ntoks++;
        _eobs=0;
      }
    }
    _dec->ti0[pli][_zzi]=ti;
    _dec->ebi0[pli][_zzi]=ebi;
    /*Add the portion of the last EOB run actually used by this coefficient.*/
    eob_count+=ntoks_left-ntoks;
    /*And remove it from the remaining EOB count.*/
    _eobs-=ntoks_left-ntoks;
    /*Add the total EOB count to the longest run length.*/
    run_counts[63]+=eob_count;
    /*And convert the run_counts array to a moment table.*/
    for(rli=63;rli-->0;)run_counts[rli]+=run_counts[rli+1];
    /*Finally, subtract off the number of coefficients that have been
       accounted for by runs started in this coefficient.*/
    for(rli=64-_zzi;rli-->0;)_ntoks_left[pli][_zzi+rli]-=run_counts[rli];
  }
  return _eobs;
}

/*Tokens describing the DCT coefficients that belong to each fragment are
   stored in the bitstream grouped by coefficient, not by fragment.

  This means that we either decode all the tokens in order, building up a
   separate coefficient list for each fragment as we go, and then go back and
   do the iDCT on each fragment, or we have to create separate lists of tokens
   for each coefficient, so that we can pull the next token required off the
   head of the appropriate list when decoding a specific fragment.

  The former was VP3's choice, and it meant 2*w*h extra storage for all the
   decoded coefficient values.

  We take the second option, which lets us store just one or three bytes per
   token (generally far fewer than the number of coefficients, due to EOB
   tokens and zero runs), and which requires us to only maintain a counter for
   each of the 64 coefficients, instead of a counter for every fragment to
   determine where the next token goes.

  We actually use 3 counters per coefficient, one for each color plane, so we
   can decode all color planes simultaneously.
  This lets color conversion, etc., be done as soon as a full MCU (one or
   two super block rows) is decoded, while the image data is still in cache.*/

static void oc_dec_residual_tokens_unpack(oc_dec_ctx *_dec){
  static const unsigned char OC_HUFF_LIST_MAX[5]={1,6,15,28,64};
  ptrdiff_t  ntoks_left[3][64];
  int        huff_idxs[2];
  ptrdiff_t  eobs;
  long       val;
  int        pli;
  int        zzi;
  int        hgi;
  for(pli=0;pli<3;pli++)for(zzi=0;zzi<64;zzi++){
    ntoks_left[pli][zzi]=_dec->state.ncoded_fragis[pli];
  }
  theorapackB_read(&_dec->opb,4,&val);
  huff_idxs[0]=(int)val;
  theorapackB_read(&_dec->opb,4,&val);
  huff_idxs[1]=(int)val;
  _dec->eob_runs[0][0]=0;
  eobs=oc_dec_dc_coeff_unpack(_dec,huff_idxs,ntoks_left);
  theorapackB_read(&_dec->opb,4,&val);
  huff_idxs[0]=(int)val;
  theorapackB_read(&_dec->opb,4,&val);
  huff_idxs[1]=(int)val;
  zzi=1;
  for(hgi=1;hgi<5;hgi++){
    huff_idxs[0]+=16;
    huff_idxs[1]+=16;
    for(;zzi<OC_HUFF_LIST_MAX[hgi];zzi++){
      eobs=oc_dec_ac_coeff_unpack(_dec,zzi,huff_idxs,ntoks_left,eobs);
    }
  }
  /*TODO: eobs should be exactly zero, or 4096 or greater.
    The second case occurs when an EOB run of size zero is encountered, which
     gets treated as an infinite EOB run (where infinity is PTRDIFF_MAX).
    If neither of these conditions holds, then a warning should be issued.*/
}



/*Expands a single token into the given coefficient list.
  This fills in the zeros for zero runs as well as coefficient values, and
   updates the index of the current coefficient.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to expand.
  _extra_bits: The extra bits associated with the token.
  _dct_coeffs: The current list of coefficients, in zig-zag order.
  _zzi:        The zig-zag index of the next coefficient to write to.
  Return: The updated index of the next coefficient to write to.*/
typedef int (*oc_token_expand_func)(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi);

/*Expands a zero run token.*/
static int oc_token_expand_zrl(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  do _dct_coeffs[_zzi++]=0;
  while(_extra_bits-->0);
  return _zzi;
}

/*Expands a constant, single-value token.*/
static int oc_token_expand_const(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  _dct_coeffs[_zzi++]=(ogg_int16_t)oc_token_dec1val_const(_token);
  return _zzi;
}

/*Expands category 2 single-valued tokens.*/
static int oc_token_expand_cat2(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  _dct_coeffs[_zzi++]=(ogg_int16_t)oc_token_dec1val_cat2(_token,_extra_bits);
  return _zzi;
}

/*Expands category 3 through 6 single-valued tokens.*/
static int oc_token_expand_cat3_6(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  _dct_coeffs[_zzi++]=(ogg_int16_t)oc_token_dec1val_cat3_6(_token,_extra_bits);
  return _zzi;
}

/*Expands category 7 through 8 single-valued tokens.*/
static int oc_token_expand_cat7_8(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  _dct_coeffs[_zzi++]=(ogg_int16_t)oc_token_dec1val_cat7_8(_token,_extra_bits);
  return _zzi;
}

/*Expands a category 1a zero run/value combo token.*/
static int oc_token_expand_run_cat1a(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  int rl;
  /*LOOP VECTORIZES.*/
  for(rl=_token-OC_DCT_RUN_CAT1A+1;rl-->0;)_dct_coeffs[_zzi++]=0;
  _dct_coeffs[_zzi++]=(ogg_int16_t)(1-(_extra_bits<<1));
  return _zzi;
}

/*Expands all other zero run/value combo tokens.*/
static int oc_token_expand_run(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  int nzeros_mask;
  int nzeros_adjust;
  int sign_shift;
  int value_shift;
  int value_mask;
  int value_adjust;
  int mask;
  int rl;
  _token-=OC_DCT_RUN_CAT1B;
  nzeros_mask=OC_BYTE_TABLE32(3,7,0,1,_token);
  nzeros_adjust=OC_BYTE_TABLE32(6,10,1,2,_token);
  rl=(_extra_bits&nzeros_mask)+nzeros_adjust;
  /*LOOP VECTORIZES.*/
  while(rl-->0)_dct_coeffs[_zzi++]=0;
  sign_shift=OC_BYTE_TABLE32(2,3,1,2,_token);
  mask=-(_extra_bits>>sign_shift);
  value_shift=_token+1>>2;
  value_mask=_token>>1;
  value_adjust=value_mask+1;
  _dct_coeffs[_zzi++]=
   (ogg_int16_t)(value_adjust+(_extra_bits>>value_shift&value_mask)+mask^mask);
  return _zzi;
}

/*A jump table for expanding token values into coefficient values.
  This reduces all the conditional branches, etc., needed to parse these token
   values down to one indirect jump.*/
static const oc_token_expand_func OC_TOKEN_EXPAND_TABLE[TH_NDCT_TOKENS-
 OC_NDCT_EOB_TOKEN_MAX]={
  oc_token_expand_zrl,
  oc_token_expand_zrl,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_const,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cat2,
  oc_token_expand_cat3_6,
  oc_token_expand_cat3_6,
  oc_token_expand_cat3_6,
  oc_token_expand_cat3_6,
  oc_token_expand_cat7_8,
  oc_token_expand_cat7_8,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run_cat1a,
  oc_token_expand_run,
  oc_token_expand_run,
  oc_token_expand_run,
  oc_token_expand_run
};

/*Expands a single token into the given coefficient list.
  This fills in the zeros for zero runs as well as coefficient values, and
   updates the index of the current coefficient.
  It CANNOT be called for any of the EOB tokens.
  _token:      The token value to expand.
  _extra_bits: The extra bits associated with the token.
  _dct_coeffs: The current list of coefficients, in zig-zag order.
  _zzi:        The zig-zag index of the next coefficient to write to.
  Return: The updated index of the next coefficient to write to.*/
static int oc_dct_token_expand(int _token,int _extra_bits,
 ogg_int16_t _dct_coeffs[128],int _zzi){
  return (*OC_TOKEN_EXPAND_TABLE[_token-OC_NDCT_EOB_TOKEN_MAX])(_token,
   _extra_bits,_dct_coeffs,_zzi);
}



static int oc_dec_postprocess_init(oc_dec_ctx *_dec){
  /*pp_level 0: disabled; free any memory used and return*/
  if(_dec->pp_level<=OC_PP_LEVEL_DISABLED){
    if(_dec->dc_qis!=NULL){
      _ogg_free(_dec->dc_qis);
      _dec->dc_qis=NULL;
      _ogg_free(_dec->variances);
      _dec->variances=NULL;
      _ogg_free(_dec->pp_frame_data);
      _dec->pp_frame_data=NULL;
    }
    return 1;
  }
  if(_dec->dc_qis==NULL){
    /*If we haven't been tracking DC quantization indices, there's no point in
       starting now.*/
    if(_dec->state.frame_type!=OC_INTRA_FRAME)return 1;
    _dec->dc_qis=(unsigned char *)_ogg_malloc(
     _dec->state.nfrags*sizeof(_dec->dc_qis[0]));
    memset(_dec->dc_qis,_dec->state.qis[0],_dec->state.nfrags);
  }
  else{
    unsigned char   *dc_qis;
    const ptrdiff_t *coded_fragis;
    ptrdiff_t        ncoded_fragis;
    ptrdiff_t        fragii;
    unsigned char    qi0;
    /*Update the DC quantization index of each coded block.*/
    dc_qis=_dec->dc_qis;
    coded_fragis=_dec->state.coded_fragis;
    ncoded_fragis=_dec->state.ncoded_fragis[0]+
     _dec->state.ncoded_fragis[1]+_dec->state.ncoded_fragis[2];
    qi0=(unsigned char)_dec->state.qis[0];
    for(fragii=0;fragii<ncoded_fragis;fragii++){
      dc_qis[coded_fragis[fragii]]=qi0;
    }
  }
  /*pp_level 1: Stop after updating DC quantization indices.*/
  if(_dec->pp_level<=OC_PP_LEVEL_TRACKDCQI){
    if(_dec->variances!=NULL){
      _ogg_free(_dec->variances);
      _dec->variances=NULL;
      _ogg_free(_dec->pp_frame_data);
      _dec->pp_frame_data=NULL;
    }
    return 1;
  }
  if(_dec->variances==NULL||
   _dec->pp_frame_has_chroma!=(_dec->pp_level>=OC_PP_LEVEL_DEBLOCKC)){
    size_t frame_sz;
    frame_sz=_dec->state.info.frame_width*(size_t)_dec->state.info.frame_height;
    if(_dec->pp_level<OC_PP_LEVEL_DEBLOCKC){
      _dec->variances=(int *)_ogg_realloc(_dec->variances,
       _dec->state.fplanes[0].nfrags*sizeof(_dec->variances[0]));
      _dec->pp_frame_data=(unsigned char *)_ogg_realloc(
       _dec->pp_frame_data,frame_sz*sizeof(_dec->pp_frame_data[0]));
      _dec->pp_frame_buf[0].width=_dec->state.info.frame_width;
      _dec->pp_frame_buf[0].height=_dec->state.info.frame_height;
      _dec->pp_frame_buf[0].stride=-_dec->pp_frame_buf[0].width;
      _dec->pp_frame_buf[0].data=_dec->pp_frame_data+
       (1-_dec->pp_frame_buf[0].height)*(ptrdiff_t)_dec->pp_frame_buf[0].stride;
    }
    else{
      size_t y_sz;
      size_t c_sz;
      int    c_w;
      int    c_h;
      _dec->variances=(int *)_ogg_realloc(_dec->variances,
       _dec->state.nfrags*sizeof(_dec->variances[0]));
      y_sz=frame_sz;
      c_w=_dec->state.info.frame_width>>!(_dec->state.info.pixel_fmt&1);
      c_h=_dec->state.info.frame_height>>!(_dec->state.info.pixel_fmt&2);
      c_sz=c_w*c_h;
      frame_sz+=c_sz<<1;
      _dec->pp_frame_data=(unsigned char *)_ogg_realloc(
       _dec->pp_frame_data,frame_sz*sizeof(_dec->pp_frame_data[0]));
      _dec->pp_frame_buf[0].width=_dec->state.info.frame_width;
      _dec->pp_frame_buf[0].height=_dec->state.info.frame_height;
      _dec->pp_frame_buf[0].stride=_dec->pp_frame_buf[0].width;
      _dec->pp_frame_buf[0].data=_dec->pp_frame_data;
      _dec->pp_frame_buf[1].width=c_w;
      _dec->pp_frame_buf[1].height=c_h;
      _dec->pp_frame_buf[1].stride=_dec->pp_frame_buf[1].width;
      _dec->pp_frame_buf[1].data=_dec->pp_frame_buf[0].data+y_sz;
      _dec->pp_frame_buf[2].width=c_w;
      _dec->pp_frame_buf[2].height=c_h;
      _dec->pp_frame_buf[2].stride=_dec->pp_frame_buf[2].width;
      _dec->pp_frame_buf[2].data=_dec->pp_frame_buf[1].data+c_sz;
      oc_ycbcr_buffer_flip(_dec->pp_frame_buf,_dec->pp_frame_buf);
    }
    _dec->pp_frame_has_chroma=(_dec->pp_level>=OC_PP_LEVEL_DEBLOCKC);
  }
  /*If we're not processing chroma, copy the reference frame's chroma planes.*/
  if(_dec->pp_level<OC_PP_LEVEL_DEBLOCKC){
    memcpy(_dec->pp_frame_buf+1,
     _dec->state.ref_frame_bufs[_dec->state.ref_frame_idx[OC_FRAME_SELF]]+1,
     sizeof(_dec->pp_frame_buf[1])*2);
  }
  return 0;
}



typedef struct{
  int                 bounding_values[256];
  ptrdiff_t           ti[3][64];
  ptrdiff_t           ebi[3][64];
  ptrdiff_t           eob_runs[3][64];
  const ptrdiff_t    *coded_fragis[3];
  const ptrdiff_t    *uncoded_fragis[3];
  ptrdiff_t           ncoded_fragis[3];
  ptrdiff_t           nuncoded_fragis[3];
  const ogg_uint16_t *dequant[3][3][2];
  int                 fragy0[3];
  int                 fragy_end[3];
  int                 pred_last[3][3];
  int                 mcu_nvfrags;
  int                 loop_filter;
  int                 pp_level;
}oc_dec_pipeline_state;



/*Initialize the main decoding pipeline.*/
static void oc_dec_pipeline_init(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe){
  const ptrdiff_t *coded_fragis;
  const ptrdiff_t *uncoded_fragis;
  int              pli;
  int              qii;
  int              qti;
  /*If chroma is sub-sampled in the vertical direction, we have to decode two
     super block rows of Y' for each super block row of Cb and Cr.*/
  _pipe->mcu_nvfrags=4<<!(_dec->state.info.pixel_fmt&2);
  /*Initialize the token and extra bits indices for each plane and
     coefficient.*/
  memset(_pipe->ti[0],0,sizeof(_pipe->ti[0]));
  memset(_pipe->ebi[0],0,sizeof(_pipe->ebi[0]));
  for(pli=1;pli<3;pli++){
    memcpy(_pipe->ti[pli],_dec->ti0[pli-1],sizeof(_pipe->ti[0]));
    memcpy(_pipe->ebi[pli],_dec->ebi0[pli-1],sizeof(_pipe->ebi[0]));
  }
  /*Also copy over the initial the EOB run counts.*/
  memcpy(_pipe->eob_runs,_dec->eob_runs,sizeof(_pipe->eob_runs));
  /*Set up per-plane pointers to the coded and uncoded fragments lists.*/
  coded_fragis=_dec->state.coded_fragis;
  uncoded_fragis=coded_fragis+_dec->state.nfrags;
  for(pli=0;pli<3;pli++){
    ptrdiff_t ncoded_fragis;
    _pipe->coded_fragis[pli]=coded_fragis;
    _pipe->uncoded_fragis[pli]=uncoded_fragis;
    ncoded_fragis=_dec->state.ncoded_fragis[pli];
    coded_fragis+=ncoded_fragis;
    uncoded_fragis+=ncoded_fragis-_dec->state.fplanes[pli].nfrags;
  }
  /*Set up condensed quantizer tables.*/
  for(pli=0;pli<3;pli++){
    for(qii=0;qii<_dec->state.nqis;qii++){
      for(qti=0;qti<2;qti++){
        _pipe->dequant[pli][qii][qti]=
         _dec->state.dequant_tables[_dec->state.qis[qii]][pli][qti];
      }
    }
  }
  /*Set the previous DC predictor to 0 for all color planes and frame types.*/
  memset(_pipe->pred_last,0,sizeof(_pipe->pred_last));
  /*Initialize the bounding value array for the loop filter.*/
  _pipe->loop_filter=!oc_state_loop_filter_init(&_dec->state,
   _pipe->bounding_values);
  /*Initialize any buffers needed for post-processing.
    We also save the current post-processing level, to guard against the user
     changing it from a callback.*/
  if(!oc_dec_postprocess_init(_dec))_pipe->pp_level=_dec->pp_level;
  /*If we don't have enough information to post-process, disable it, regardless
     of the user-requested level.*/
  else{
    _pipe->pp_level=OC_PP_LEVEL_DISABLED;
    memcpy(_dec->pp_frame_buf,
     _dec->state.ref_frame_bufs[_dec->state.ref_frame_idx[OC_FRAME_SELF]],
     sizeof(_dec->pp_frame_buf[0])*3);
  }
}

/*Undo the DC prediction in a single plane of an MCU (one or two super block
   rows).
  As a side effect, the number of coded and uncoded fragments in this plane of
   the MCU is also computed.*/
static void oc_dec_dc_unpredict_mcu_plane(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli){
  const oc_fragment_plane *fplane;
  oc_fragment             *frags;
  int                     *pred_last;
  ptrdiff_t                ncoded_fragis;
  ptrdiff_t                fragi;
  int                      fragx;
  int                      fragy;
  int                      fragy0;
  int                      fragy_end;
  int                      nhfrags;
  /*Compute the first and last fragment row of the current MCU for this
     plane.*/
  fplane=_dec->state.fplanes+_pli;
  fragy0=_pipe->fragy0[_pli];
  fragy_end=_pipe->fragy_end[_pli];
  nhfrags=fplane->nhfrags;
  pred_last=_pipe->pred_last[_pli];
  frags=_dec->state.frags;
  ncoded_fragis=0;
  fragi=fplane->froffset+fragy0*(ptrdiff_t)nhfrags;
  for(fragy=fragy0;fragy<fragy_end;fragy++){
    for(fragx=0;fragx<nhfrags;fragx++,fragi++){
      if(!frags[fragi].coded)continue;
      pred_last[OC_FRAME_FOR_MODE[frags[fragi].mb_mode]]=frags[fragi].dc+=
       oc_frag_pred_dc(frags+fragi,fplane,fragx,fragy,pred_last);
      ncoded_fragis++;
    }
  }
  _pipe->ncoded_fragis[_pli]=ncoded_fragis;
  /*Also save the number of uncoded fragments so we know how many to copy.*/
  _pipe->nuncoded_fragis[_pli]=
   (fragy_end-fragy0)*(ptrdiff_t)nhfrags-ncoded_fragis;
}

/*Reconstructs all coded fragments in a single MCU (one or two super block
   rows).
  This requires that each coded fragment have a proper macro block mode and
   motion vector (if not in INTRA mode), and have it's DC value decoded, with
   the DC prediction process reversed, and the number of coded and uncoded
   fragments in this plane of the MCU be counted.
  The token lists for each color plane and coefficient should also be filled
   in, along with initial token offsets, extra bits offsets, and EOB run
   counts.*/
static void oc_dec_frags_recon_mcu_plane(oc_dec_ctx *_dec,
 oc_dec_pipeline_state *_pipe,int _pli){
  ogg_uint16_t       dc_quant[2];
  const oc_fragment *frags;
  const ptrdiff_t   *coded_fragis;
  ptrdiff_t          ncoded_fragis;
  ptrdiff_t          fragii;
  ptrdiff_t         *ti;
  ptrdiff_t         *ebi;
  ptrdiff_t         *eob_runs;
  int                qti;
  frags=_dec->state.frags;
  coded_fragis=_pipe->coded_fragis[_pli];
  ncoded_fragis=_pipe->ncoded_fragis[_pli];
  ti=_pipe->ti[_pli];
  ebi=_pipe->ebi[_pli];
  eob_runs=_pipe->eob_runs[_pli];
  for(qti=0;qti<2;qti++)dc_quant[qti]=_pipe->dequant[_pli][0][qti][0];
  for(fragii=0;fragii<ncoded_fragis;fragii++){
    /*This array is made twice as large as necessary so that an invalid zero
       run cannot cause a buffer overflow.*/
    ogg_int16_t dct_coeffs[128];
    ptrdiff_t   fragi;
    int         last_zzi;
    int         zzi;
    fragi=coded_fragis[fragii];
    /*Decode the AC coefficients.*/
    for(zzi=0;zzi<64;){
      int token;
      int eb;
      last_zzi=zzi;
      if(eob_runs[zzi]){
        eob_runs[zzi]--;
        break;
      }
      else{
        int ebflag;
        token=_dec->dct_tokens[zzi][ti[zzi]++];
        ebflag=OC_DCT_TOKEN_EXTRA_BITS[token]!=0;
        eb=_dec->extra_bits[zzi][ebi[zzi]]&-ebflag;
        ebi[zzi]+=ebflag;
        if(token<OC_NDCT_EOB_TOKEN_MAX){
          eob_runs[zzi]=-oc_dct_token_skip(token,eb);
        }
        else zzi=oc_dct_token_expand(token,eb,dct_coeffs,zzi);
      }
    }
    /*TODO: zzi should be exactly 64 here.
      If it's not, we should report some kind of warning.*/
    zzi=OC_MINI(zzi,64);
    dct_coeffs[0]=(ogg_int16_t)frags[fragi].dc;
    qti=frags[fragi].mb_mode!=OC_MODE_INTRA;
    /*last_zzi is always initialized.
      If your compiler thinks otherwise, it is dumb.*/
    oc_state_frag_recon(&_dec->state,fragi,_pli,dct_coeffs,last_zzi,zzi,
     dc_quant[qti],_pipe->dequant[_pli][frags[fragi].qii][qti]);
  }
  _pipe->coded_fragis[_pli]+=ncoded_fragis;
  /*Right now the reconstructed MCU has only the coded blocks in it.*/
  /*TODO: We make the decision here to always copy the uncoded blocks into it
     from the reference frame.
    We could also copy the coded blocks back over the reference frame, if we
     wait for an additional MCU to be decoded, which might be faster if only a
     small number of blocks are coded.
    However, this introduces more latency, creating a larger cache footprint.
    It's unknown which decision is better, but this one results in simpler
     code, and the hard case (high bitrate, high resolution) is handled
     correctly.*/
  /*Copy the uncoded blocks from the previous reference frame.*/
  _pipe->uncoded_fragis[_pli]-=_pipe->nuncoded_fragis[_pli];
  oc_state_frag_copy_list(&_dec->state,_pipe->uncoded_fragis[_pli],
   _pipe->nuncoded_fragis[_pli],OC_FRAME_SELF,OC_FRAME_PREV,_pli);
}

/*Filter a horizontal block edge.*/
static void oc_filter_hedge(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,int _qstep,int _flimit,
 int *_variance0,int *_variance1){
  unsigned char       *rdst;
  const unsigned char *rsrc;
  unsigned char       *cdst;
  const unsigned char *csrc;
  int                  r[10];
  int                  sum0;
  int                  sum1;
  int                  bx;
  int                  by;
  rdst=_dst;
  rsrc=_src;
  for(bx=0;bx<8;bx++){
    cdst=rdst;
    csrc=rsrc;
    for(by=0;by<10;by++){
      r[by]=*csrc;
      csrc+=_src_ystride;
    }
    sum0=sum1=0;
    for(by=0;by<4;by++){
      sum0+=abs(r[by+1]-r[by]);
      sum1+=abs(r[by+5]-r[by+6]);
    }
    *_variance0+=OC_MINI(255,sum0);
    *_variance1+=OC_MINI(255,sum1);
    if(sum0<_flimit&&sum1<_flimit&&r[5]-r[4]<_qstep&&r[4]-r[5]<_qstep){
      *cdst=(unsigned char)(r[0]*3+r[1]*2+r[2]+r[3]+r[4]+4>>3);
      cdst+=_dst_ystride;
      *cdst=(unsigned char)(r[0]*2+r[1]+r[2]*2+r[3]+r[4]+r[5]+4>>3);
      cdst+=_dst_ystride;
      for(by=0;by<4;by++){
        *cdst=(unsigned char)(r[by]+r[by+1]+r[by+2]+r[by+3]*2+
         r[by+4]+r[by+5]+r[by+6]+4>>3);
        cdst+=_dst_ystride;
      }
      *cdst=(unsigned char)(r[4]+r[5]+r[6]+r[7]*2+r[8]+r[9]*2+4>>3);
      cdst+=_dst_ystride;
      *cdst=(unsigned char)(r[5]+r[6]+r[7]+r[8]*2+r[9]*3+4>>3);
    }
    else{
      for(by=1;by<=8;by++){
        *cdst=(unsigned char)r[by];
        cdst+=_dst_ystride;
      }
    }
    rdst++;
    rsrc++;
  }
}

/*Filter a vertical block edge.*/
static void oc_filter_vedge(unsigned char *_dst,int _dst_ystride,
 int _qstep,int _flimit,int *_variances){
  unsigned char       *rdst;
  const unsigned char *rsrc;
  unsigned char       *cdst;
  int                  r[10];
  int                  sum0;
  int                  sum1;
  int                  bx;
  int                  by;
  cdst=_dst;
  for(by=0;by<8;by++){
    rsrc=cdst-1;
    rdst=cdst;
    for(bx=0;bx<10;bx++)r[bx]=*rsrc++;
    sum0=sum1=0;
    for(bx=0;bx<4;bx++){
      sum0+=abs(r[bx+1]-r[bx]);
      sum1+=abs(r[bx+5]-r[bx+6]);
    }
    _variances[0]+=OC_MINI(255,sum0);
    _variances[1]+=OC_MINI(255,sum1);
    if(sum0<_flimit&&sum1<_flimit&&r[5]-r[4]<_qstep&&r[4]-r[5]<_qstep){
      *rdst++=(unsigned char)(r[0]*3+r[1]*2+r[2]+r[3]+r[4]+4>>3);
      *rdst++=(unsigned char)(r[0]*2+r[1]+r[2]*2+r[3]+r[4]+r[5]+4>>3);
      for(bx=0;bx<4;bx++){
        *rdst++=(unsigned char)(r[bx]+r[bx+1]+r[bx+2]+r[bx+3]*2+
         r[bx+4]+r[bx+5]+r[bx+6]+4>>3);
      }
      *rdst++=(unsigned char)(r[4]+r[5]+r[6]+r[7]*2+r[8]+r[9]*2+4>>3);
      *rdst=(unsigned char)(r[5]+r[6]+r[7]+r[8]*2+r[9]*3+4>>3);
    }
    else for(bx=1;bx<=8;bx++)*rdst++=(unsigned char)r[bx];
    cdst+=_dst_ystride;
  }
}

static void oc_dec_deblock_frag_rows(oc_dec_ctx *_dec,
 th_img_plane *_dst,th_img_plane *_src,int _pli,int _fragy0,
 int _fragy_end){
  oc_fragment_plane   *fplane;
  int                 *variance;
  unsigned char       *dc_qi;
  unsigned char       *dst;
  const unsigned char *src;
  ptrdiff_t            froffset;
  int                  dst_ystride;
  int                  src_ystride;
  int                  nhfrags;
  int                  width;
  int                  notstart;
  int                  notdone;
  int                  flimit;
  int                  qstep;
  int                  y_end;
  int                  y;
  int                  x;
  _dst+=_pli;
  _src+=_pli;
  fplane=_dec->state.fplanes+_pli;
  nhfrags=fplane->nhfrags;
  froffset=fplane->froffset+_fragy0*(ptrdiff_t)nhfrags;
  variance=_dec->variances+froffset;
  dc_qi=_dec->dc_qis+froffset;
  notstart=_fragy0>0;
  notdone=_fragy_end<fplane->nvfrags;
  /*We want to clear an extra row of variances, except at the end.*/
  memset(variance+(nhfrags&-notstart),0,
   (_fragy_end+notdone-_fragy0-notstart)*(nhfrags*sizeof(variance[0])));
  /*Except for the first time, we want to point to the middle of the row.*/
  y=(_fragy0<<3)+(notstart<<2);
  dst_ystride=_dst->stride;
  src_ystride=_src->stride;
  dst=_dst->data+y*(ptrdiff_t)dst_ystride;
  src=_src->data+y*(ptrdiff_t)src_ystride;
  width=_dst->width;
  for(;y<4;y++){
    memcpy(dst,src,width*sizeof(dst[0]));
    dst+=dst_ystride;
    src+=src_ystride;
  }
  /*We also want to skip the last row in the frame for this loop.*/
  y_end=_fragy_end-!notdone<<3;
  for(;y<y_end;y+=8){
    qstep=_dec->pp_dc_scale[*dc_qi];
    flimit=(qstep*3)>>2;
    oc_filter_hedge(dst,dst_ystride,src-src_ystride,src_ystride,
     qstep,flimit,variance,variance+nhfrags);
    variance++;
    dc_qi++;
    for(x=8;x<width;x+=8){
      qstep=_dec->pp_dc_scale[*dc_qi];
      flimit=(qstep*3)>>2;
      oc_filter_hedge(dst+x,dst_ystride,src+x-src_ystride,src_ystride,
       qstep,flimit,variance,variance+nhfrags);
      oc_filter_vedge(dst+x-(dst_ystride<<2)-4,dst_ystride,
       qstep,flimit,variance-1);
      variance++;
      dc_qi++;
    }
    dst+=dst_ystride<<3;
    src+=src_ystride<<3;
  }
  /*And finally, handle the last row in the frame, if it's in the range.*/
  if(!notdone){
    int height;
    height=_dst->height;
    for(;y<height;y++){
      memcpy(dst,src,width*sizeof(dst[0]));
      dst+=dst_ystride;
      src+=src_ystride;
    }
    /*Filter the last row of vertical block edges.*/
    dc_qi++;
    for(x=8;x<width;x+=8){
      qstep=_dec->pp_dc_scale[*dc_qi++];
      flimit=(qstep*3)>>2;
      oc_filter_vedge(dst+x-(dst_ystride<<3)-4,dst_ystride,
       qstep,flimit,variance++);
    }
  }
}

static void oc_dering_block(unsigned char *_idata,int _ystride,int _b,
 int _dc_scale,int _sharp_mod,int _strong){
  static const unsigned char MOD_MAX[2]={24,32};
  static const unsigned char MOD_SHIFT[2]={1,0};
  const unsigned char *psrc;
  const unsigned char *src;
  const unsigned char *nsrc;
  unsigned char       *dst;
  int                  vmod[72];
  int                  hmod[72];
  int                  mod_hi;
  int                  by;
  int                  bx;
  mod_hi=OC_MINI(3*_dc_scale,MOD_MAX[_strong]);
  dst=_idata;
  src=dst;
  psrc=src-(_ystride&-!(_b&4));
  for(by=0;by<9;by++){
    for(bx=0;bx<8;bx++){
      int mod;
      mod=32+_dc_scale-(abs(src[bx]-psrc[bx])<<MOD_SHIFT[_strong]);
      vmod[(by<<3)+bx]=mod<-64?_sharp_mod:OC_CLAMPI(0,mod,mod_hi);
    }
    psrc=src;
    src+=_ystride&-(!(_b&8)|by<7);
  }
  nsrc=dst;
  psrc=dst-!(_b&1);
  for(bx=0;bx<9;bx++){
    src=nsrc;
    for(by=0;by<8;by++){
      int mod;
      mod=32+_dc_scale-(abs(*src-*psrc)<<MOD_SHIFT[_strong]);
      hmod[(bx<<3)+by]=mod<-64?_sharp_mod:OC_CLAMPI(0,mod,mod_hi);
      psrc+=_ystride;
      src+=_ystride;
    }
    psrc=nsrc;
    nsrc+=!(_b&2)|bx<7;
  }
  src=dst;
  psrc=src-(_ystride&-!(_b&4));
  nsrc=src+_ystride;
  for(by=0;by<8;by++){
    int a;
    int b;
    int w;
    a=128;
    b=64;
    w=hmod[by];
    a-=w;
    b+=w**(src-!(_b&1));
    w=vmod[by<<3];
    a-=w;
    b+=w*psrc[0];
    w=vmod[by+1<<3];
    a-=w;
    b+=w*nsrc[0];
    w=hmod[(1<<3)+by];
    a-=w;
    b+=w*src[1];
    dst[0]=OC_CLAMP255(a*src[0]+b>>7);
    for(bx=1;bx<7;bx++){
      a=128;
      b=64;
      w=hmod[(bx<<3)+by];
      a-=w;
      b+=w*src[bx-1];
      w=vmod[(by<<3)+bx];
      a-=w;
      b+=w*psrc[bx];
      w=vmod[(by+1<<3)+bx];
      a-=w;
      b+=w*nsrc[bx];
      w=hmod[(bx+1<<3)+by];
      a-=w;
      b+=w*src[bx+1];
      dst[bx]=OC_CLAMP255(a*src[bx]+b>>7);
    }
    a=128;
    b=64;
    w=hmod[(7<<3)+by];
    a-=w;
    b+=w*src[6];
    w=vmod[(by<<3)+7];
    a-=w;
    b+=w*psrc[7];
    w=vmod[(by+1<<3)+7];
    a-=w;
    b+=w*nsrc[7];
    w=hmod[(8<<3)+by];
    a-=w;
    b+=w*src[7+!(_b&2)];
    dst[7]=OC_CLAMP255(a*src[7]+b>>7);
    dst+=_ystride;
    psrc=src;
    src=nsrc;
    nsrc+=_ystride&-(!(_b&8)|by<6);
  }
}

#define OC_DERING_THRESH1 (384)
#define OC_DERING_THRESH2 (4*OC_DERING_THRESH1)
#define OC_DERING_THRESH3 (5*OC_DERING_THRESH1)
#define OC_DERING_THRESH4 (10*OC_DERING_THRESH1)

static void oc_dec_dering_frag_rows(oc_dec_ctx *_dec,th_img_plane *_img,
 int _pli,int _fragy0,int _fragy_end){
  th_img_plane      *iplane;
  oc_fragment_plane *fplane;
  oc_fragment       *frag;
  int               *variance;
  unsigned char     *idata;
  ptrdiff_t          froffset;
  int                ystride;
  int                nhfrags;
  int                sthresh;
  int                strong;
  int                y_end;
  int                width;
  int                height;
  int                y;
  int                x;
  iplane=_img+_pli;
  fplane=_dec->state.fplanes+_pli;
  nhfrags=fplane->nhfrags;
  froffset=fplane->froffset+_fragy0*(ptrdiff_t)nhfrags;
  variance=_dec->variances+froffset;
  frag=_dec->state.frags+froffset;
  strong=_dec->pp_level>=(_pli?OC_PP_LEVEL_SDERINGC:OC_PP_LEVEL_SDERINGY);
  sthresh=_pli?OC_DERING_THRESH4:OC_DERING_THRESH3;
  y=_fragy0<<3;
  ystride=iplane->stride;
  idata=iplane->data+y*(ptrdiff_t)ystride;
  y_end=_fragy_end<<3;
  width=iplane->width;
  height=iplane->height;
  for(;y<y_end;y+=8){
    for(x=0;x<width;x+=8){
      int b;
      int qi;
      int var;
      qi=_dec->state.qis[frag->qii];
      var=*variance;
      b=(x<=0)|(x+8>=width)<<1|(y<=0)<<2|(y+8>=height)<<3;
      if(strong&&var>sthresh){
        oc_dering_block(idata+x,ystride,b,
         _dec->pp_dc_scale[qi],_dec->pp_sharp_mod[qi],1);
        if(_pli||!(b&1)&&*(variance-1)>OC_DERING_THRESH4||
         !(b&2)&&variance[1]>OC_DERING_THRESH4||
         !(b&4)&&*(variance-nhfrags)>OC_DERING_THRESH4||
         !(b&8)&&variance[nhfrags]>OC_DERING_THRESH4){
          oc_dering_block(idata+x,ystride,b,
           _dec->pp_dc_scale[qi],_dec->pp_sharp_mod[qi],1);
          oc_dering_block(idata+x,ystride,b,
           _dec->pp_dc_scale[qi],_dec->pp_sharp_mod[qi],1);
        }
      }
      else if(var>OC_DERING_THRESH2){
        oc_dering_block(idata+x,ystride,b,
         _dec->pp_dc_scale[qi],_dec->pp_sharp_mod[qi],1);
      }
      else if(var>OC_DERING_THRESH1){
        oc_dering_block(idata+x,ystride,b,
         _dec->pp_dc_scale[qi],_dec->pp_sharp_mod[qi],0);
      }
      frag++;
      variance++;
    }
    idata+=ystride<<3;
  }
}



th_dec_ctx *th_decode_alloc(const th_info *_info,const th_setup_info *_setup){
  oc_dec_ctx *dec;
  if(_info==NULL||_setup==NULL)return NULL;
  dec=_ogg_malloc(sizeof(*dec));
  if(oc_dec_init(dec,_info,_setup)<0){
    _ogg_free(dec);
    return NULL;
  }
  dec->state.curframe_num=0;
  return dec;
}

void th_decode_free(th_dec_ctx *_dec){
  if(_dec!=NULL){
    oc_dec_clear(_dec);
    _ogg_free(_dec);
  }
}

int th_decode_ctl(th_dec_ctx *_dec,int _req,void *_buf,
 size_t _buf_sz){
  switch(_req){
  case TH_DECCTL_GET_PPLEVEL_MAX:{
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(int))return TH_EINVAL;
    (*(int *)_buf)=OC_PP_LEVEL_MAX;
    return 0;
  }break;
  case TH_DECCTL_SET_PPLEVEL:{
    int pp_level;
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(int))return TH_EINVAL;
    pp_level=*(int *)_buf;
    if(pp_level<0||pp_level>OC_PP_LEVEL_MAX)return TH_EINVAL;
    _dec->pp_level=pp_level;
    return 0;
  }break;
  case TH_DECCTL_SET_GRANPOS:{
    ogg_int64_t granpos;
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(ogg_int64_t))return TH_EINVAL;
    granpos=*(ogg_int64_t *)_buf;
    if(granpos<0)return TH_EINVAL;
    _dec->state.granpos=granpos;
    _dec->state.keyframe_num=(granpos>>_dec->state.info.keyframe_granule_shift)
     -_dec->state.granpos_bias;
    _dec->state.curframe_num=_dec->state.keyframe_num
     +(granpos&(1<<_dec->state.info.keyframe_granule_shift)-1);
    return 0;
  }break;
  case TH_DECCTL_SET_STRIPE_CB:{
    th_stripe_callback *cb;
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(th_stripe_callback))return TH_EINVAL;
    cb=(th_stripe_callback *)_buf;
    _dec->stripe_cb.ctx=cb->ctx;
    _dec->stripe_cb.stripe_decoded=cb->stripe_decoded;
    return 0;
  }break;
#ifdef HAVE_CAIRO
  case TH_DECCTL_SET_TELEMETRY_MBMODE:{
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(int))return TH_EINVAL;
    _dec->telemetry=1;
    _dec->telemetry_mbmode=*(int *)_buf;
    return 0;
  }break;
  case TH_DECCTL_SET_TELEMETRY_MV:{
    if(_dec==NULL||_buf==NULL)return TH_EFAULT;
    if(_buf_sz!=sizeof(int))return TH_EINVAL;
    _dec->telemetry=1;
    _dec->telemetry_mv=*(int *)_buf;
    return 0;
  }break;
#endif
  default:return TH_EIMPL;
  }
}

int th_decode_packetin(th_dec_ctx *_dec,const ogg_packet *_op,
 ogg_int64_t *_granpos){
  int ret;
  if(_dec==NULL||_op==NULL)return TH_EFAULT;
  /*A completely empty packet indicates a dropped frame and is treated exactly
     like an inter frame with no coded blocks.
    Only proceed if we have a non-empty packet.*/
  if(_op->bytes!=0){
    oc_dec_pipeline_state pipe;
    th_ycbcr_buffer       stripe_buf;
    int                   stripe_fragy;
    int                   refi;
    int                   pli;
    int                   notstart;
    int                   notdone;
    theorapackB_readinit(&_dec->opb,_op->packet,_op->bytes);
    ret=oc_dec_frame_header_unpack(_dec);
    if(ret<0)return ret;
    /*Select a free buffer to use for the reconstructed version of this
       frame.*/
    if(_dec->state.frame_type!=OC_INTRA_FRAME&&
     (_dec->state.ref_frame_idx[OC_FRAME_GOLD]<0||
     _dec->state.ref_frame_idx[OC_FRAME_PREV]<0)){
      th_info *info;
      size_t       yplane_sz;
      size_t       cplane_sz;
      int          yhstride;
      int          yheight;
      int          chstride;
      int          cheight;
      /*We're decoding an INTER frame, but have no initialized reference
         buffers (i.e., decoding did not start on a key frame).
        We initialize them to a solid gray here.*/
      _dec->state.ref_frame_idx[OC_FRAME_GOLD]=0;
      _dec->state.ref_frame_idx[OC_FRAME_PREV]=0;
      _dec->state.ref_frame_idx[OC_FRAME_SELF]=refi=1;
      info=&_dec->state.info;
      yhstride=info->frame_width+2*OC_UMV_PADDING;
      yheight=info->frame_height+2*OC_UMV_PADDING;
      chstride=yhstride>>!(info->pixel_fmt&1);
      cheight=yheight>>!(info->pixel_fmt&2);
      yplane_sz=yhstride*(size_t)yheight;
      cplane_sz=chstride*(size_t)cheight;
      memset(_dec->state.ref_frame_data[0],0x80,yplane_sz+2*cplane_sz);
    }
    else{
      for(refi=0;refi==_dec->state.ref_frame_idx[OC_FRAME_GOLD]||
       refi==_dec->state.ref_frame_idx[OC_FRAME_PREV];refi++);
      _dec->state.ref_frame_idx[OC_FRAME_SELF]=refi;
    }
    if(_dec->state.frame_type==OC_INTRA_FRAME){
      oc_dec_mark_all_intra(_dec);
      _dec->state.keyframe_num=_dec->state.curframe_num;
    }else{
      oc_dec_coded_flags_unpack(_dec);
      oc_dec_mb_modes_unpack(_dec);
      oc_dec_mv_unpack_and_frag_modes_fill(_dec);
    }
    oc_dec_block_qis_unpack(_dec);
    oc_dec_residual_tokens_unpack(_dec);
    /*Update granule position.
      This must be done before the striped decode callbacks so that the
       application knows what to do with the frame data.*/
    _dec->state.granpos=(_dec->state.keyframe_num+_dec->state.granpos_bias<<
     _dec->state.info.keyframe_granule_shift)
     +(_dec->state.curframe_num-_dec->state.keyframe_num);
    _dec->state.curframe_num++;
    if(_granpos!=NULL)*_granpos=_dec->state.granpos;
    /*All of the rest of the operations -- DC prediction reversal,
       reconstructing coded fragments, copying uncoded fragments, loop
       filtering, extending borders, and out-of-loop post-processing -- should
       be pipelined.
      I.e., DC prediction reversal, reconstruction, and uncoded fragment
       copying are done for one or two super block rows, then loop filtering is
       run as far as it can, then bordering copying, then post-processing.
      For 4:2:0 video a Minimum Codable Unit or MCU contains two luma super
       block rows, and one chroma.
      Otherwise, an MCU consists of one super block row from each plane.
      Inside each MCU, we perform all of the steps on one color plane before
       moving on to the next.
      After reconstruction, the additional filtering stages introduce a delay
       since they need some pixels from the next fragment row.
      Thus the actual number of decoded rows available is slightly smaller for
       the first MCU, and slightly larger for the last.

      This entire process allows us to operate on the data while it is still in
       cache, resulting in big performance improvements.
      An application callback allows further application processing (blitting
       to video memory, color conversion, etc.) to also use the data while it's
       in cache.*/
    oc_dec_pipeline_init(_dec,&pipe);
    oc_ycbcr_buffer_flip(stripe_buf,_dec->pp_frame_buf);
    notstart=0;
    notdone=1;
    for(stripe_fragy=0;notdone;stripe_fragy+=pipe.mcu_nvfrags){
      int avail_fragy0;
      int avail_fragy_end;
      avail_fragy0=avail_fragy_end=_dec->state.fplanes[0].nvfrags;
      notdone=stripe_fragy+pipe.mcu_nvfrags<avail_fragy_end;
      for(pli=0;pli<3;pli++){
        oc_fragment_plane *fplane;
        int                frag_shift;
        int                pp_offset;
        int                sdelay;
        int                edelay;
        fplane=_dec->state.fplanes+pli;
        /*Compute the first and last fragment row of the current MCU for this
           plane.*/
        frag_shift=pli!=0&&!(_dec->state.info.pixel_fmt&2);
        pipe.fragy0[pli]=stripe_fragy>>frag_shift;
        pipe.fragy_end[pli]=OC_MINI(fplane->nvfrags,
         pipe.fragy0[pli]+(pipe.mcu_nvfrags>>frag_shift));
        oc_dec_dc_unpredict_mcu_plane(_dec,&pipe,pli);
        oc_dec_frags_recon_mcu_plane(_dec,&pipe,pli);
        sdelay=edelay=0;
        if(pipe.loop_filter){
          sdelay+=notstart;
          edelay+=notdone;
          oc_state_loop_filter_frag_rows(&_dec->state,pipe.bounding_values,
           refi,pli,pipe.fragy0[pli]-sdelay,pipe.fragy_end[pli]-edelay);
        }
        /*To fill the borders, we have an additional two pixel delay, since a
           fragment in the next row could filter its top edge, using two pixels
           from a fragment in this row.
          But there's no reason to delay a full fragment between the two.*/
        oc_state_borders_fill_rows(&_dec->state,refi,pli,
         (pipe.fragy0[pli]-sdelay<<3)-(sdelay<<1),
         (pipe.fragy_end[pli]-edelay<<3)-(edelay<<1));
        /*Out-of-loop post-processing.*/
        pp_offset=3*(pli!=0);
        if(pipe.pp_level>=OC_PP_LEVEL_DEBLOCKY+pp_offset){
          /*Perform de-blocking in one plane.*/
          sdelay+=notstart;
          edelay+=notdone;
          oc_dec_deblock_frag_rows(_dec,_dec->pp_frame_buf,
           _dec->state.ref_frame_bufs[refi],pli,
           pipe.fragy0[pli]-sdelay,pipe.fragy_end[pli]-edelay);
          if(pipe.pp_level>=OC_PP_LEVEL_DERINGY+pp_offset){
            /*Perform de-ringing in one plane.*/
            sdelay+=notstart;
            edelay+=notdone;
            oc_dec_dering_frag_rows(_dec,_dec->pp_frame_buf,pli,
             pipe.fragy0[pli]-sdelay,pipe.fragy_end[pli]-edelay);
          }
        }
        /*If no post-processing is done, we still need to delay a row for the
           loop filter, thanks to the strange filtering order VP3 chose.*/
        else if(pipe.loop_filter){
          sdelay+=notstart;
          edelay+=notdone;
        }
        /*Compute the intersection of the available rows in all planes.
          If chroma is sub-sampled, the effect of each of its delays is
           doubled, but luma might have more post-processing filters enabled
           than chroma, so we don't know up front which one is the limiting
           factor.*/
        avail_fragy0=OC_MINI(avail_fragy0,pipe.fragy0[pli]-sdelay<<frag_shift);
        avail_fragy_end=OC_MINI(avail_fragy_end,
         pipe.fragy_end[pli]-edelay<<frag_shift);
      }
      if(_dec->stripe_cb.stripe_decoded!=NULL){
        /*The callback might want to use the FPU, so let's make sure they can.
          We violate all kinds of ABI restrictions by not doing this until
           now, but none of them actually matter since we don't use floating
           point ourselves.*/
        oc_restore_fpu(&_dec->state);
        /*Make the callback, ensuring we flip the sense of the "start" and
           "end" of the available region upside down.*/
        (*_dec->stripe_cb.stripe_decoded)(_dec->stripe_cb.ctx,stripe_buf,
         _dec->state.fplanes[0].nvfrags-avail_fragy_end,
         _dec->state.fplanes[0].nvfrags-avail_fragy0);
      }
      notstart=1;
    }
    /*Finish filling in the reference frame borders.*/
    for(pli=0;pli<3;pli++)oc_state_borders_fill_caps(&_dec->state,refi,pli);
    /*Update the reference frame indices.*/
    if(_dec->state.frame_type==OC_INTRA_FRAME){
      /*The new frame becomes both the previous and gold reference frames.*/
      _dec->state.ref_frame_idx[OC_FRAME_GOLD]=
       _dec->state.ref_frame_idx[OC_FRAME_PREV]=
       _dec->state.ref_frame_idx[OC_FRAME_SELF];
    }
    else{
      /*Otherwise, just replace the previous reference frame.*/
      _dec->state.ref_frame_idx[OC_FRAME_PREV]=
       _dec->state.ref_frame_idx[OC_FRAME_SELF];
    }
    /*Restore the FPU before dump_frame, since that _does_ use the FPU (for PNG
       gamma values, if nothing else).*/
    oc_restore_fpu(&_dec->state);
#if defined(OC_DUMP_IMAGES)
    /*Don't dump images for dropped frames.*/
    oc_state_dump_frame(&_dec->state,OC_FRAME_SELF,"dec");
#endif
    return 0;
  }
  else{
    /*Just update the granule position and return.*/
    _dec->state.granpos=(_dec->state.keyframe_num+_dec->state.granpos_bias<<
     _dec->state.info.keyframe_granule_shift)
     +(_dec->state.curframe_num-_dec->state.keyframe_num);
    _dec->state.curframe_num++;
    if(_granpos!=NULL)*_granpos=_dec->state.granpos;
    return TH_DUPFRAME;
  }
}

int th_decode_ycbcr_out(th_dec_ctx *_dec,th_ycbcr_buffer _ycbcr){
  oc_ycbcr_buffer_flip(_ycbcr,_dec->pp_frame_buf);
#if defined(HAVE_CAIRO)
  /*If telemetry ioctls are active, we need to draw to the output buffer.
    Stuff the plane into cairo.*/
  if(_dec->telemetry){
    cairo_surface_t *cs;
    unsigned char   *data;
    unsigned char   *y_row;
    unsigned char   *u_row;
    unsigned char   *v_row;
    unsigned char   *rgb_row;
    int              cstride;
    int              w;
    int              h;
    int              x;
    int              y;
    int              hdec;
    int              vdec;
    w=_ycbcr[0].width;
    h=_ycbcr[0].height;
    hdec=!(_dec->state.info.pixel_fmt&1);
    vdec=!(_dec->state.info.pixel_fmt&2);
    /*Lazy data buffer init.
      We could try to re-use the post-processing buffer, which would save
       memory, but complicate the allocation logic there.
      I don't think anyone cares about memory usage when using telemetry; it is
       not meant for embedded devices.*/
    if(!_dec->telemetry_frame_data){
      _dec->telemetry_frame_data=_ogg_malloc(
       (w*h+2*(w>>hdec)*(h>>vdec))*sizeof(*_dec->telemetry_frame_data));
    }
    cs=cairo_image_surface_create(CAIRO_FORMAT_RGB24,w,h);
    /*Sadly, no YUV support in Cairo (yet); convert into the RGB buffer.*/
    data=cairo_image_surface_get_data(cs);
    cstride=cairo_image_surface_get_stride(cs);
    y_row=_ycbcr[0].data;
    u_row=_ycbcr[1].data;
    v_row=_ycbcr[2].data;
    rgb_row=data;
    for(y=0;y<h;y++){
      for(x=0;x<w;x++){
        int r;
        int g;
        int b;
        r=(1904000*y_row[x]+2609823*v_row[x>>hdec]-363703744)/1635200;
        g=(3827562*y_row[x]-1287801*u_row[x>>hdec]
         -2672387*v_row[x>>hdec]+447306710)/3287200;
        b=(952000*y_row[x]+1649289*u_row[x>>hdec]-225932192)/817600;
        rgb_row[4*x+0]=OC_CLAMP255(b);
        rgb_row[4*x+1]=OC_CLAMP255(g);
        rgb_row[4*x+2]=OC_CLAMP255(r);
      }
      y_row+=_ycbcr[0].stride;
      u_row+=_ycbcr[1].stride&-((y&1)|!vdec);
      v_row+=_ycbcr[2].stride&-((y&1)|!vdec);
      rgb_row+=cstride;
    }
    /*Draw coded identifier for each macroblock (stored in Hilbert order).*/
    {
      cairo_t           *c;
      const oc_fragment *frags;
      oc_mv             *frag_mvs;
      const signed char *mb_modes;
      oc_mb_map         *mb_maps;
      size_t             nmbs;
      size_t             mbi;
      int                row2;
      int                col2;
      c=cairo_create(cs);
      frags=_dec->state.frags;
      frag_mvs=_dec->state.frag_mvs;
      mb_modes=_dec->state.mb_modes;
      mb_maps=_dec->state.mb_maps;
      nmbs=_dec->state.nmbs;
      row2=0;
      col2=0;
      for(mbi=0;mbi<nmbs;mbi++){
        float x;
        float y;
        int   bi;
        y=h-(row2+((col2+1>>1)&1))*16-16;
        x=(col2>>1)*16;
        cairo_set_line_width(c,1.);
        if(_dec->state.frame_type==OC_INTRA_FRAME){
          if(_dec->telemetry_mbmode&0x02){
            cairo_set_source_rgba(c,1.,0,0,.5);
            cairo_rectangle(c,x+2.5,y+2.5,11,11);
            cairo_stroke_preserve(c);
            cairo_set_source_rgba(c,1.,0,0,.25);
            cairo_fill(c);
          }
        }
        else{
          const signed char *frag_mv;
          ptrdiff_t          fragi;
          for(bi=0;bi<4;bi++){
            fragi=mb_maps[mbi][0][bi];
            if(fragi>=0&&frags[fragi].coded){
              frag_mv=frag_mvs[fragi];
              break;
            }
          }
          if(bi<4){
            switch(mb_modes[mbi]){
              case OC_MODE_INTRA:{
                if(_dec->telemetry_mbmode&0x02){
                  cairo_set_source_rgba(c,1.,0,0,.5);
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_stroke_preserve(c);
                  cairo_set_source_rgba(c,1.,0,0,.25);
                  cairo_fill(c);
                }
              }break;
              case OC_MODE_INTER_NOMV:{
                if(_dec->telemetry_mbmode&0x01){
                  cairo_set_source_rgba(c,0,0,1.,.5);
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_stroke_preserve(c);
                  cairo_set_source_rgba(c,0,0,1.,.25);
                  cairo_fill(c);
                }
              }break;
              case OC_MODE_INTER_MV:{
                if(_dec->telemetry_mbmode&0x04){
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_set_source_rgba(c,0,1.,0,.5);
                  cairo_stroke(c);
                }
                if(_dec->telemetry_mv&0x04){
                  cairo_move_to(c,x+8+frag_mv[0],y+8-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+8+frag_mv[0]*.66,y+8-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+8+frag_mv[0]*.33,y+8-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+8,y+8);
                  cairo_stroke(c);
                }
              }break;
              case OC_MODE_INTER_MV_LAST:{
                if(_dec->telemetry_mbmode&0x08){
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_set_source_rgba(c,0,1.,0,.5);
                  cairo_move_to(c,x+13.5,y+2.5);
                  cairo_line_to(c,x+2.5,y+8);
                  cairo_line_to(c,x+13.5,y+13.5);
                  cairo_stroke(c);
                }
                if(_dec->telemetry_mv&0x08){
                  cairo_move_to(c,x+8+frag_mv[0],y+8-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+8+frag_mv[0]*.66,y+8-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+8+frag_mv[0]*.33,y+8-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+8,y+8);
                  cairo_stroke(c);
                }
              }break;
              case OC_MODE_INTER_MV_LAST2:{
                if(_dec->telemetry_mbmode&0x10){
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_set_source_rgba(c,0,1.,0,.5);
                  cairo_move_to(c,x+8,y+2.5);
                  cairo_line_to(c,x+2.5,y+8);
                  cairo_line_to(c,x+8,y+13.5);
                  cairo_move_to(c,x+13.5,y+2.5);
                  cairo_line_to(c,x+8,y+8);
                  cairo_line_to(c,x+13.5,y+13.5);
                  cairo_stroke(c);
                }
                if(_dec->telemetry_mv&0x10){
                  cairo_move_to(c,x+8+frag_mv[0],y+8-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+8+frag_mv[0]*.66,y+8-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+8+frag_mv[0]*.33,y+8-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+8,y+8);
                  cairo_stroke(c);
                }
              }break;
              case OC_MODE_GOLDEN_NOMV:{
                if(_dec->telemetry_mbmode&0x20){
                  cairo_set_source_rgba(c,1.,1.,0,.5);
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_stroke_preserve(c);
                  cairo_set_source_rgba(c,1.,1.,0,.25);
                  cairo_fill(c);
                }
              }break;
              case OC_MODE_GOLDEN_MV:{
                if(_dec->telemetry_mbmode&0x40){
                  cairo_rectangle(c,x+2.5,y+2.5,11,11);
                  cairo_set_source_rgba(c,1.,1.,0,.5);
                  cairo_stroke(c);
                }
                if(_dec->telemetry_mv&0x40){
                  cairo_move_to(c,x+8+frag_mv[0],y+8-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+8+frag_mv[0]*.66,y+8-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+8+frag_mv[0]*.33,y+8-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+8,y+8);
                  cairo_stroke(c);
                }
              }break;
              case OC_MODE_INTER_MV_FOUR:{
                if(_dec->telemetry_mbmode&0x80){
                  cairo_rectangle(c,x+2.5,y+2.5,4,4);
                  cairo_rectangle(c,x+9.5,y+2.5,4,4);
                  cairo_rectangle(c,x+2.5,y+9.5,4,4);
                  cairo_rectangle(c,x+9.5,y+9.5,4,4);
                  cairo_set_source_rgba(c,0,1.,0,.5);
                  cairo_stroke(c);
                }
                /*4mv is odd, coded in raster order.*/
                fragi=mb_maps[mbi][0][0];
                if(frags[fragi].coded&&_dec->telemetry_mv&0x80){
                  frag_mv=frag_mvs[fragi];
                  cairo_move_to(c,x+4+frag_mv[0],y+12-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+4+frag_mv[0]*.66,y+12-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+4+frag_mv[0]*.33,y+12-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+4,y+12);
                  cairo_stroke(c);
                }
                fragi=mb_maps[mbi][0][1];
                if(frags[fragi].coded&&_dec->telemetry_mv&0x80){
                  frag_mv=frag_mvs[fragi];
                  cairo_move_to(c,x+12+frag_mv[0],y+12-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+12+frag_mv[0]*.66,y+12-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+12+frag_mv[0]*.33,y+12-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+12,y+12);
                  cairo_stroke(c);
                }
                fragi=mb_maps[mbi][0][2];
                if(frags[fragi].coded&&_dec->telemetry_mv&0x80){
                  frag_mv=frag_mvs[fragi];
                  cairo_move_to(c,x+4+frag_mv[0],y+4-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+4+frag_mv[0]*.66,y+4-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+4+frag_mv[0]*.33,y+4-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+4,y+4);
                  cairo_stroke(c);
                }
                fragi=mb_maps[mbi][0][3];
                if(frags[fragi].coded&&_dec->telemetry_mv&0x80){
                  frag_mv=frag_mvs[fragi];
                  cairo_move_to(c,x+12+frag_mv[0],y+4-frag_mv[1]);
                  cairo_set_source_rgba(c,1.,1.,1.,.9);
                  cairo_set_line_width(c,3.);
                  cairo_line_to(c,x+12+frag_mv[0]*.66,y+4-frag_mv[1]*.66);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,2.);
                  cairo_line_to(c,x+12+frag_mv[0]*.33,y+4-frag_mv[1]*.33);
                  cairo_stroke_preserve(c);
                  cairo_set_line_width(c,1.);
                  cairo_line_to(c,x+12,y+4);
                  cairo_stroke(c);
                }
              }break;
            }
          }
        }
        col2++;
        if((col2>>1)>=_dec->state.nhmbs){
          col2=0;
          row2+=2;
        }
      }
      cairo_destroy(c);
    }
    /*Out of the Cairo plane into the telemetry YUV buffer.*/
    _ycbcr[0].data=_dec->telemetry_frame_data;
    _ycbcr[0].stride=_ycbcr[0].width;
    _ycbcr[1].data=_ycbcr[0].data+h*_ycbcr[0].stride;
    _ycbcr[1].stride=_ycbcr[1].width;
    _ycbcr[2].data=_ycbcr[1].data+(h>>vdec)*_ycbcr[1].stride;
    _ycbcr[2].stride=_ycbcr[2].width;
    y_row=_ycbcr[0].data;
    u_row=_ycbcr[1].data;
    v_row=_ycbcr[2].data;
    rgb_row=data;
    /*This is one of the few places it's worth handling chroma on a
       case-by-case basis.*/
    switch(_dec->state.info.pixel_fmt){
      case TH_PF_420:{
        for(y=0;y<h;y+=2){
          unsigned char *y_row2;
          unsigned char *rgb_row2;
          y_row2=y_row+_ycbcr[0].stride;
          rgb_row2=rgb_row+cstride;
          for(x=0;x<w;x+=2){
            int y;
            int u;
            int v;
            y=(65481*rgb_row[4*x+2]+128553*rgb_row[4*x+1]
             +24966*rgb_row[4*x+0]+4207500)/255000;
            y_row[x]=OC_CLAMP255(y);
            y=(65481*rgb_row[4*x+6]+128553*rgb_row[4*x+5]
             +24966*rgb_row[4*x+4]+4207500)/255000;
            y_row[x+1]=OC_CLAMP255(y);
            y=(65481*rgb_row2[4*x+2]+128553*rgb_row2[4*x+1]
             +24966*rgb_row2[4*x+0]+4207500)/255000;
            y_row2[x]=OC_CLAMP255(y);
            y=(65481*rgb_row2[4*x+6]+128553*rgb_row2[4*x+5]
             +24966*rgb_row2[4*x+4]+4207500)/255000;
            y_row2[x+1]=OC_CLAMP255(y);
            u=(-8372*(rgb_row[4*x+2]+rgb_row[4*x+6]
             +rgb_row2[4*x+2]+rgb_row2[4*x+6])
             -16436*(rgb_row[4*x+1]+rgb_row[4*x+5]
             +rgb_row2[4*x+1]+rgb_row2[4*x+5])
             +24808*(rgb_row[4*x+0]+rgb_row[4*x+4]
             +rgb_row2[4*x+0]+rgb_row2[4*x+4])+29032005)/225930;
            v=(39256*(rgb_row[4*x+2]+rgb_row[4*x+6]
             +rgb_row2[4*x+2]+rgb_row2[4*x+6])
             -32872*(rgb_row[4*x+1]+rgb_row[4*x+5]
              +rgb_row2[4*x+1]+rgb_row2[4*x+5])
             -6384*(rgb_row[4*x+0]+rgb_row[4*x+4]
              +rgb_row2[4*x+0]+rgb_row2[4*x+4])+45940035)/357510;
            u_row[x>>1]=OC_CLAMP255(u);
            v_row[x>>1]=OC_CLAMP255(v);
          }
          y_row+=_ycbcr[0].stride<<1;
          u_row+=_ycbcr[1].stride;
          v_row+=_ycbcr[2].stride;
          rgb_row+=cstride<<1;
        }
      }break;
      case TH_PF_422:{
        for(y=0;y<h;y++){
          for(x=0;x<w;x+=2){
            int y;
            int u;
            int v;
            y=(65481*rgb_row[4*x+2]+128553*rgb_row[4*x+1]
             +24966*rgb_row[4*x+0]+4207500)/255000;
            y_row[x]=OC_CLAMP255(y);
            y=(65481*rgb_row[4*x+6]+128553*rgb_row[4*x+5]
             +24966*rgb_row[4*x+4]+4207500)/255000;
            y_row[x+1]=OC_CLAMP255(y);
            u=(-16744*(rgb_row[4*x+2]+rgb_row[4*x+6])
             -32872*(rgb_row[4*x+1]+rgb_row[4*x+5])
             +49616*(rgb_row[4*x+0]+rgb_row[4*x+4])+29032005)/225930;
            v=(78512*(rgb_row[4*x+2]+rgb_row[4*x+6])
             -65744*(rgb_row[4*x+1]+rgb_row[4*x+5])
             -12768*(rgb_row[4*x+0]+rgb_row[4*x+4])+45940035)/357510;
            u_row[x>>1]=OC_CLAMP255(u);
            v_row[x>>1]=OC_CLAMP255(v);
          }
          y_row+=_ycbcr[0].stride;
          u_row+=_ycbcr[1].stride;
          v_row+=_ycbcr[2].stride;
          rgb_row+=cstride;
        }
      }break;
      /*case TH_PF_444:*/
      default:{
        for(y=0;y<h;y++){
          for(x=0;x<w;x++){
            int y;
            int u;
            int v;
            y=(65481*rgb_row[4*x+2]+128553*rgb_row[4*x+1]
             +24966*rgb_row[4*x+0]+4207500)/255000;
            u=(-33488*rgb_row[4*x+2]-65744*rgb_row[4*x+1]
             +99232*rgb_row[4*x+0]+29032005)/225930;
            v=(157024*rgb_row[4*x+2]-131488*rgb_row[4*x+1]
             -25536*rgb_row[4*x+0]+45940035)/357510;
            y_row[x]=OC_CLAMP255(y);
            u_row[x]=OC_CLAMP255(u);
            v_row[x]=OC_CLAMP255(v);
          }
          y_row+=_ycbcr[0].stride;
          u_row+=_ycbcr[1].stride;
          v_row+=_ycbcr[2].stride;
          rgb_row+=cstride;
        }
      }break;
    }
    /*Finished.
      Destroy the surface.*/
    cairo_surface_destroy(cs);
  }
#endif
  return 0;
}