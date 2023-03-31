#include "class.h"


using namespace godot;


void godot::videoClass::_register_methods()
{
	register_method("loadFile", &godot::videoClass::loadFile);
	register_method("process", &godot::videoClass::process);
	register_method("processBufferOnly", &godot::videoClass::processBufferOnly);
	register_method("getDimensions", &godot::videoClass::getDimensions);
	register_method("popAudioBuffer", &godot::videoClass::popSampleBuffer);
	register_method("popRawBuffer", &godot::videoClass::popRawBuffer);
	register_method("getAudioInfo", &godot::videoClass::getAudioInfo);
	register_method("getSizeOfNextAudioFrame", &godot::videoClass::getSizeOfNextAudioFrame);
	register_method("getCurAudioTime", &godot::videoClass::getCurAudioTime);
	register_method("getCurVideoTime", &godot::videoClass::getCurVideoTime);
	register_method("getDuration", &godot::videoClass::getDuration);
	register_method("seek", &godot::videoClass::seek);
	register_method("seekPreviousFrame", &godot::videoClass::seekPreviousFrame);
	register_method("dgbPrintPoolSize", &godot::videoClass::dgbPrintPoolSize);
	register_method("getImageBufferSize", &godot::videoClass::getImageBufferSize);
	register_method("getAudioBufferSize", &godot::videoClass::getAudioBufferSize);
	register_method("getAudioFrameBufferSize", &godot::videoClass::getAudioFrameBufferSize);
	register_method("getImageFrameBufferSize", &godot::videoClass::getImageFrameBufferSize);
	register_method("getNumberVideoFrames", &godot::videoClass::getNumberVideoFrames);
	register_method("getVersion",&godot::videoClass::getVersion);
	register_method("getLicense", &godot::videoClass::getLicense);
	register_method("popImageBuffer", &godot::videoClass::popPoolRGB);
	register_method("getPoolSize", &godot::videoClass::getPoolSize);
	register_method("setVideoBufferSize", &godot::videoClass::getPoolSize);
	register_method("clearPoolEntry", &godot::videoClass::clearPoolEntry);
	register_method("close", &godot::videoClass::close);
}



void godot::videoClass::_init()
{
;
}

godot::videoClass::videoClass()
{
}
		
godot::videoClass::~videoClass()
{
	videoClass::close();
}




Dictionary godot::videoClass::loadFile(String path,bool usePool)
{
	
	this->usePool = usePool;
	Dictionary dict;

	formatCtx = avformat_alloc_context();
	int ret = avformat_open_input(&formatCtx, path.alloc_c_string(), NULL, NULL);

	if (ret != 0)
	{
		printError(ret);
		dict["error"] = ret;
		return dict;
	}

	avformat_find_stream_info(formatCtx, NULL);

	AVCodec* codec = nullptr;
	AVCodecParameters* codecParams = nullptr;
	const AVCodec* curCodec;
	
	durationSec = formatCtx->duration / 1000000.0;
	

	for (int i = 0; i < formatCtx->nb_streams; ++i)
	{
		codecParams = formatCtx->streams[i]->codecpar;
		curCodec = avcodec_find_decoder(codecParams->codec_id);

		if (!curCodec)
		{
			continue;
		}

		if (curCodec->type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = StreamInfo(i, curCodec, codecParams);
			videoStream.firstDts = formatCtx->streams[i]->start_time;
			AVRational timeBase = formatCtx->streams[i]->time_base;
			numVideoFrames = formatCtx->streams[i]->nb_frames;

			if (videoStream.firstDts == AV_NOPTS_VALUE)
			{
				videoStream.firstDts = 0;
			}

			ratio = (double)timeBase.num / (double)timeBase.den;
			num = timeBase.num;
			den = timeBase.den;
			width = codecParams->width;
			height = codecParams->height;
			
			if (width < 16) width = 16;
			if (height < 16) height = 16;

			videoFrameSize = av_image_get_buffer_size(videoStream.codecCtx->pix_fmt, width, height, 32);
			imagePool = new SimplePool(Vector2(width, height));
			hasVideo = true;


		}

		if (curCodec->type == AVMEDIA_TYPE_AUDIO)
		{
			AVRational timeBase = formatCtx->streams[i]->time_base;
			audioRatio = (double)timeBase.num / (double)timeBase.den;
			audioStream = StreamInfo(i, curCodec, codecParams);
			audioStream.firstDts = formatCtx->streams[i]->start_time;

			if (audioStream.firstDts == AV_NOPTS_VALUE)
				audioStream.firstDts = 0;

			sampleRate = audioStream.codecCtx->sample_rate;
			channels = audioStream.codecCtx->channels;

			hasAudio = true;
		}


		if (durationSec < 0)
		{
			validDuration = false;
		}

	}

	//avcodec_parameters_free(&codecParams);

	dict["hasAudio"] = hasAudio;
	dict["hasVideo"] = hasVideo;
	dict["videoRatio"] = ratio;
	dict["audioRatio"] = ratio;
	dict["error"] = ret;
	initialized = true;
	return dict;
}

