/*
 * CAVideoTrans.cpp
 *
 *  Created on: Dec 7, 2015
 *      Author: Xiangbo liu
 */

#include "CAVideoTrans.h"

CAVideoTrans::CAVideoTrans(readwrite_packet reader, readwrite_packet writer) :
		m_pReadCallback(reader), m_pWriteCallback(writer), m_VideoPts(0), m_AudioPts(
				0) {
	int temp = 0;
	m_pavio_ctx = NULL;
	m_pOrigOpaque = NULL;

	m_pVideoStream = NULL;
	m_pAudioStream = NULL;

	m_pAudioDecodeCtx = NULL;
	m_pAudioEncodeCtx = NULL;
	m_pAudioSwrCtx = NULL;

	m_pLog = CALog::GetInstance(g_data.ZlogCname);
	m_FirstFrame = false;
	m_firstVideoPts = 0;
	m_firstAudioPts = 0;

	/*
	 * 11520 = 90000 * INPUT_FRAME_SIZE / m_pAudioEncodeCtx->sample_rate
	 */
	m_audio_pts_increment = 11520;

	av_register_all();
	memset(&m_InDataBuffer, 0, sizeof(AvInOutBufferData));

	/* allocate the input media context */
	m_pInputFmtCtx = avformat_alloc_context();
	if (!m_pInputFmtCtx) {
		m_pLog->Error("%s %d %s", __FILE__, __LINE__,
				"avformat_alloc_context() is not successful!");
		throw(CAException("avformat_alloc_context error"));
	}

	m_pInputFmtCtx->pb = avio_alloc_context(NULL, 0, 0, &m_InDataBuffer,
			this->m_pReadCallback, NULL, NULL);
	if (!m_pInputFmtCtx->pb) {
		m_pLog->Error("%s %d %s", __FILE__, __LINE__,
				"avio_alloc_context() is not successful!");
		throw(CAException("avio_alloc_context error"));
	}

	temp = avformat_alloc_output_context2(&m_pOutputFmtCtx, NULL,
			g_data.AvOutputFormat.c_str(), NULL);
	if (temp < 0) {
		m_pLog->Error("%s %d %s", __FILE__, __LINE__,
				"avformat_alloc_output_context2() is not successful!");
		avformat_free_context(m_pInputFmtCtx);
		throw(CAException("avformat_alloc_output_context2 error!"));
	}

	m_DecodecPacket.nAudioFrameNums = 0;
	memset(m_DecodecPacket.pAudioData, 0, 1024);

	m_pLog->Info("CAVideoTrans::CAVideoTrans()");
}

int CAVideoTrans::TransH264ToTs(Mdvrinfo* pMdvrinfo, size_t len,
		unsigned char* frame, size_t framelen, unsigned char* extra,
		size_t extralen) {
	m_InDataBuffer.pFrameData = frame;
	m_InDataBuffer.nFrameLen = framelen;
	m_InDataBuffer.pExtraData = extra;
	m_InDataBuffer.nExtraDataLen = extralen;
	m_InDataBuffer.pMdvrinfoData = pMdvrinfo;
	m_InDataBuffer.nMdvrinfoDataLen = len;
	if (!m_FirstFrame) {
		m_InDataBuffer.pCustomValue = frame;
		m_InDataBuffer.nCustomDataLen = len;
	}
//	m_pLog->Debug("received pts:%lld", m_InDataBuffer.pMdvrinfoData->pts);

	if (!m_FirstFrame) {
		try {
			InitVideoContext();
			InitAudioContext();
			WriteHeader();
		} catch (const CAException &excp) {
			m_pLog->Error("InitAVCodecDataStruct() false;%s %d %s", __FILE__,
			__LINE__, excp.what());
			return -1;
		}
		m_FirstFrame = true;
	}

	AVPacket packet;
	av_init_packet(&packet);

	try {
		switch (m_InDataBuffer.pMdvrinfoData->frameType) {
		case FRAME_TYPE_I_FRAME:
		case FRAME_TYPE_P_FRAME:
			DealIOrPFrame(packet);
			break;
		case FRAME_TYPE_A_FRAME:
			DealAFrame(packet);
			if (m_DecodecPacket.nAudioFrameNums % 3)
				return 0;
			break;
		case FRAME_TYPE_EXTRA_FRAME:
			DealExtraFrame(packet);
			break;
		case FRAME_TYPE_UNKNOWN:
			DealUnknownFrame(packet);
			return 0;
		}
	} catch (CAException &ex) {
		this->m_pLog->Error(ex.what());
	}

	int ret = av_interleaved_write_frame(m_pOutputFmtCtx, &packet);
	if (ret != 0) {
		char szError[256] = { 0 };
		av_strerror(ret, szError, 256);
		m_pLog->Info("av_interleaved_write_frame() false!"
				"Errmsg[%s];status[%d]", szError, ret);
		return -1;
	}

	av_free_packet(&packet);
	return 0;
}

