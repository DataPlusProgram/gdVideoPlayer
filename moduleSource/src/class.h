#pragma once
#ifndef CLASS_HPP
#define CLASS_HPP

#include <Godot.hpp>
#include <Node.hpp>

#include <string>
#include<deque>
#include<Image.hpp>
#include"ImageFrame.h"
#include"simplePool.h"
#include<thread>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <inttypes.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "libavutil/imgutils.h"
}




#define MAX_AUDIO_FRAME_SIZE 192000


struct rgbFrame
{
	int poolId;
	double timeStamp;
	unsigned char* rgb;
	rgbFrame(double timeStamp, unsigned char* rgb,int poolId);
};



struct audioFrame
{
	double timeStamp;
	int size;
	uint8_t* samples;
	audioFrame(int size, uint8_t* samples,double timeStamp);
};



struct StreamInfo
{
	int index = -1;
	const AVCodec* codec;
	AVCodecParameters* codecParams;
	AVCodecContext* codecCtx;
	int64_t firstDts;
	StreamInfo();
	StreamInfo(int index,const AVCodec* codec, AVCodecParameters* codecParams);

};

namespace godot
{

class videoClass : public Node 
{
	GODOT_CLASS(videoClass, Node)

	public:
		videoClass();
		~videoClass();

		Dictionary loadFile(String path,bool useBuffer = false);
		
		static void _register_methods();
		void _init();

		StreamInfo videoStream, audioStream;
		AVFormatContext* formatCtx;
		void close();
		void printError(int);
		int readFrame();
		int width, height;
		int videoFrameSize;
		bool initialized = false;
		bool gotFirstFrame = false;
		bool hasAudio = false;
		bool hasVideo = false;
		int sampleRate, channels;
		double latestTime = 0;
		int imageBufferSize = 10;
		int audioBufferSize = 2;
		int numVideoFrames = 0;
		PoolByteArray rgbGodot;
		std::deque<AVPacket*> videoFrameBuffer;
		std::deque<AVPacket*> audioFrameBuffer;
		std::deque<audioFrame> audioBuffer;
		SimplePool* imagePool = nullptr;
		Array getAudioInfo();
		SwsContext* sws_scalar_ctx = nullptr;
		unsigned char* data = nullptr;
		Array popPoolRGB();
		PoolByteArray popRawBuffer(bool updateTime = true);
		int getImageBufferSize();
		void clearPoolEntry(int id);
		int getPoolSize();
		int getAudioBufferSize();
		int getAudioFrameBufferSize();
		int getImageFrameBufferSize();
		int getSizeOfNextAudioFrame();
		void setVideoBufferSize(int size);
		PoolVector2Array popSampleBuffer();
		int processVideoFrame(AVPacket* pkt, bool justGetTimeStamp = false,double targetTime = -1);
		void processAudioFrame(AVPacket* pkt, bool justGetTimeStamp = false);


		void procesVideoBuffers();
		void procesAudioBuffers();
		bool usePool = false;
		void clearAudio();
		PoolByteArray rgbArrToByte(unsigned char* data);
		Array rgbAToBytePool(unsigned char* data);
		
		std::deque<ImageFrame> imageBuffer;
		std::deque<Array> rawBuffer;
		Vector2 getDimensions();
		audioFrame fetchAudioBuffer(); 
		double getDuration();
		

		int seek(double msec);
		int seekPreviousFrame(double msec);
		bool audioSeekSubFunction(bool forward,double sec,double bestRatio,bool& goneBack);
		void dgbPrintPoolSize();

		SwrContext* audioConv = nullptr;
		double durationSec, ratio, audioRatio;
		int num, den;
		bool validDuration = true;
		int process();
		int processBufferOnly();
		bool videoOver = false;
		String getVersion();
		String getLicense();
		double getCurVideoTime();
		double getCurAudioTime();
		int getNumberVideoFrames();
		double curVideoTime =99999;
		double curAudioTime = 99999;
		
		
};


}

#endif