void godot::videoClass::close()
{

	curVideoTime = -1;
	curAudioTime = -1;
	numVideoFrames = 0;
	for (int i = 0; i < videoFrameBuffer.size(); i++)
	{
		//av_frame_free(&videoFrameBuffer[i]);
		av_packet_free(&videoFrameBuffer[i]);
	}

	for (int i = 0; i < audioFrameBuffer.size(); i++)
	{
		//av_frame_free(&audioFrameBuffer[i]);
		av_packet_free(&audioFrameBuffer[i]);
	}

	videoFrameBuffer.clear();
	audioFrameBuffer.clear();

	rawBuffer.clear();
	imageBuffer.clear();

	for (int i = 0; i < audioBuffer.size(); i++)
	{
		delete[] audioBuffer[i].samples;
	}


	/*for (int i = 0; i < rawBuffer.size(); i++)
	{
		delete[] rawBuffer[i][1];
	}*/

	audioBuffer.clear();

	if (imagePool != nullptr)
		imagePool->pool.clear();

	sws_scalar_ctx = nullptr;
	initialized = false;
	gotFirstFrame = false;
	if (data != nullptr)
	{
		delete[] data;
		data = nullptr;
	}
	rgbGodot.resize(0);


	if (hasVideo) avcodec_close(videoStream.codecCtx);
	if (hasAudio) avcodec_close(audioStream.codecCtx);

	if (audioConv != nullptr)
	{

		swr_close(audioConv);
		swr_free(&audioConv);
		audioConv = nullptr;
	}

	videoStream.index = -1;
	audioStream.index = -1;

	hasVideo = false;
	hasAudio = false;



	avformat_close_input(&formatCtx);



}

void godot::videoClass::printError(int err)
{
	char* t = new char[44];
	av_strerror(err, t, 44);

}

int godot::videoClass::readFrame()
{
	int skip = 0;
	AVPacket* av_packet = av_packet_alloc();
	int ret = av_read_frame(formatCtx, av_packet);


	while (ret >= 0)
	{


		if (av_packet->stream_index != videoStream.index && av_packet->stream_index != audioStream.index)
		{

			ret = av_read_frame(formatCtx, av_packet);
			continue;
		}


		if (av_packet->stream_index == videoStream.index)
		{
			videoFrameBuffer.push_back(av_packet);
			return 1;
			//av_packet = av_packet_alloc();
			break;

		}

		if (av_packet->stream_index == audioStream.index)
		{
			audioFrameBuffer.push_back(av_packet);
			av_packet = av_packet_alloc();
			if (!hasVideo) return 1;
		}


		av_packet = av_packet_alloc();
		ret = av_read_frame(formatCtx, av_packet);
	}

	av_packet_unref(av_packet);
	return 1;

}

