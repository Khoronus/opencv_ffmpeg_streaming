#include "streamer/streamer.hpp"

#include <chrono>
#include <thread>
#include <string>
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <cstdlib>
//#include <unistd.h>
#include <chrono>

#define usethis
#ifdef usethis

using namespace streamer;


class MovingAverage
{
    int size;
    int pos;
    bool crossed;
    std::vector<double> v;

public:
    explicit MovingAverage(int sz)
    {
        size = sz;
        v.resize(size);
        pos = 0;
        crossed = false;
    }

    void add_value(double value)
    {
        v[pos] = value;
        pos++;
        if(pos == size) {
            pos = 0;
            crossed = true;
        }
    }

    double get_average()
    {
        double avg = 0.0;
        int last = crossed ? size : pos;
        int k=0;
        for(k=0;k<last;k++) {
            avg += v[k];
        }
        return avg / (double)last;
    }
};


static void add_delay(size_t streamed_frames, size_t fps, double elapsed, double avg_frame_time)
{
    //compute min number of frames that should have been streamed based on fps and elapsed
    double dfps = fps;
    size_t min_streamed = (size_t) (dfps*elapsed);
    size_t min_plus_margin = min_streamed + 2;

    if(streamed_frames > min_plus_margin) {
        size_t excess = streamed_frames - min_plus_margin;
        double dexcess = excess;

        //add a delay ~ excess*processing_time
//#define SHOW_DELAY
#ifdef SHOW_DELAY
        double delay = dexcess*avg_frame_time*1000000.0;
        printf("frame %07lu adding delay %.4f\n", streamed_frames, delay);
        printf("avg fps = %.2f\n", streamed_frames/elapsed);
#endif
		//usleep(dexcess*avg_frame_time*1000000.0);
		std::this_thread::sleep_for(std::chrono::microseconds((int)(dexcess*avg_frame_time*1000000.0)));
    }
}

void process_frame(const cv::Mat &in, cv::Mat &out)
{
    in.copyTo(out);
}




int test_streaming()
{
	av_register_all();
	avdevice_register_all();
	avcodec_register_all();
	avformat_network_init();

	//const char  *filenameSrc = "udp://127.0.0.1:1234";
	const char  *filenameSrc = "bv.sdp";

	AVCodecContext  *pCodecCtx;
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	pFormatCtx->protocol_whitelist = "file,udp,rtp";

	AVCodec * pCodec;
	AVFrame *pFrame, *pFrameRGB;

	if (avformat_open_input(&pFormatCtx, filenameSrc, NULL, NULL) != 0) return -12;
	// https://lists.ffmpeg.org/pipermail/ffmpeg-cvslog/2011-August/039866.html
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)   return -13;
	av_dump_format(pFormatCtx, 0, filenameSrc, 0);
	int videoStream = 1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->coder_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			break;
		}
	}

	if (videoStream == -1) return -14;
	pCodecCtx = pFormatCtx->streams[videoStream]->codec;

	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) return -15; //codec not found

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) return -16;

	// https://stackoverflow.com/questions/24057248/ffmpeg-undefined-references-to-av-frame-alloc
	pFrame = av_frame_alloc();
	pFrameRGB = av_frame_alloc();

	uint8_t *buffer;
	int numBytes;

	AVPixelFormat  pFormat = AV_PIX_FMT_BGR24;
	numBytes = avpicture_get_size(pFormat, pCodecCtx->width, pCodecCtx->height); //AV_PIX_FMT_RGB24
	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	avpicture_fill((AVPicture *)pFrameRGB, buffer, pFormat, pCodecCtx->width, pCodecCtx->height);

	int res;
	int frameFinished;
	AVPacket packet;
	while (res = av_read_frame(pFormatCtx, &packet) >= 0)
	{

		if (packet.stream_index == videoStream) {

			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			if (frameFinished) {

				struct SwsContext * img_convert_ctx;
				img_convert_ctx = sws_getCachedContext(NULL, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);
				sws_scale(img_convert_ctx, ((AVPicture*)pFrame)->data, ((AVPicture*)pFrame)->linesize, 0, pCodecCtx->height, ((AVPicture *)pFrameRGB)->data, ((AVPicture *)pFrameRGB)->linesize);

				cv::Mat img(pFrame->height, pFrame->width, CV_8UC3, pFrameRGB->data[0]); //dst->data[0]);
				cv::imshow("display", img);
				cvWaitKey(1);

				av_free_packet(&packet);
				sws_freeContext(img_convert_ctx);

			}

		}

	}

	av_free_packet(&packet);
	avcodec_close(pCodecCtx);
	av_free(pFrame);
	av_free(pFrameRGB);
	avformat_close_input(&pFormatCtx);

	return 0;
}