CAVideoTrans::~CAVideoTrans() {
	m_pLog->Debug("~CAVideoTrans ----begin!");
//	//free input and output format context,codec,io context
	if (m_FirstFrame) {
//		av_free(m_pInputFmtCtx->pb);
		for (size_t i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
			avcodec_close(m_pInputFmtCtx->streams[i]->codec);
			m_pLog->Debug("~CAVideoTrans -----in nb_streams");
			if (m_pOutputFmtCtx && m_pOutputFmtCtx->nb_streams > i
					&& m_pOutputFmtCtx->streams[i]
					&& m_pOutputFmtCtx->streams[i]->codec) {
				avcodec_close(m_pOutputFmtCtx->streams[i]->codec);
				m_pLog->Debug("~CAVideoTrans -----out nb_streams");
			}
		}
		avformat_close_input(&m_pInputFmtCtx); // corresponding avformat_open_input
		m_pLog->Debug("~CAVideoTrans -----m_pInputFmtCtx");
	} else {
		avformat_free_context(m_pInputFmtCtx);
	}

	if (m_pAudioSwrCtx)
		swr_free(&m_pAudioSwrCtx);
	m_pLog->Debug("~CAVideoTrans ------- m_pAudioSwrCtx");
	if (m_pAudioEncodeCtx)
		avcodec_close(m_pAudioEncodeCtx);
	m_pLog->Debug("~CAVideoTrans ------- m_pAudioEncodeCtx");

	if (m_pavio_ctx) {
		av_freep(&m_pavio_ctx->buffer);
		av_freep(&m_pavio_ctx);
	}
	m_pLog->Debug("~CAVideoTrans ------- m_pavio_ctx");
	avformat_free_context(m_pOutputFmtCtx);

	if (m_pAudioDecodeCtx)
		avcodec_free_context(&m_pAudioDecodeCtx);
	m_pLog->Debug("~CAVideoTrans ------- m_pAudioDecodeCtx");

	if (m_pInputFmtCtx) {
		av_free(m_pInputFmtCtx->pb);
		avformat_close_input(&m_pInputFmtCtx);
	}
	m_pLog->Info("CAVideoTrans::~CAVideoTrans()");
}