int godot::videoClass::process()
{
	int ret = 1;
	if (initialized == false)
		return 1;

	if (hasVideo)
		if (imagePool->pool.size() > 200)//memory leak killswitch
			return -1;



	/*if (videoFrameBuffer.size() > 4 && rawBuffer.size() < 4)
	{
		processVideoFrame(videoFrameBuffer.front());
		videoFrameBuffer.pop_front();
	}

	if (audioFrameBuffer.size() > 4 && audioBuffer.size() < 4)
	{
		processAudioFrame(audioFrameBuffer.front());
		audioFrameBuffer.pop_front();
	}*/


	bool condition1 = hasVideo && !videoOver && rawBuffer.size() < imageBufferSize;
	bool condition2 = hasAudio && !videoOver && audioBuffer.size() < audioBufferSize;

	
	

	if (condition1 || condition2)
	{
		ret = readFrame();
		//if (ret == -1) return -1;
	}


	std::thread vThread(&videoClass::procesVideoBuffers, this);
	std::thread aThread(&videoClass::procesAudioBuffers, this);

	vThread.join();
	aThread.join();



	int breakCond = 0;

	while (rawBuffer.size() == 0 && !gotFirstFrame && hasVideo)
	{
		int t = 3;

		readFrame();
		processVideoFrame(videoFrameBuffer.front());
		videoFrameBuffer.pop_front();
		if (breakCond > 50)
		{
			break;
			gotFirstFrame = true;
		}
	}

	


	if (!validDuration && durationSec < getCurVideoTime())
		durationSec = getCurVideoTime();

	//procesVideoBuffers();
	//procesAudioBuffers();

	//if (audioBuffer.size() > 0) curAudioTime = audioBuffer[0].timeStamp;

	return ret;
}

int godot::videoClass::processBufferOnly()
{
	if (videoFrameBuffer.size() > 0 && rawBuffer.size() < imageBufferSize)//change if to while to process whole buffer
	{
		processVideoFrame(videoFrameBuffer.front());
		videoFrameBuffer.pop_front();
	}

	if (rawBuffer.size() > 0) curVideoTime = rawBuffer[0][2];

	if (audioFrameBuffer.size() > 0 && audioBuffer.size() < imageBufferSize)
	{
		processAudioFrame(audioFrameBuffer.front());
		audioFrameBuffer.pop_front();
	}


	if (audioBuffer.size() > 0) curAudioTime = audioBuffer[0].timeStamp;

	return 1;
}


void videoClass::procesVideoBuffers()
{
	if (videoFrameBuffer.size() > 0 && rawBuffer.size() < imageBufferSize)//change if to while to process whole buffer
	{
		processVideoFrame(videoFrameBuffer.front());
		videoFrameBuffer.pop_front();

		
	}

	if (rawBuffer.size() > 0)
	{
		curVideoTime = rawBuffer[0][2];
		gotFirstFrame = true;
	}
	//else
	//	curVideoTime = 0;
}


void videoClass::procesAudioBuffers()
{
	if (audioFrameBuffer.size() > 0 && audioBuffer.size() < imageBufferSize)
	{
		processAudioFrame(audioFrameBuffer.front());
		audioFrameBuffer.pop_front();

		

	}
	if (audioBuffer.size() > 0) curAudioTime = audioBuffer[0].timeStamp;
}

String videoClass::getVersion()
{
	return String(av_version_info());
}

String videoClass::getLicense()
{
	return String(avutil_license());
}


double godot::videoClass::getCurVideoTime()
{
	return curVideoTime;
}

double godot::videoClass::getCurAudioTime()
{
	return curAudioTime;
}

int godot::videoClass::getNumberVideoFrames()
{
	return numVideoFrames;
}

void godot::videoClass::setVideoBufferSize(int size)
{
	imageBufferSize = size;
}


audioFrame godot::videoClass::fetchAudioBuffer()
{
	if (audioBuffer.size() > 0)
	{
		audioFrame ret = audioBuffer[0];
		audioBuffer.pop_front();
		curAudioTime = ret.timeStamp;
		return ret;
	}

	return audioFrame(0, nullptr,-1);
}

double godot::videoClass::getDuration()
{
	return durationSec;
}



Array godot::videoClass::getAudioInfo()
{
	Array ret;
	ret.append(hasAudio);
	ret.append(channels);
	ret.append(sampleRate);
	return ret;
}


