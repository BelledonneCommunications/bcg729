/*
 bcg729_enc.c

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
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mscodecutils.h"
#include "bcg729/encoder.h"

/*filter common method*/
struct bcg729Encoder_struct {
    bcg729EncoderChannelContextStruct *encoderChannelContext;
	MSBufferizer *bufferizer;
	unsigned char ptime;
	unsigned char max_ptime;
	uint32_t ts;
};

static void filter_init(MSFilter *f){
	f->data = ms_new0(struct bcg729Encoder_struct,1);
	struct bcg729Encoder_struct* obj= (struct bcg729Encoder_struct*) f->data;
	obj->ptime=20;
	obj->max_ptime=100;
}

static void filter_preprocess(MSFilter *f){
	struct bcg729Encoder_struct* obj= (struct bcg729Encoder_struct*) f->data;

	obj->encoderChannelContext = initBcg729EncoderChannel(); /* initialize bcg729 encoder, return context */
	obj->bufferizer=ms_bufferizer_new();
}

static void filter_process(MSFilter *f){
	struct bcg729Encoder_struct* obj= (struct bcg729Encoder_struct*) f->data;
	mblk_t *inputMessage;
	mblk_t *outputMessage=NULL;
	uint8_t inputBuffer[160]; /* 2bytes per sample at 8000Hz -> 16 bytes per ms */
	uint8_t bufferIndex=0;
	/* get all input data into a buffer */
	while((inputMessage=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(obj->bufferizer,inputMessage);
	}
	
	/* process ptimes ms of data : (ptime in ms)/1000->ptime is seconds * 8000(sample rate) * 2(byte per sample) */
	while(ms_bufferizer_get_avail(obj->bufferizer)>=obj->ptime*16){
		outputMessage = allocb(obj->ptime,0); /* output bitStream is 80 bits long * number of samples */
		/* process buffer in 10 ms frames */
		for (bufferIndex=0; bufferIndex<obj->ptime; bufferIndex+=10) {
			ms_bufferizer_read(obj->bufferizer,inputBuffer,160);
			bcg729Encoder(obj->encoderChannelContext, (int16_t *)inputBuffer, outputMessage->b_wptr);
			outputMessage->b_wptr+=10;

		}
		obj->ts+=obj->ptime*8;
		mblk_set_timestamp_info(outputMessage,obj->ts);
		ms_queue_put(f->outputs[0],outputMessage);
	}

}

static void filter_postprocess(MSFilter *f){
	struct bcg729Encoder_struct* obj= (struct bcg729Encoder_struct*) f->data;
	ms_bufferizer_destroy(obj->bufferizer);
	closeBcg729EncoderChannel(obj->encoderChannelContext);
}

static void filter_uninit(MSFilter *f){
	ms_free(f->data);
}


/*filter specific method*/

static int filter_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	struct bcg729Encoder_struct* obj= (struct bcg729Encoder_struct*) f->data;
	const char *fmtp=(const char *)arg;
	buf[0] ='\0';
	
	if (fmtp_get_value(fmtp,"maxptime:",buf,sizeof(buf))){
		obj->max_ptime=atoi(buf);
		if (obj->max_ptime <10 || obj->max_ptime >100 ) {
			ms_warning("MSBCG729Enc: unknown value [%i] for maxptime, use default value (100) instead",obj->max_ptime);
			obj->max_ptime=100;
		}
		ms_message("MSBCG729Enc: got maxptime=%i",obj->max_ptime);
	} else 	if (fmtp_get_value(fmtp,"ptime",buf,sizeof(buf))){
		obj->ptime=atoi(buf);
		if (obj->ptime > obj->max_ptime) {
			obj->ptime=obj->max_ptime;
		} else if (obj->ptime%10) {
		//if the ptime is not a mulptiple of 10, go to the next multiple
		obj->ptime = obj->ptime - obj->ptime%10 + 10; 
		}
		
		ms_message("MSBCG729Enc: got ptime=%i",obj->ptime);
	}
	return 0;
}

static MSFilterMethod filter_methods[]={
	{	MS_FILTER_ADD_FMTP		,	filter_add_fmtp },
	{	0, NULL}
};



MSFilterDesc ms_bcg729_enc_desc={
	.id=MS_FILTER_PLUGIN_ID, /* from Allfilters.h*/
	.name="MSBCG729Enc",
	.text="G729 audio encoder filter.",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G729",
	.ninputs=1, /*number of inputs*/
	.noutputs=1, /*number of outputs*/
	.init=filter_init,
	.preprocess=filter_preprocess,
	.process=filter_process,
	.postprocess=filter_postprocess,
	.uninit=filter_uninit,
	.methods=filter_methods
};
MS_FILTER_DESC_EXPORT(ms_bcg729_enc_desc)

extern MSFilterDesc ms_bcg729_dec_desc;
#ifndef VERSION
	#define VERSION "debug"
#endif
void libmsbcg729_init(){
	ms_filter_register(&ms_bcg729_enc_desc);
	ms_filter_register(&ms_bcg729_dec_desc);
	ms_message(" libmsbcg729 " VERSION " plugin loaded");
}