int CAVideoTrans::InitAudioContext() {
	int ret = 0;
	char szError[256] = { 0 };

	AVCodec* pDecodeCodec = NULL;
	AVCodec* pEncodeCodec = NULL;

	/* find the mpeg audio decoder eg: AV_CODEC_ID_ADPCM_G726*/
	if (!(pDecodeCodec = avcodec_find_decoder(AV_CODEC_ID_ADPCM_G726))) {
		m_pLog->Error("avcodec_find_decoder() false;");
		return -1;
	}

	m_pAudioDecodeCtx = avcodec_alloc_context3(pDecodeCodec);
	if (!m_pAudioDecodeCtx) {
		m_pLog->Error("avcodec_alloc_context3() false;");
		return -1;
	}
	m_pAudioDecodeCtx->bits_per_coded_sample = 4;
	m_pAudioDecodeCtx->channels = INPUT_CHANNELS;
	m_pAudioDecodeCtx->sample_rate = INPUT_SAMPLE_RATE;
	m_pAudioDecodeCtx->sample_fmt = INPUT_SAMPLE_FORMAT;
	m_pAudioDecodeCtx->channel_layout = CHANNLE_LAYOUT(INPUT_CHANNELS);

	if ((ret = avcodec_open2(m_pAudioDecodeCtx, pDecodeCodec, NULL)) < 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avcodec_open2() false;Errstr=%s", szError);
		return -1;
	}

	/*find encode*/
	pEncodeCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!pEncodeCodec)
		return -1;

	m_pAudioStream = avformat_new_stream(m_pOutputFmtCtx, pEncodeCodec);
	if (!m_pAudioStream) {
		m_pLog->Error("avformat_new_stream"
				" m_pOutputAFmtCtx (audio) error!!");
		return -1;
	}

	/*
	 * Set the basic encoder parameters.
	 * The input file's sample rate is used to avoid a sample rate conversion.
	 */
	m_pAudioEncodeCtx = m_pAudioStream->codec;
	m_pAudioEncodeCtx->sample_fmt = INPUT_SAMPLE_FORMAT;
	m_pAudioEncodeCtx->sample_rate = INPUT_SAMPLE_RATE;
	m_pAudioEncodeCtx->bit_rate = OUTPUT_BIT_RATE;
	m_pAudioEncodeCtx->channels = OUTPUT_CHANNELS;
	m_pAudioEncodeCtx->channel_layout = SelectChannelLayout(pEncodeCodec);
	m_pAudioEncodeCtx->time_base = av_make_q(1, m_pAudioEncodeCtx->sample_rate);

	/*
	 * Some container formats (like MP4) require global headers to be present
	 * Mark the encoder so that it behaves accordingly.
	 * Solution AAC bitstream not in ADTS format and extradata missing.
	 */
	if (m_pOutputFmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
		m_pAudioEncodeCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	/* Open the encoder for the audio stream to use it later. */
	if ((ret = avcodec_open2(m_pAudioEncodeCtx, pEncodeCodec, NULL)) < 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avcodec_open2() error!ErrMsg[%s]", szError);
		return -1;
	}

	/*
	 * Init the resampler to be able to convert audio sample formats
	 * 当源音频和目标音频格式或者采样率不同时,要进行转换
	 */
	m_pAudioSwrCtx = swr_alloc_set_opts(NULL,
			CHANNLE_LAYOUT(m_pAudioEncodeCtx->channels),
			m_pAudioEncodeCtx->sample_fmt, m_pAudioEncodeCtx->sample_rate,
			CHANNLE_LAYOUT(m_pAudioDecodeCtx->channels),
			m_pAudioDecodeCtx->sample_fmt, m_pAudioDecodeCtx->sample_rate, 0,
			NULL);
	if (!m_pAudioSwrCtx) {
		m_pLog->Error("swr_alloc_set_opts() false!!");
		return FUNCTION_ERROR;
	}

	/*
	 * Perform a sanity check so that the number of converted samples is
	 * not greater than the number of samples to be converted.
	 * If the sample rates differ, this case has to be handled differently
	 */
	if (m_pAudioEncodeCtx->sample_rate != m_pAudioDecodeCtx->sample_rate) {
		m_pLog->Error("swr_alloc() false!!");
		return FUNCTION_ERROR;
	}

	/* Open the resampler with the specified parameters. */
	if ((ret = swr_init(m_pAudioSwrCtx)) < 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("swr_init() error!ErrMsg[%s]", szError);
		swr_free(&m_pAudioSwrCtx);
	}

	m_pLog->Info("MDVR: %s; SessionID: %s InitAudioContext success!",
			this->m_InDataBuffer.pMdvrinfoData->zmqData.mdvrid,
			this->m_InDataBuffer.pMdvrinfoData->zmqData.handle);
	return 0;
}

int CAVideoTrans::InitVideoContext() {
	int ret = 0;
	char szError[256] = { 0 };

	ret = avformat_open_input(&m_pInputFmtCtx, NULL, NULL, NULL);
	if (ret != 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avformat_open_input() false;");
		throw(CAException(szError));
	}

	ret = avformat_find_stream_info(m_pInputFmtCtx, NULL);
	if (ret < 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avformat_find_stream_info() false! %s", szError);
		avformat_close_input(&m_pInputFmtCtx);
		throw(CAException(szError));
	}

	for (unsigned int i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
		switch (m_pInputFmtCtx->streams[i]->codec->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			if (PrepareVideo(i)) {
				return FUNCTION_ERROR;
			}
			break;
		default:
//			m_pInputFmtCtx->streams[i]->discard = AVDISCARD_ALL;
			m_pLog->Debug("Input stream is not video stream, stream ID:%d", i);
			break;
		}
	}
	m_pLog->Info("MDVR: %s; SessionID: %s InitVideoContext success!",
			this->m_InDataBuffer.pMdvrinfoData->zmqData.mdvrid,
			this->m_InDataBuffer.pMdvrinfoData->zmqData.handle);
	return 0;
}