Array godot::videoClass::popPoolRGB()
{
	PoolByteArray img;
	//godot_dictionary dict;
	Array arr;
	if (imageBuffer.size() > 0)
	{
		img = imageBuffer[0].img;
		arr.append(imageBuffer[0].img);
		arr.append(imageBuffer[0].poolId);
		curVideoTime = imageBuffer[0].timeStamp;
		imageBuffer.pop_front();


		if (imageBuffer.size() > 0)
		{
			curVideoTime = imageBuffer[0].timeStamp;
		}
	}
	return arr;
}


PoolByteArray godot::videoClass::popRawBuffer(bool updateTime)
{
	//Dictionary dict = rawBuffer[0];
	Array arr = rawBuffer[0];
	PoolByteArray r = arr[0];//dict["data"];
	
	curVideoTime = rawBuffer[0][2];

	if (usePool)
	{
		imagePool->free(rawBuffer[0][1]);
		
	}

	rawBuffer.pop_front();
	

	

	if (rawBuffer.size() > 0 && updateTime)
		curVideoTime = rawBuffer[0][2];

	return r;
}


void godot::videoClass::clearAudio()
{
	audioBuffer.clear();
	audioFrameBuffer.clear();
	for (int i = 0; i < audioBuffer.size(); i++)
	{
		delete[] audioBuffer[i].samples;
	}


	rawBuffer.clear();
	
}

PoolByteArray godot::videoClass::rgbArrToByte(unsigned char* data)
{
	PoolByteArray ret;

	int size = width * height * 3;

	ret.resize(size);

	PoolByteArray::Write w = ret.write();
	uint8_t* p = w.ptr();
	memcpy(p, data, size);


	return ret;
}

Array godot::videoClass::rgbAToBytePool(unsigned char* data)
{
	Array r;
	int size = width * height * 3;

	PoolEntry* pe = imagePool->fetch(size);



	PoolByteArray::Write w = pe->data.write();

	uint8_t* p = w.ptr();
	memcpy(p, data, size);

	r.append(pe->data);
	r.append(pe->id);

	return r;
}





int godot::videoClass::getImageBufferSize()
{
	return rawBuffer.size();
	
}

int godot::videoClass::getImageFrameBufferSize()
{
	return videoFrameBuffer.size();
}

void godot::videoClass::clearPoolEntry(int id)
{
	imagePool->free(id);
}

int godot::videoClass::getAudioBufferSize()
{
	return audioBuffer.size();
}

int godot::videoClass::getAudioFrameBufferSize()
{
	return audioFrameBuffer.size();
}

int godot::videoClass::getPoolSize()
{
	return imagePool->pool.size();
}



int godot::videoClass::getSizeOfNextAudioFrame()
{
	if (audioBuffer.size() == 0)
		return 2147483647;
	return audioBuffer[0].size;
}

PoolVector2Array godot::videoClass::popSampleBuffer()
{
	PoolVector2Array sampleGodot;
	

	if (audioBuffer.size() > 0)
	{

		audioFrame audioFrame = audioBuffer[0];
		audioBuffer.pop_front();

		int size = (audioFrame.size/2)/2;
		sampleGodot.resize(size);

		for (int i = 0; i < size; i++)
		{
			unsigned short u1 = audioFrame.samples[(i*4)+0] | (audioFrame.samples[(i*4)+1] << 8);
			u1 = (u1 + 32768) & 0xffff;
			float s1 = float(u1 - 32768) / 32768.0;
			
			unsigned short u2 = audioFrame.samples[(i * 4) + 2] | (audioFrame.samples[(i * 4) + 3] << 8);
			u2 = (u2 + 32768) & 0xffff;
			float s2 = float(u2 - 32768) / 32768.0;

			Vector2 vec = Vector2(s1, s2);
			sampleGodot.set(i,vec);


		}


		if (audioBuffer.size() > 0)
		{
			curAudioTime = audioBuffer[0].timeStamp;
		}


		//delete[] audioFrame.samples;
		//av_free(audioFrame.samples);
		 
	}
	return sampleGodot;
}

