/*
 * CAVideoTrans.h
 *
 *  Created on: Dec 7, 2015
 *      Author: Xiangbo liu
 */

#ifndef CAVIDEOTRANS_H_
#define CAVIDEOTRANS_H_

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

#include  <libswresample/swresample.h>
#include  <libavformat/avformat.h>
#include  <libavcodec/avcodec.h>
//#include "libavutil/avassert.h"
//#include "libavutil/avutil.h"
//#include "libavutil/opt.h"


#ifdef __cplusplus
}
#endif //__cplusplus

#include "CAException.h"
#include "CAZeroMQ.h"
#include  <vector>

#define CHANNLE_LAYOUT(chanel) av_get_default_channel_layout(chanel)
/** The number of channels layout*/
//#define OUTPUT_CHANNEL_LAYOUT AV_CH_FRONT_LEFT
#define INPUT_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO
/** The audio sample output format */
#define INPUT_SAMPLE_FORMAT AV_SAMPLE_FMT_S16
#define USE_G726_AAC_ECODEC 1

/*
 * V1.4.1 Add data structure
 * must slave 3 nums audio frames data
 * ecodec need A frame near 1024 size,
 * Don't > 1024 size;
 */
typedef struct av_decodec_packet {
	/*
	 * include three audio frame data,audio frame length always equal 164
	 * only three audio frame transfer to a packet can play normal
	 */
	uint8_t pAudioData[1024];
	int nAudioFrameNums;
} AvDecodecPacket;

/*
 * data exchange structure
 */
typedef struct buffer_data {
	/*
	 * include I,P,B Video data
	 */
	uint8_t *pFrameData;
    size_t nFrameLen;
    /*
     * include third-party bits
     */
    uint8_t *pExtraData;
    size_t nExtraDataLen;
    /*
     * include Mdvrinfo data
     * define in CAPublicStruct class
     */
    Mdvrinfo *pMdvrinfoData;
    size_t nMdvrinfoDataLen;
    /*
     * 1.2Ver Before No used
     */
    uint8_t *pCustomValue;
    size_t nCustomDataLen;

}AvInOutBufferData;
typedef int (*readwrite_packet)(void *opaque, uint8_t *buf, int buf_size);

class CAVideoTrans {
public:
	enum {
	INPUT_SAMPLE_RATE = 8000,
	INPUT_CHANNELS = 1,
	INPUT_FRAME_SIZE = 1024, //AAC always equal 1024
	OUTPUT_BIT_RATE = 16000, //which effect audio packet.size
	OUTPUT_CHANNELS = 2};

	CAVideoTrans(readwrite_packet reader,readwrite_packet writer);
	int TransH264ToTs(Mdvrinfo* pMdvrinfo,size_t len,unsigned char* frame,size_t framelen, unsigned char* extra,size_t extralen);
	virtual ~CAVideoTrans();

private:
	CALog* m_pLog;
	AVFormatContext	*m_pOutputFmtCtx;
	AVFormatContext	*m_pInputFmtCtx;
	readwrite_packet m_pReadCallback;
	readwrite_packet m_pWriteCallback;
	AvInOutBufferData m_InDataBuffer;

	AvDecodecPacket m_DecodecPacket;
	AVIOContext* m_pavio_ctx;
	void *m_pOrigOpaque;

	//ensure the packet.stream_index,reduce calculate
	AVStream* m_pVideoStream;
	AVStream* m_pAudioStream;

	//decode and encode audio need those
	AVCodecContext* m_pAudioDecodeCtx;
	AVCodecContext* m_pAudioEncodeCtx;
	SwrContext* m_pAudioSwrCtx;

	//sync video and audio pts
	bool m_FirstFrame;
	long long m_firstVideoPts;
	long long m_firstAudioPts;
	int64_t m_VideoPts;
	int64_t m_AudioPts;
	uint64_t m_audio_pts_increment;

	int InitVideoContext();
	int InitAudioContext();
	int WriteHeader();
	int DealIOrPFrame(AVPacket &packet);
	int DealUnknownFrame(AVPacket &packet);
	int DealAFrame(AVPacket &packet);
	int DealExtraFrame(AVPacket &packet);
	int DecodeAudioFrame(AVFrame* poutFrame);
	int EncodeAudioFrame(AVPacket &packet,AVFrame* poutFrame);
	int PrepareVideo(int num);
	int SelectChannelLayout(const AVCodec *pCodec);
};

#endif /* CAVIDEOTRANS_H_ */