int CAVideoTrans::WriteHeader() {
	int ret = 0;
	char szError[256] = { 0 };

	/*
	 * Enable optimize function
	 * second par replaced pipe
	 * Further performance improvements
	 */
	ret = avio_open(&m_pOutputFmtCtx->pb, "/dev/null", AVIO_FLAG_WRITE);
	if (ret < 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avio_open() m_pOutputFmtCtx false", szError);
		/* Close each codec stream at Destructor*/
		avformat_close_input(&m_pInputFmtCtx);
		throw(CAException(szError));
	}

	/*
	 * make sure the end data is in multiples of 188
	 */
	uint8_t *avio_ctx_buffer = NULL;
	avio_ctx_buffer = (uint8_t*) av_malloc(SENDSIZE * 10);
	m_pavio_ctx = avio_alloc_context(avio_ctx_buffer,
	SENDSIZE * 10, 1, &this->m_InDataBuffer.pMdvrinfoData->zmqData, NULL,
			this->m_pWriteCallback, NULL);
	if (!m_pavio_ctx) {
		m_pLog->Error("%s %d %s", __FILE__, __LINE__,
				" avio_alloc_context() is not successful!");
	}
	m_pOutputFmtCtx->pb = m_pavio_ctx;

//	//把原始数据保存,后期释放到时候还原,否则avio_close崩溃
//	m_pOrigOpaque = m_pOutputFmtCtx->pb->opaque;
//	m_pOutputFmtCtx->pb->opaque = &this->m_InDataBuffer.pMdvrinfoData->zmqData;
//	m_pOutputFmtCtx->pb->buffer_size = SENDSIZE * 10;
//	m_pOutputFmtCtx->pb->write_packet = m_pWriteCallback;

	ret = avformat_write_header(m_pOutputFmtCtx, NULL);
	if (ret != 0) {
		av_strerror(ret, szError, 256);
		m_pLog->Error("avformat_write_header() false", szError);
		/* Close each codec stream at destructor*/
		avformat_close_input(&m_pInputFmtCtx);
		throw(CAException(szError));
	}

	m_pLog->Info("MDVR: %s; SessionID: %s WriteHeader success!",
			this->m_InDataBuffer.pMdvrinfoData->zmqData.mdvrid,
			this->m_InDataBuffer.pMdvrinfoData->zmqData.handle);
	return 0;
}

int CAVideoTrans::DealIOrPFrame(AVPacket& packet) {
	packet.flags |= AV_PKT_FLAG_KEY;
	packet.stream_index = m_pVideoStream->index;

	/*
	 * PTS and DTS must add,without B-frames;
	 * PTS==DTS(StreamId=0);
	 * PTS!=DTS(StreamId!=0);
	 */
	if (this->m_firstVideoPts == 0) {
		m_firstVideoPts = m_InDataBuffer.pMdvrinfoData->pts;
		packet.dts = packet.pts = m_InDataBuffer.pMdvrinfoData->pts
				- m_firstVideoPts;
		m_VideoPts = packet.pts;
	} else {
		AVRational us_time_base = { 1, 1000 };
		packet.pts = av_rescale_q(
				m_InDataBuffer.pMdvrinfoData->pts - m_firstVideoPts,
				us_time_base,
				m_pOutputFmtCtx->streams[m_pVideoStream->index]->time_base);
		packet.dts = packet.pts;
		m_VideoPts = packet.pts;
	}
	//audio quicker than video
	if (m_AudioPts - m_VideoPts > 9000 && m_DecodecPacket.nAudioFrameNums)
		m_DecodecPacket.nAudioFrameNums--;
	//	video quicker than audio
	else if (m_VideoPts - m_AudioPts > 9000)
		m_AudioPts = m_VideoPts;

	packet.data = m_InDataBuffer.pFrameData;
	packet.size = m_InDataBuffer.nFrameLen;
//	m_pLog->Debug("DealIOrPFrame pts:%lld;dts:%lld;size:%d", packet.pts,
//			packet.dts, packet.size);
	return 0;
}