int godot::videoClass::processVideoFrame(AVPacket* pkt,bool justGetTimeStamp,double targetTime)
{
	int e = 0;
	int gotFrame = -1;

	if (avcodec_send_packet(videoStream.codecCtx, pkt) >= 0)
	{
		AVFrame* frame = av_frame_alloc();
		e = avcodec_receive_frame(videoStream.codecCtx, frame);

		while (e >= 0)
		{
			gotFrame = 1;

			AVCodecParameters* codecParam = videoStream.codecParams;
			AVPixelFormat pixFormat = videoStream.codecCtx->pix_fmt;


			double timestamp = (frame->pts - videoStream.firstDts) * ratio;


			if (frame->pts < 0 && frame->pkt_dts > 0)
			{
				timestamp = (frame->pkt_dts - videoStream.firstDts) * ratio;
			}


			if (rawBuffer.size() == 0)
			{
				curVideoTime = timestamp;
			}

			Array arr;

			if (frame->pts == AV_NOPTS_VALUE)
			{
				curVideoTime = 0;
				timestamp = 0;
			}



			if (justGetTimeStamp && curVideoTime != targetTime)
			{
				e = avcodec_receive_frame(videoStream.codecCtx, frame);
				continue;
			}

			if (sws_scalar_ctx == nullptr)
				sws_scalar_ctx = sws_getContext(width, height, pixFormat, width, height, AVPixelFormat::AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);

			if (sws_scalar_ctx == NULL)
				return 0;

			int size = width * height * 4;//this should be 4 but there was a certain video that crashed and when changed to 5 it didn't. 
			//int size = videoFrameSize;
			if (data == nullptr)
				data = new unsigned char[size];

			unsigned char* dest[4] = { data,NULL,NULL,NULL };

			int dest_linesize[4] = { width * 3,0,0,0 };

			sws_scale(sws_scalar_ctx, frame->data, frame->linesize, 0, height, dest, dest_linesize);//converting YUV to RGBA
			
			
			if (!usePool)
			{
				arr.push_back(rgbArrToByte(data));
				arr.push_back(0);
				arr.push_back(timestamp);
			}
			else
			{

				Array r = rgbAToBytePool(data);
				arr.push_back(r[0]);
				arr.push_back(r[1]);
				arr.push_back(timestamp);

			}
			
			


			//arr.append(timestamp);

			rawBuffer.push_back(arr);
			//curVideoTime = rawBuffer[0][2];
			int t = 3;
			e = avcodec_receive_frame(videoStream.codecCtx, frame);

			
		}
		av_frame_free(&frame);
	}
	av_packet_unref(pkt);
	av_packet_free(&pkt);

	
	return gotFrame;
}





void godot::videoClass::processAudioFrame(AVPacket* pkt,bool justGetTimeStamp)
{
	if (avcodec_send_packet(audioStream.codecCtx, pkt) >= 0)
	{
		AVFrame* frame = av_frame_alloc();

		static double sum = 0;
		while (avcodec_receive_frame(audioStream.codecCtx, frame) >= 0)
		{


			double timestamp = (frame->pts - audioStream.firstDts) * audioRatio;


			//if (timestamp < 0)
			//	timestamp = frame->best_effort_timestamp;

			if (timestamp < 0)
				timestamp = curVideoTime;


			if (justGetTimeStamp)
			{
				curAudioTime = timestamp;
				av_frame_free(&frame);
				av_packet_free(&pkt);// I think it's okay to do this
				return;
			}

			AVSampleFormat sampleForamt = audioStream.codecCtx->sample_fmt;

			int out_linesize;
			int dataSize = av_samples_get_buffer_size(&out_linesize, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);

			if (dataSize < 0)
			{
				printError(dataSize);
				av_frame_free(&frame);
				av_packet_free(&pkt);// I think it's okay to do this
				return;
			}

			uint8_t* audio_out_buffer = new uint8_t[dataSize];
			//uint8_t* audio_out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
			if (audioConv == nullptr)
			{
				audioConv = swr_alloc();

				audioConv = swr_alloc_set_opts(audioConv, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, frame->sample_rate, av_get_default_channel_layout(channels), sampleForamt, sampleRate, 1, NULL);
				int err = swr_init(audioConv);
				if (err < 0)
					printError(err);
				int t = 3;


			}

			int dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(audioConv, frame->sample_rate) + frame->nb_samples, sampleRate, AV_CH_LAYOUT_STEREO, AV_ROUND_INF);
			int size2 = swr_convert(audioConv, &audio_out_buffer, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);

			if (size2 < 0)
			{
				continue;
			}

			int remaining = dataSize - size2;


			


			audioBuffer.push_back(audioFrame(dataSize, audio_out_buffer, timestamp));
		}

		av_frame_free(&frame);
	}

	av_packet_free(&pkt);

}



