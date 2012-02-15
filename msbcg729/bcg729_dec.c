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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "mediastreamer2/mscodecutils.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "bcg729/decoder.h"

/* signal and bitstream frame size in byte */
#define SIGNAL_FRAME_SIZE 160
#define BITSTREAM_FRAME_SIZE 10

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
			outputMessage = allocb(SIGNAL_FRAME_SIZE,0);
			bcg729Decoder(obj->decoderChannelContext, inputMessage->b_rptr, 0, (int16_t *)(outputMessage->b_wptr));
			outputMessage->b_wptr+=SIGNAL_FRAME_SIZE;
			inputMessage->b_rptr+=BITSTREAM_FRAME_SIZE;
			ms_queue_put(f->outputs[0],outputMessage);
			ms_concealer_inc_sample_time(obj->concealer,f->ticker->time,10, 1);
		}
		freemsg(inputMessage);
	}

	if (ms_concealer_context_is_concealement_required(obj->concealer, f->ticker->time)) {
		outputMessage = allocb(SIGNAL_FRAME_SIZE,0);
		bcg729Decoder(obj->decoderChannelContext, NULL, 1, (int16_t *)(outputMessage->b_wptr));
		outputMessage->b_wptr+=SIGNAL_FRAME_SIZE;
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


/*filter specific method*/

static MSFilterMethod filter_methods[]={
	{	0, NULL}
};



MSFilterDesc ms_bcg729_dec_desc={
	.id=MS_FILTER_PLUGIN_ID, /* from Allfilters.h*/
	.name="MSBCG729Dec",
	.text="G729 decoder filter.",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G729",
	.ninputs=1, /*number of inputs*/
	.noutputs=1, /*number of outputs*/
	.init=filter_init,
	.preprocess=filter_preprocess,
	.process=filter_process,
	.postprocess=filter_postprocess,
	.uninit=filter_uninit,
	.methods=filter_methods,
	.flags=MS_FILTER_IS_PUMP
};
MS_FILTER_DESC_EXPORT(ms_bcg729_dec_desc)