int main(int argc, char *argv[])
{
	std::cout << test_streaming() << std::endl;
	return 0;
    if(argc != 2) {
        printf("must provide one command argument with the video file or stream to open\n");
        return 1;
    }
    std::string video_fname;
    video_fname = std::string(argv[1]);
    cv::VideoCapture video_capture;
    bool from_camera = false;
    if(video_fname == "0") {
        video_capture = cv::VideoCapture(0);
        from_camera = true;
    } else {
        video_capture=  cv::VideoCapture(video_fname, cv::CAP_FFMPEG);
    }


    if(!video_capture.isOpened()) {
        fprintf(stderr, "could not open video %s\n", video_fname.c_str());
        video_capture.release();
        return 1;
    }

    int cap_frame_width = video_capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int cap_frame_height = video_capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    int cap_fps = video_capture.get(cv::CAP_PROP_FPS);
    printf("video info w = %d, h = %d, fps = %d\n", cap_frame_width, cap_frame_height, cap_fps);

	int stream_fps = 30;// cap_fps;

    int bitrate = 500000;
    Streamer streamer;
    StreamerConfig streamer_config(cap_frame_width, cap_frame_height,
                                   640, 360,
		stream_fps, bitrate, "high444", "rtp://127.0.0.1:1234");// "rtmp://localhost:8889/live/app"); //"rtmp://localhost/live/mystream"); // rtsp://127.0.0.1:8554/live.sdp

	std::cout << "Log" << std::endl;
    streamer.enable_av_debug_log();

	std::cout << "Init" << std::endl;
	streamer.init(streamer_config);

    size_t streamed_frames = 0;

    std::chrono::high_resolution_clock clk;
    std::chrono::high_resolution_clock::time_point time_start = clk.now();
    std::chrono::high_resolution_clock::time_point time_stop = time_start;
    std::chrono::high_resolution_clock::time_point time_prev = time_start;

    MovingAverage moving_average(10);
    double avg_frame_time;

    cv::Mat read_frame;
    cv::Mat proc_frame;
	std::cout << "Capture" << std::endl;

    bool ok = video_capture.read(read_frame);
	std::cout << "ok: " << ok << std::endl;

    std::chrono::duration<double> elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_start);
    std::chrono::duration<double> frame_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_prev);

    while(ok) {
		std::cout << "process_frame" << std::endl;
        process_frame(read_frame, proc_frame);
        if(!from_camera) {
            streamer.stream_frame(proc_frame);
        } else {
			streamer.stream_frame(proc_frame, 30);// frame_time.count()*streamer.inv_stream_timebase);
        }

        time_stop = clk.now();
        elapsed_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_start);
        frame_time = std::chrono::duration_cast<std::chrono::duration<double>>(time_stop - time_prev);

        if(!from_camera) {
            streamed_frames++;
            moving_average.add_value(frame_time.count());
            avg_frame_time = moving_average.get_average();
            add_delay(streamed_frames, stream_fps, elapsed_time.count(), avg_frame_time);
        }

        ok = video_capture.read(read_frame);
        time_prev = time_stop;
		std::cout << "capture: " << ok << std::endl;
    }
    video_capture.release();

    return 0;
}

#endif // usethis

#ifdef usepipe // usethis : pipeline and rtp (plenty of errors)