int CAVideoTrans::DealAFrame(AVPacket& packet) {
#if USE_G726_AAC_ECODEC

//	memcpy(m_DecodecPacket.pAudioData,m_InDataBuffer.pFrameData, m_InDataBuffer.nFrameLen);

	memcpy(
			m_DecodecPacket.pAudioData
					+ m_DecodecPacket.nAudioFrameNums
							* m_InDataBuffer.nFrameLen,
			m_InDataBuffer.pFrameData, m_InDataBuffer.nFrameLen);
	m_DecodecPacket.nAudioFrameNums += 1;
	if (m_DecodecPacket.nAudioFrameNums % 3) {
		return 0;
	}

	/*
	 * Decode the audio frame and store it in the temporary frame.
	 * The input audio stream decoder is used to do this.
	 */
	AVFrame* poutFrame = NULL;
	if (!(poutFrame = av_frame_alloc())) {
		int error = 0;
		char szError[256] = { 0 };
		av_strerror(error, szError, 256);
		m_pLog->Error("av_frame_alloc() [OutFrame] error!ErrMsg[%s]", szError);
		return -1;
	}

	if (DecodeAudioFrame(poutFrame)) {
		m_DecodecPacket.nAudioFrameNums = 0;
		m_pLog->Error("DecodeAudioFrame() false;");
		return FUNCTION_ERROR;
	}
	m_DecodecPacket.nAudioFrameNums = 0;
	/*
	 * Encode the audio frame and store it in the temporary packet.
	 * The output audio stream encoder is used to do this.
	 * last data eg: "ÿùl\200\200\037ü! \005 ¤\eÿÀ";
	 */
	if (EncodeAudioFrame(packet, poutFrame)) {
		m_pLog->Error("EcodecAudioFrame() false;");
		return FUNCTION_ERROR;
	}

#else //NO DEcodec
	m_Packet.data = m_Opaque.pFrameData;
	m_Packet.size = m_Opaque.nFrameLen;
#endif

	packet.flags |= AV_PKT_FLAG_KEY;
	packet.stream_index = m_pAudioStream->index;
	;
	if (this->m_firstAudioPts == 0) {
		this->m_firstAudioPts = m_InDataBuffer.pMdvrinfoData->pts;
		packet.dts = packet.pts = m_InDataBuffer.pMdvrinfoData->pts
				- m_firstAudioPts;
	} else {
		packet.pts = m_AudioPts + m_audio_pts_increment;
		m_AudioPts = packet.pts;
		packet.dts = packet.pts;
	}
//	m_pLog->Debug("DealAFrame pts:%lld", packet.pts);

	av_frame_free(&poutFrame);
	return 0;
}

int CAVideoTrans::DealUnknownFrame(AVPacket& packet) {
	//一次请求结束的最后一个数据包发送处理代码
	m_pLog->Info(
			"SendMsgToZeroMQ() endPacket!!SessionId=%s,sendFlag=[%d],MDVRID=[%s],Channelid=[%d]",
			m_InDataBuffer.pMdvrinfoData->zmqData.handle,
			m_InDataBuffer.pMdvrinfoData->zmqData.sendFlag,
			m_InDataBuffer.pMdvrinfoData->zmqData.mdvrid,
			m_InDataBuffer.pMdvrinfoData->zmqData.channelid);
	CAZeroMQ *pZeroMQ = CAZeroMQ::GetInstance("tcp", "*", DEFAULT_PORT,
	ZMQ_PUB);
	if (pZeroMQ->SendData(m_InDataBuffer.pFrameData, m_InDataBuffer.nFrameLen,
			&m_InDataBuffer.pMdvrinfoData->zmqData)) {
		m_pLog->Error("SendMsgToZeroMQ() false!!FrameType=%d",
				m_InDataBuffer.pMdvrinfoData->frameType);
	}
	return 0;
}

int CAVideoTrans::DealExtraFrame(AVPacket& packet) {
	m_pLog->Info("TransH264ToTs() stream_index=FRAME_TYPE_EXTRA_FRAME");
	return 0;
}

int CAVideoTrans::PrepareVideo(int num) {
	AVCodecContext* ic = NULL;
	AVCodecContext* oc = NULL;
	AVCodec* codec = NULL;

	ic = m_pInputFmtCtx->streams[num]->codec;
	codec = avcodec_find_encoder(m_pOutputFmtCtx->oformat->video_codec);
	if (!codec)
		return -1;
	m_pVideoStream = avformat_new_stream(m_pOutputFmtCtx, codec);
	if (!m_pVideoStream) {
		m_pLog->Error("avformat_new_stream m_pOutputFmtCtx (video) error!!!!!");
		return -1;
	}
	oc = m_pVideoStream->codec;

	oc->codec_id = ic->codec_id;
	oc->codec_type = ic->codec_type;
	oc->pix_fmt = ic->pix_fmt;
	oc->width = ic->width;
	oc->height = ic->height;
	oc->has_b_frames = ic->has_b_frames;
	oc->extradata = ic->extradata;
	oc->extradata_size = ic->extradata_size;

	m_pLog->Info("ic->width=%d   ic->height=%d", ic->width, ic->height);
	if (m_pOutputFmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
		oc->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	return 0;
}

int CAVideoTrans::SelectChannelLayout(const AVCodec *pCodec) {
	const uint64_t *p = NULL;
	uint64_t best_ch_layout = 0;
	int best_nb_channels = 0;

	if (!pCodec->channel_layouts)
		return AV_CH_LAYOUT_STEREO;

	p = pCodec->channel_layouts;
	while (*p) {
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels > best_nb_channels) {
			best_ch_layout = *p;
			best_nb_channels = nb_channels;
		}
		p++;
	}
	return best_ch_layout;
}

