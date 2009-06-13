/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2008                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id$

 ********************************************************************/
#if !defined(_encint_H)
# define _encint_H (1)
# if defined(HAVE_CONFIG_H)
#  include "config.h"
# endif
# include "theora/theoraenc.h"
# include "../internal.h"
# include "../dec/ocintrin.h"
# include "mathops.h"
# include "enquant.h"
# include "huffenc.h"
/*# define OC_COLLECT_METRICS*/



typedef struct oc_enc_opt_vtable      oc_enc_opt_vtable;
typedef struct oc_mcenc_ctx           oc_mcenc_ctx;
typedef struct oc_mb_enc_info         oc_mb_enc_info;
typedef struct oc_mode_scheme_chooser oc_mode_scheme_chooser;
typedef struct oc_rc_state            oc_rc_state;
typedef struct th_enc_ctx             oc_enc_ctx;
typedef struct oc_token_checkpoint    oc_token_checkpoint;



/*Constants for the packet-out state machine specific to the encoder.*/

/*Next packet to emit: Data packet, but none are ready yet.*/
#define OC_PACKET_EMPTY (0)
/*Next packet to emit: Data packet, and one is ready.*/
#define OC_PACKET_READY (1)



/*The bits used for each of the MB mode codebooks.*/
extern const unsigned char OC_MODE_BITS[2][OC_NMODES];

/*The bits used for each of the MV codebooks.*/
extern const unsigned char OC_MV_BITS[2][64];

/*The minimum value that can be stored in a SB run for each codeword.
  The last entry is the upper bound on the length of a single SB run.*/
extern const ogg_uint16_t  OC_SB_RUN_VAL_MIN[8];
/*The bits used for each SB run codeword.*/
extern const unsigned char OC_SB_RUN_CODE_NBITS[7];

/*The bits used for each block run length (starting with 1).*/
extern const unsigned char OC_BLOCK_RUN_CODE_NBITS[30];