Vector2 godot::videoClass::getDimensions()
{

	return Vector2(width,height);
}


StreamInfo::StreamInfo()
{
	;
}

StreamInfo::StreamInfo(int index,const AVCodec* codec, AVCodecParameters* codecParams)
{
	this->index = index;
	this->codec = codec;
	this->codecParams = codecParams;
	this->codecCtx = avcodec_alloc_context3(codec);


	if (this->codec->id != AV_CODEC_ID_PNG && this->codec->id != AV_CODEC_ID_WEBP && this->codec->id != AV_CODEC_ID_PSD)
	{

		this->codecCtx->thread_count = 0;

		if (codec->capabilities | AV_CODEC_CAP_FRAME_THREADS)
			this->codecCtx->thread_type = FF_THREAD_FRAME;
		else if (codec->capabilities | AV_CODEC_CAP_SLICE_THREADS)
			this->codecCtx->thread_type = FF_THREAD_SLICE;
		else
			this->codecCtx->thread_count = 1;
	}

	avcodec_parameters_to_context(codecCtx, codecParams);
	avcodec_open2(codecCtx, codec, NULL);
}

rgbFrame::rgbFrame(double timeStamp, unsigned char* rgb,int poolId)
{
	this->timeStamp = timeStamp;
	this->rgb = rgb;
	this->poolId = poolId;
}

audioFrame::audioFrame(int size, uint8_t* samples,double timeStamp)
{
	this->size = size;
	this->samples = samples;
	this->timeStamp = timeStamp;
}

int videoClass::seek(double sec)
{
	//int videoTime = av_rescale(sec, num, den);
	double videoTimeDouble = sec / ratio;
	int flag = 0;

	double bestRatio = 0;
	bool forward = true;

	if (sec < curVideoTime && hasVideo)
	{
		flag = AVSEEK_FLAG_BACKWARD;
		forward = false;
	}

	else if (sec < curAudioTime && (hasAudio && !hasVideo) )
	{
		flag = AVSEEK_FLAG_BACKWARD;
		forward = false;
	}

	int audioErr = -1;
	int videoErr = -1;


	if (hasVideo)
	{
		//if (hasAudio)
		//	avformat_seek_file(formatCtx, audioStream.index, INT64_MIN, videoTimeDouble, INT64_MAX, flag);

		videoErr = avformat_seek_file(formatCtx, videoStream.index, INT64_MIN, videoTimeDouble, INT64_MAX, flag);
		bestRatio = ratio;

		if (videoErr < 0)
		{
			audioErr = avformat_seek_file(formatCtx, audioStream.index, INT64_MIN, sec / audioRatio, INT64_MAX, flag);
			bestRatio = audioRatio;

			if (sec < curAudioTime)
			{
				flag = AVSEEK_FLAG_BACKWARD;
				forward = false;
			}
			else forward = true;

		}
	}
		
	if (hasAudio && videoErr < 0)//only seek on one stream seeking to audio and video doesn't work.
	{
		audioErr = avformat_seek_file(formatCtx, audioStream.index, INT64_MIN, sec / audioRatio, INT64_MAX, flag);
		bestRatio = audioRatio;
	}
			
		


	
	//avformat_flush(formatCtx);
	//if (hasAudio) avcodec_flush_buffers(audioStream.codecCtx);
	//if (hasVideo) avcodec_flush_buffers(videoStream.codecCtx);
	
		
	//avcodec_flush_buffers(videoStream.codecCtx);
	audioFrameBuffer.clear();
	videoFrameBuffer.clear();


	for (int i = 0; i < audioBuffer.size(); i++)
	{
		delete[] audioBuffer[i].samples;
	}



	rawBuffer.clear();
	audioBuffer.clear();

	int breakCond = 0;
	bool goneBack = false;

	while (readFrame())
	{

		breakCond += 1;

		if (breakCond > 300)
			return 0;


		double curTime = 0;
		
		if (hasVideo && videoErr >= 0) curTime = curVideoTime;
		else if (hasAudio)curTime = curAudioTime;


		if (videoFrameBuffer.size() == 0 && hasVideo && videoErr >= 0)
		{
			clearAudio();
			return 0;
		}

		if (hasVideo && videoErr >= 0)
			processVideoFrame(videoFrameBuffer.front(), true);
		else if (hasAudio)
			if (audioFrameBuffer.size() > 0)
			{
				if (audioSeekSubFunction(forward, sec, bestRatio,goneBack) == 1)
					return 0;
				
				//processAudioFrame(audioFrameBuffer.front(), true);
			}



		if (videoFrameBuffer.size() > 0)
			videoFrameBuffer.pop_front();

		if (audioFrameBuffer.size() > 0)
			audioFrameBuffer.pop_front();

		if (rawBuffer.size() > 0)
			rawBuffer.clear();

		if (forward)
		{
			if (curTime >= sec)
			{
				if (videoErr >= 0)
					clearAudio();
				return 0;
			}
		}
		else
		{

			if (sec < 2 * bestRatio)
				sec = 2 * bestRatio;	


			if (curTime < sec)
				goneBack = true;

				if(goneBack == true && curTime >= sec)
				{
					if (videoErr >= 0)
						clearAudio();
					return 0;
				}

		}

		

	}

	if (videoErr >= 0)
		clearAudio();
	
	for (int i = 0; i < audioBuffer.size(); i++)
	{
		delete[] audioBuffer[i].samples;
	}

	return 0;

}