/** I was testing the possibility to get the result from the PIPE or directly from the video streaming.
    The result was poor.
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}


#include <windows.h>



int testpipe(void)
{
	HANDLE hPipe;
	size_t size = 1600 * 1600 * 3;
	char* buffer = new char(size);
	DWORD dwRead;


	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\my_pipe2"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		0,
		0,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, size, &dwRead, NULL) != FALSE)
			{
				std::cout << "received: " << dwRead << " bytes" << std::endl;
			}
		}

		DisconnectNamedPipe(hPipe);
	}

	return 0;
}

int main(int argc, char** argv) {

	// In order to use on windows
	// create an environment variable with whitelist of the possible protocol
	// key: OPENCV_FFMPEG_CAPTURE_OPTIONS
	// value: protocol_whitelist;file,rtp,udp
	// link: https://stackoverflow.com/questions/55456870/emgucv-opencv-whitelist-rtp-protocol
	{
		cv::VideoCapture vc("bv.sdp");
		if (vc.isOpened()) {
			while (true) {
				cv::Mat m;
				vc >> m;
				if (m.empty()) {
					std::cout << "empty" << std::endl;
					continue;
				}
				cv::resize(m, m, cv::Size(600, 600));
				cv::imshow("m", m);
				cv::waitKey(1);
			}
		}
		return 0;
	}


	//{
	//	HANDLE hPipe;
	//	char* buffer = new char(1600 * 1600 * 3);
	//	DWORD dwRead;


	//	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\my_pipe2"),
	//		PIPE_ACCESS_INBOUND,
	//		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
	//		1,
	//		0,
	//		0,
	//		NMPWAIT_USE_DEFAULT_WAIT,
	//		NULL);

	//	cv::VideoCapture vc("\\\\.\\pipe\\my_pipe2");
	//	if (vc.isOpened()) {
	//		while (true) {
	//			cv::Mat m;
	//			vc >> m;
	//			if (m.empty()) {
	//				std::cout << "empty" << std::endl;
	//			}
	//			cv::imshow("m", m);
	//			cv::waitKey(1);
	//		}
	//	}
	//	return 0;
	//}

	//testpipe();
	//return 0;

	// Open the initial context variables that are needed
	SwsContext *img_convert_ctx;
	AVFormatContext* format_ctx = avformat_alloc_context();
	AVCodecContext* codec_ctx = NULL;
	int video_stream_index;

	// Register everything
	av_register_all();
	avformat_network_init();

	//open RTSP
	if (avformat_open_input(&format_ctx, "udp://127.0.0.1:1234",
		NULL, NULL) != 0) {
		std::cout << "here" << std::endl;
		return EXIT_FAILURE;
	}

	if (avformat_find_stream_info(format_ctx, NULL) < 0) {
		return EXIT_FAILURE;
	}

	//search video stream
	for (int i = 0; i < format_ctx->nb_streams; i++) {
		if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			video_stream_index = i;
	}

	AVPacket packet;
	av_init_packet(&packet);

	//open output file
	AVFormatContext* output_ctx = avformat_alloc_context();

	AVStream* stream = NULL;
	int cnt = 0;

	//start reading packets from stream and write them to file
	av_read_play(format_ctx);    //play RTSP

	// Get the codec
	AVCodec *codec = NULL;
	codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
	if (!codec) {
		exit(1);
	}

	// Add this to allocate the context by codec
	codec_ctx = avcodec_alloc_context3(codec);

	avcodec_get_context_defaults3(codec_ctx, codec);
	avcodec_copy_context(codec_ctx, format_ctx->streams[video_stream_index]->codec);
	std::ofstream output_file;

	if (avcodec_open2(codec_ctx, codec, NULL) < 0)
		exit(1);

	img_convert_ctx = sws_getContext(codec_ctx->width, codec_ctx->height,
		codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
		SWS_BICUBIC, NULL, NULL, NULL);

	int size = avpicture_get_size(AV_PIX_FMT_YUV420P, codec_ctx->width,
		codec_ctx->height);
	uint8_t* picture_buffer = (uint8_t*)(av_malloc(size));
	AVFrame* picture = av_frame_alloc();
	AVFrame* picture_rgb = av_frame_alloc();
	int size2 = avpicture_get_size(AV_PIX_FMT_RGB24, codec_ctx->width,
		codec_ctx->height);
	uint8_t* picture_buffer_2 = (uint8_t*)(av_malloc(size2));
	avpicture_fill((AVPicture *)picture, picture_buffer, AV_PIX_FMT_YUV420P,
		codec_ctx->width, codec_ctx->height);
	avpicture_fill((AVPicture *)picture_rgb, picture_buffer_2, AV_PIX_FMT_RGB24,
		codec_ctx->width, codec_ctx->height);

	while (av_read_frame(format_ctx, &packet) >= 0 && cnt < 1000) { //read ~ 1000 frames

		std::cout << "1 Frame: " << cnt << std::endl;
		if (packet.stream_index == video_stream_index) {    //packet is video
			std::cout << "2 Is Video" << std::endl;
			if (stream == NULL) {    //create stream in file
				std::cout << "3 create stream" << std::endl;
				stream = avformat_new_stream(output_ctx,
					format_ctx->streams[video_stream_index]->codec->codec);
				avcodec_copy_context(stream->codec,
					format_ctx->streams[video_stream_index]->codec);
				stream->sample_aspect_ratio =
					format_ctx->streams[video_stream_index]->codec->sample_aspect_ratio;
			}
			int check = 0;
			packet.stream_index = stream->id;
			std::cout << "4 decoding" << std::endl;
			int result = avcodec_decode_video2(codec_ctx, picture, &check, &packet);
			std::cout << "Bytes decoded " << result << " check " << check
				<< std::endl;
			//if (cnt > 100)    //cnt < 0)
			{
				sws_scale(img_convert_ctx, picture->data, picture->linesize, 0,
					codec_ctx->height, picture_rgb->data, picture_rgb->linesize);
				cv::Mat img(codec_ctx->height, codec_ctx->width, CV_8UC3, picture_rgb->data[0]);
				cv::imshow("img", img);
				cv::waitKey(0);
				//std::stringstream file_name;
				//file_name << "test" << cnt << ".ppm";
				//output_file.open(file_name.str().c_str());
				//output_file << "P3 " << codec_ctx->width << " " << codec_ctx->height
				//	<< " 255\n";
				//for (int y = 0; y < codec_ctx->height; y++) {
				//	for (int x = 0; x < codec_ctx->width * 3; x++)
				//		output_file
				//		<< (int)(picture_rgb->data[0]
				//			+ y * picture_rgb->linesize[0])[x] << " ";
				//}
				//output_file.close();
			}
			cnt++;
		}
		av_free_packet(&packet);
		av_init_packet(&packet);
	}
	av_free(picture);
	av_free(picture_rgb);
	av_free(picture_buffer);
	av_free(picture_buffer_2);

	av_read_pause(format_ctx);
	avio_close(output_ctx->pb);
	avformat_free_context(output_ctx);

	return (EXIT_SUCCESS);
}

#endif // usepipe