int CAVideoTrans::DecodeAudioFrame(AVFrame* poutFrame) {
	int len = 0;
	int error = 0;
	int getframe = 0;
	char szError[256] = { 0 };
	AVFrame* inFrame = NULL;
	AVPacket pkt;

	av_init_packet(&pkt);

	/*Initialize temporary storage for one output frame.*/
	if (!(inFrame = av_frame_alloc())) {
		av_strerror(error, szError, 256);
		m_pLog->Error("av_frame_alloc() [InFrame] error!ErrMsg[%s]", szError);
		return -1;
	}

	/*
	 * Set the frame's parameters, especially its size and format.
	 * av_frame_get_buffer needs this to allocate memory for the
	 * audio samples of the frame.
	 * Default channel layouts based on the number of channels
	 * are assumed for simplicity.
	 */
	poutFrame->channel_layout = m_pAudioEncodeCtx->channel_layout;
	poutFrame->format = m_pAudioEncodeCtx->sample_fmt;
	poutFrame->sample_rate = m_pAudioEncodeCtx->sample_rate;
	poutFrame->nb_samples = m_pAudioEncodeCtx->frame_size;

	if (poutFrame->nb_samples) {
		error = av_frame_get_buffer(poutFrame, 0);
		if (error < 0) {
			av_strerror(error, szError, 256);
			m_pLog->Error("av_frame_alloc() [Tmpframe] error!ErrMsg[%s]",
					szError);
			return error;
		}
	}

	/*
	 * Set the pkt data and size
	 */
	pkt.data = m_DecodecPacket.pAudioData;
	pkt.size = m_InDataBuffer.nFrameLen * m_DecodecPacket.nAudioFrameNums;

	len = avcodec_decode_audio4(m_pAudioDecodeCtx, inFrame, &getframe, &pkt);
	if (len < 0) {
		m_pLog->Error("avcodec_decode_audio4() error!");
		av_free_packet(&pkt);
		return FUNCTION_ERROR;
	}

	/*
	 * convert samples from native format to destination codec format,
	 * using the resampler
	 */
	if (getframe) {
		/* convert to destination format */
		error = swr_convert(m_pAudioSwrCtx, poutFrame->data,
				inFrame->nb_samples, (const uint8_t **) inFrame->extended_data,
				inFrame->nb_samples);
		if (error < 0) {
			av_strerror(error, szError, 256);
			m_pLog->Error("swr_convert() false;Errstr=%s", szError);
			av_free_packet(&pkt);
			return FUNCTION_ERROR;
		}
		poutFrame->nb_samples = inFrame->nb_samples;
	}

	av_frame_free(&inFrame);
	av_free_packet(&pkt);
	return FUNCTION_SUCCESS;
}

int CAVideoTrans::EncodeAudioFrame(AVPacket& packet, AVFrame* poutFrame) {
	int error = 0;
	int frame_size = 0;
	int data_written = 0;
	char szError[256] = { 0 };

	/*
	 * Use the maximum number of possible samples per frame.
	 * If there is less than the maximum possible frame size in the FIFO
	 * buffer use this number. Otherwise, use the maximum possible frame size
	 */
	frame_size = FFMIN(poutFrame->nb_samples, m_pAudioEncodeCtx->frame_size);
	poutFrame->nb_samples = frame_size;

	packet.data = NULL;
	packet.size = 0;

	if ((error = avcodec_encode_audio2(m_pAudioEncodeCtx, &packet, poutFrame,
			&data_written)) < 0) {
		av_strerror(error, szError, 256);
		m_pLog->Error("avcodec_encode_audio2() false;Errstr=%s,WriteData[%d]",
				szError, data_written);
		return FUNCTION_ERROR;
	}
	return FUNCTION_SUCCESS;
}

