/*
 bcg729_dec.c

 Copyright (C) 2011 Belledonne Communications, Grenoble, France
 Author : Jehan Monnier
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "mediastreamer2/mscodecutils.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "bcg729/decoder.h"

/* signal and bitstream frame size in byte */
#define SIGNAL_FRAME_SIZE 160
#define BITSTREAM_FRAME_SIZE 10
#define NOISE_BITSTREAM_FRAME_SIZE 2

/* decoder struct: context for decoder channel and concealment */
struct bcg729Decoder_struct {
	bcg729DecoderChannelContextStruct *decoderChannelContext;
	MSConcealerContext *concealer;
};

static void filter_init(MSFilter *f){
	f->data = ms_new0(struct bcg729Decoder_struct,1);
}

static void filter_preprocess(MSFilter *f){
	struct bcg729Decoder_struct* obj= (struct bcg729Decoder_struct*) f->data;
	obj->decoderChannelContext = initBcg729DecoderChannel(); /* initialize bcg729 decoder, return channel context */
	obj->concealer = ms_concealer_context_new(UINT32_MAX);
}

static void filter_process(MSFilter *f){
	struct bcg729Decoder_struct* obj= (struct bcg729Decoder_struct*) f->data;
	mblk_t *inputMessage, *outputMessage;

	while((inputMessage=ms_queue_get(f->inputs[0]))) {
		while(inputMessage->b_rptr<inputMessage->b_wptr) {
			/* if remaining data in RTP payload have the size of a SID frame it must be one, see RFC3551 section 4.5.6 : any SID frame must be the last one of the RPT payload */
			uint8_t SIDFrameFlag = ((inputMessage->b_wptr-inputMessage->b_rptr)==NOISE_BITSTREAM_FRAME_SIZE)?1:0;
			outputMessage = allocb(SIGNAL_FRAME_SIZE,0);
			mblk_meta_copy(inputMessage, outputMessage);
			bcg729Decoder(obj->decoderChannelContext, inputMessage->b_rptr, (SIDFrameFlag==1)?NOISE_BITSTREAM_FRAME_SIZE:BITSTREAM_FRAME_SIZE, 0, SIDFrameFlag, 0, (int16_t *)(outputMessage->b_wptr));
			outputMessage->b_wptr+=SIGNAL_FRAME_SIZE;
			inputMessage->b_rptr += (SIDFrameFlag==1)?NOISE_BITSTREAM_FRAME_SIZE:BITSTREAM_FRAME_SIZE;
			ms_queue_put(f->outputs[0],outputMessage);
			ms_concealer_inc_sample_time(obj->concealer,f->ticker->time,10, 1);
		}
		freemsg(inputMessage);
	}

	if (ms_concealer_context_is_concealement_required(obj->concealer, f->ticker->time)) {
		outputMessage = allocb(SIGNAL_FRAME_SIZE,0);
		bcg729Decoder(obj->decoderChannelContext, NULL, 0, 1, 0, 0, (int16_t *)(outputMessage->b_wptr));
		outputMessage->b_wptr+=SIGNAL_FRAME_SIZE;
		mblk_set_plc_flag(outputMessage, 1);
		ms_queue_put(f->outputs[0],outputMessage);
		ms_concealer_inc_sample_time(obj->concealer,f->ticker->time,10, 0);
	}
}

static void filter_postprocess(MSFilter *f){
	struct bcg729Decoder_struct* obj= (struct bcg729Decoder_struct*) f->data;
	ms_concealer_context_destroy(obj->concealer);
	closeBcg729DecoderChannel(obj->decoderChannelContext);
}

static void filter_uninit(MSFilter *f){
	ms_free(f->data);
}

static int filter_have_plc(MSFilter *f, void *arg)
{
	*((int *)arg) = 1;
	return 0;
}

/*filter specific method*/

static MSFilterMethod filter_methods[]={
	{ 	MS_DECODER_HAVE_PLC		, 	filter_have_plc		},
	{	0				, 	NULL			}
};


#define MS_BCG729_DEC_ID				MS_FILTER_PLUGIN_ID
#define MS_BCG729_DEC_NAME				"MSBCG729Dec"
#define MS_BCG729_DEC_DESCRIPTION		"G729 audio decoder filter"
#define MS_BCG729_DEC_CATEGORY			MS_FILTER_DECODER
#define MS_BCG729_DEC_ENC_FMT			"G729"
#define MS_BCG729_DEC_NINPUTS			1
#define MS_BCG729_DEC_NOUTPUTS			1
#define MS_BCG729_DEC_FLAGS				MS_FILTER_IS_PUMP

#ifndef _MSC_VER

MSFilterDesc ms_bcg729_dec_desc={
	.id=MS_BCG729_DEC_ID,
	.name=MS_BCG729_DEC_NAME,
	.text=MS_BCG729_DEC_DESCRIPTION,
	.category=MS_BCG729_DEC_CATEGORY,
	.enc_fmt=MS_BCG729_DEC_ENC_FMT,
	.ninputs=MS_BCG729_DEC_NINPUTS, /*number of inputs*/
	.noutputs=MS_BCG729_DEC_NOUTPUTS, /*number of outputs*/
	.init=filter_init,
	.preprocess=filter_preprocess,
	.process=filter_process,
	.postprocess=filter_postprocess,
	.uninit=filter_uninit,
	.methods=filter_methods,
	.flags=MS_BCG729_DEC_FLAGS
};

#else

MSFilterDesc ms_bcg729_dec_desc={
	MS_BCG729_DEC_ID,
	MS_BCG729_DEC_NAME,
	MS_BCG729_DEC_DESCRIPTION,
	MS_BCG729_DEC_CATEGORY,
	MS_BCG729_DEC_ENC_FMT,
	MS_BCG729_DEC_NINPUTS,
	MS_BCG729_DEC_NOUTPUTS,
	filter_init,
	filter_preprocess,
	filter_process,
	filter_postprocess,
	filter_uninit,
	filter_methods,
	MS_BCG729_DEC_FLAGS
};

#endif

MS_FILTER_DESC_EXPORT(ms_bcg729_dec_desc)