bool videoClass::audioSeekSubFunction(bool forward,double sec,double bestRatio,bool &goneBack)
{
	while (audioFrameBuffer.size() > 0)
	{
		processAudioFrame(audioFrameBuffer.front(), true);
		audioFrameBuffer.pop_front();


		if (forward)
		{
			if (curAudioTime >= sec)
			{
				return true;
			}
		}
		else
		{

			if (sec < 2 * bestRatio)
				sec = 2 * bestRatio;


			if (curAudioTime < sec)
				goneBack = true;

			if (goneBack == true && curAudioTime >= sec)
			{
				return true;
			}

		}
	}

	return false;
}

int godot::videoClass::seekPreviousFrame(double sec)
{

	if (sec <= 0)
		return 0;

	if (!hasVideo) return 0;

	double pre = curVideoTime;
	double post = curVideoTime;

	videoFrameBuffer.clear();
	rawBuffer.clear();
	clearAudio();

	avcodec_flush_buffers(videoStream.codecCtx);
	
	if (hasAudio) avcodec_flush_buffers(audioStream.codecCtx);
	if (hasVideo) avcodec_flush_buffers(videoStream.codecCtx);

	double sseek = sec - 3;
	if (sseek < 0)
		sseek = 0;
	

	avformat_seek_file(formatCtx, videoStream.index, INT64_MIN, sseek, INT64_MAX, AVSEEK_FLAG_BACKWARD);



	bool goneBack = false;

	while (readFrame())
	{
		if (curVideoTime < post)
			goneBack = true;

		double pVideoTime = curVideoTime;
		int e = processVideoFrame(videoFrameBuffer.front(), true);
			
		videoFrameBuffer.pop_front();

		if (curVideoTime >= sec && goneBack)
			break;
			
		pre = curVideoTime;
	

		
	}


	avformat_seek_file(formatCtx, videoStream.index, INT64_MIN, sec - 3, INT64_MAX, AVSEEK_FLAG_BACKWARD);
	
	goneBack = false;

	while (readFrame())
	{
		if (curVideoTime < post)
			goneBack = true;

		double pVideoTime = curVideoTime;
		int e = processVideoFrame(videoFrameBuffer.front(), true,pre);
		videoFrameBuffer.pop_front();

		
		if (rawBuffer.size() >= 1)
			break;
		



	}


	return 0;
}

void godot::videoClass::dgbPrintPoolSize()
{
	//std::cout << imagePool->pool.size();
}