/*Encoder specific functions with accelerated variants.*/
struct oc_enc_opt_vtable{
  unsigned (*frag_sad)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  unsigned (*frag_sad_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_sad2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_satd_thresh)(const unsigned char *_src,
   const unsigned char *_ref,int _ystride,unsigned _thresh);
  unsigned (*frag_satd2_thresh)(const unsigned char *_src,
   const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
   unsigned _thresh);
  unsigned (*frag_intra_satd)(const unsigned char *_src,int _ystride);
  void     (*frag_sub)(ogg_int16_t _diff[64],const unsigned char *_src,
   const unsigned char *_ref,int _ystride);
  void     (*frag_sub_128)(ogg_int16_t _diff[64],
   const unsigned char *_src,int _ystride);
  void     (*frag_copy2)(unsigned char *_dst,
   const unsigned char *_src1,const unsigned char *_src2,int _ystride);
  void     (*frag_recon_intra)(unsigned char *_dst,int _ystride,
   const ogg_int16_t _residue[64]);
  void     (*frag_recon_inter)(unsigned char *_dst,
   const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
  void     (*fdct8x8)(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
};


void oc_enc_vtable_init(oc_enc_ctx *_enc);



/*Encoder-specific macroblock information.*/
struct oc_mb_enc_info{
  /*Neighboring macro blocks that have MVs available from the current frame.*/
  unsigned      cneighbors[4];
  /*Neighboring macro blocks to use for MVs from the previous frame.*/
  unsigned      pneighbors[4];
  /*The number of current-frame neighbors.*/
  unsigned char ncneighbors;
  /*The number of previous-frame neighbors.*/
  unsigned char npneighbors;
  /*Flags indicating which MB modes have been refined.*/
  unsigned char refined;
  /*Motion vectors for a macro block for the current frame and the
     previous two frames.
    Each is a set of 2 vectors against OC_FRAME_GOLD and OC_FRAME_PREV, which
     can be used to estimate constant velocity and constant acceleration
     predictors.
    Uninitialized MVs are (0,0).*/
  oc_mv         analysis_mv[3][2]; /* [cur,prev,prev2][frame,golden] */
  /*Current unrefined analysis MVs.*/
  oc_mv         unref_mv[2];
  /*Unrefined block MVs.*/
  oc_mv         block_mv[4];
  /*Refined block MVs.*/
  oc_mv         ref_mv[4];
  /*Minimum motion estimation error from the analysis stage.*/
  ogg_uint16_t  error[2];
  /*MB error for half-pel refinement for each frame type.*/
  unsigned      satd[2];
  /*Block error for half-pel refinement.*/
  unsigned      block_satd[4];
};



/*State machine to estimate the opportunity cost of coding a MB mode.*/
struct oc_mode_scheme_chooser{
  /*Pointers to the a list containing the index of each mode in the mode
     alphabet used by each scheme.
    The first entry points to the dynamic scheme0_ranks, while the remaining 7
     point to the constant entries stored in OC_MODE_SCHEMES.*/
  const unsigned char *mode_ranks[8];
  /*The ranks for each mode when coded with scheme 0.
    These are optimized so that the more frequent modes have lower ranks.*/
  unsigned char        scheme0_ranks[OC_NMODES];
  /*The list of modes, sorted in descending order of frequency, that
    corresponds to the ranks above.*/
  unsigned char        scheme0_list[OC_NMODES];
  /*The number of times each mode has been chosen so far.*/
  int                  mode_counts[OC_NMODES];
  /*The list of mode coding schemes, sorted in ascending order of bit cost.*/
  unsigned char        scheme_list[8];
  /*The number of bits used by each mode coding scheme.*/
  int                  scheme_bits[8];
};


void oc_mode_scheme_chooser_init(oc_mode_scheme_chooser *_chooser);



/*Rate control state information.*/
struct oc_rc_state{
  /*The target average bits per frame.*/
  ogg_int64_t  bits_per_frame;
  /*The current buffer fullness (bits available to be used).*/
  ogg_int64_t  fullness;
  /*The target buffer fullness.
    This is where we'd like to be by the last keyframe the appears in the next
     buf_delay frames.*/
  ogg_int64_t  target;
  /*The maximum buffer fullness (total size of the buffer).*/
  ogg_int64_t  max;
  /*The log of the number of pixels in a frame in Q57 format.*/
  ogg_int64_t  log_npixels;
  /*The exponent used in the rate model in Q8 format.*/
  unsigned     exp[2];
  /*The number of frames to distribute the buffer usage over.*/
  int          buf_delay;
  /*The total drop count from the previous frame.
    This includes duplicates explicitly requested via the
     TH_ENCCTL_SET_DUP_COUNT API as well as frames we chose to drop ourselves.*/
  ogg_uint32_t prev_drop_count;
  /*The log of an estimated scale factor used to obtain the real framerate, for
     VFR sources or, e.g., 12 fps content doubled to 24 fps, etc.*/
  ogg_int64_t  log_drop_scale;
  /*The log of estimated scale factor for the rate model in Q57 format.*/
  ogg_int64_t  log_scale[2];
  /*The log of the target quantizer level in Q57 format.*/
  ogg_int64_t  log_qtarget;
};


void oc_enc_calc_lambda(oc_enc_ctx *_enc,int _frame_type);
void oc_rc_state_init(oc_rc_state *_rc,const oc_enc_ctx *_enc);
void oc_enc_update_rc_state(oc_enc_ctx *_enc,
 long _bits,int _qti,int _qi,int _trial);
int oc_enc_select_qi(oc_enc_ctx *_enc,int _qti,int _clamp);



/*The internal encoder state.*/
struct th_enc_ctx{
  /*Shared encoder/decoder state.*/
  oc_theora_state          state;
  /*Buffer in which to assemble packets.*/
  oggpack_buffer           opb;
  /*Encoder-specific macroblock information.*/
  oc_mb_enc_info          *mb_info;
  /*DC coefficients after prediction.*/
  ogg_int16_t             *frag_dc;
  /*The list of coded macro blocks, in coded order.*/
  unsigned                *coded_mbis;
  /*The number of coded macro blocks.*/
  size_t                   ncoded_mbis;
  /*Whether or not packets are ready to be emitted.
    This takes on negative values while there are remaining header packets to
     be emitted, reaches 0 when the codec is ready for input, and becomes
     positive when a frame has been processed and data packets are ready.*/
  int                      packet_state;
  /*The maximum distance between keyframes.*/
  ogg_uint32_t             keyframe_frequency_force;
  /*The number of duplicates to produce for the next frame.*/
  ogg_uint32_t             dup_count;
  /*The number of duplicates remaining to be emitted for the current frame.*/
  ogg_uint32_t             nqueued_dups;
  /*The number of duplicates emitted for the last frame.*/
  ogg_uint32_t             prev_dup_count;
  /*Whether or not VP3 compatibility mode has been enabled.*/
  unsigned char            vp3_compatible;
  /*Whether or not any INTER frames have been coded.*/
  unsigned char            coded_inter_frame;
  /*Stores most recently chosen Huffman tables for each frame type, DC and AC
     coefficients, and luma and chroma tokens.
    The actual Huffman table used for a given coefficient depends not only on
     the choice made here, but also its index in the zig-zag ordering.*/
  unsigned char            huff_idxs[2][2][2];
  /*Current count of bits used by each MV coding mode.*/
  size_t                   mv_bits[2];
  /*The mode scheme chooser for estimating mode coding costs.*/
  oc_mode_scheme_chooser   chooser;
  /*The DCT token lists for each coefficient and each plane.*/
  unsigned char          **dct_tokens[3];
  /*The extra bits associated with each DCT token.*/
  ogg_uint16_t           **extra_bits[3];
  /*The number of DCT tokens for each coefficient for each plane.*/
  ptrdiff_t                ndct_tokens[3][64];
  /*Pending EOB runs for each coefficient for each plane.*/
  ogg_uint16_t             eob_run[3][64];
  /*The offset of the first DCT token for each coefficient for each plane.*/
  unsigned char            dct_token_offs[3][64];
  /*The last DC coefficient for each plane and reference frame.*/
  int                      dc_pred_last[3][3];
#if defined(OC_COLLECT_METRICS)
  /*Fragment SATD statistics for MB mode estimation metrics.*/
  int                     *frag_satd;
  /*Fragment SSD statistics for MB mode estimation metrics.*/
  int                     *frag_ssd;
#endif
  /*The R-D optimization parameter.*/
  int                      lambda;
  /*The huffman tables in use.*/
  th_huff_code             huff_codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS];
  /*The quantization parameters in use.*/
  th_quant_info            qinfo;
  oc_iquant               *enquant_tables[64][3][2];
  oc_iquant_table          enquant_table_data[64][3][2];
  /*An "average" quantizer for each quantizer type (INTRA or INTER) and qi
     value.
    This is used to paramterize the rate control decisions.
    They are kept in the log domain to simplify later processing.
    Keep in mind these are DCT domain quantizers, and so are scaled by an
     additional factor of 4 from the pixel domain.*/
  ogg_int64_t              log_qavg[2][64];
  /*The buffer state used to drive rate control.*/
  oc_rc_state              rc;
  /*Table for encoder acceleration functions.*/
  oc_enc_opt_vtable        opt_vtable;
};


int oc_enc_analyze(oc_enc_ctx *_enc,int _frame_type,int _recode);
#if defined(OC_COLLECT_METRICS)
extern void ModeMetrics(oc_enc_ctx *_enc);
extern void oc_enc_mode_metrics_dump(oc_enc_ctx *_enc);
#endif



/*Temporary state used for motion estimation.*/
struct oc_mcenc_ctx{
  /*The candidate motion vectors.*/
  int                candidates[12][2];
  /*The start of the Set B candidates.*/
  int                setb0;
  /*The total number of candidates.*/
  int                ncandidates;
  /*Accelerated predictor weights for each frame type.*/
  ogg_int32_t        mvapw1[2];
  ogg_int32_t        mvapw2[2];
};


/*Prep the motion search for the next frame.*/
void oc_mcenc_start(oc_enc_ctx *_enc,oc_mcenc_ctx *_mcenc);

/*Search for a single MB MV (and with OC_FRAME_PREV, block MVs) in one frame.*/
void oc_mcenc_search(oc_enc_ctx *_enc,oc_mcenc_ctx *_mcenc,int _mbi,int _frame);
/*Refine a MB MV for one frame.*/
void oc_mcenc_refine1mv(oc_enc_ctx *_enc,int _mbi,int _frame);
/*Refine the block MVs.*/
void oc_mcenc_refine4mv(oc_enc_ctx *_enc,int _mbi);



/*Used to rollback a tokenlog transaction when we retroactively decide to skip
   a fragment.
  A checkpoint is taken right before each token is added.*/
struct oc_token_checkpoint{
  /*The color plane the token was added to.*/
  unsigned char pli;
  /*The zig-zag index the token was added to.*/
  unsigned char zzi;
  /*The outstanding EOB run count before the token was added.*/
  ogg_uint16_t  eob_run;
  /*The token count before the token was added.*/
  ptrdiff_t     ndct_tokens;
};



void oc_enc_tokenize_start(oc_enc_ctx *_enc);
int oc_enc_tokenize_ac(oc_enc_ctx *_enc,ptrdiff_t _fragi,ogg_int16_t *_qdct,
 const ogg_uint16_t *_dequant,const ogg_int16_t *_dct,int _pli,
 oc_token_checkpoint **_stack,int _acmin);
void oc_enc_tokenlog_rollback(oc_enc_ctx *_enc,
 const oc_token_checkpoint *_stack,int _n);
void oc_enc_tokenize_finish(oc_enc_ctx *_enc);



/*Utility routine to encode one of the header packets.*/
int oc_state_flushheader(oc_theora_state *_state,int *_packet_state,
 oggpack_buffer *_opb,const th_quant_info *_qinfo,
 const th_huff_code _codes[TH_NHUFFMAN_TABLES][TH_NDCT_TOKENS],
 const char *_vendor,th_comment *_tc,ogg_packet *_op);



/*Encoder-specific accelerated functions.*/
void oc_enc_frag_sub(const oc_enc_ctx *_enc,ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128(const oc_enc_ctx *_enc,ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
unsigned oc_enc_frag_sad(const oc_enc_ctx *_enc,const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh(const oc_enc_ctx *_enc,
 const unsigned char *_src,const unsigned char *_ref1,
 const unsigned char *_ref2,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_intra_satd(const oc_enc_ctx *_enc,
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2(const oc_enc_ctx *_enc,unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
void oc_enc_frag_recon_intra(const oc_enc_ctx *_enc,
 unsigned char *_dst,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_frag_recon_inter(const oc_enc_ctx *_enc,unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_enc_fdct8x8(const oc_enc_ctx *_enc,ogg_int16_t _y[64],
 const ogg_int16_t _x[64]);

/*Default pure-C implementations.*/
void oc_enc_vtable_init_c(oc_enc_ctx *_enc);

void oc_enc_frag_sub_c(ogg_int16_t _diff[64],
 const unsigned char *_src,const unsigned char *_ref,int _ystride);
void oc_enc_frag_sub_128_c(ogg_int16_t _diff[64],
 const unsigned char *_src,int _ystride);
void oc_enc_frag_copy2_c(unsigned char *_dst,
 const unsigned char *_src1,const unsigned char *_src2,int _ystride);
unsigned oc_enc_frag_sad_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride);
unsigned oc_enc_frag_sad_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_sad2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_satd_thresh_c(const unsigned char *_src,
 const unsigned char *_ref,int _ystride,unsigned _thresh);
unsigned oc_enc_frag_satd2_thresh_c(const unsigned char *_src,
 const unsigned char *_ref1,const unsigned char *_ref2,int _ystride,
 unsigned _thresh);
unsigned oc_enc_frag_intra_satd_c(const unsigned char *_src,int _ystride);
void oc_enc_fdct8x8_c(ogg_int16_t _y[64],const ogg_int16_t _x[64]);

#endif
