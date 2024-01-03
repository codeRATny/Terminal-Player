#ifndef _TERMINAL_PLAYER_H_
#define _TERMINAL_PLAYER_H_

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "Colors.h"
#include "TerminalUtils.h"

extern AVFormatContext *input_ctx;
extern AVCodecContext *codec_ctx;
extern AVPacket *pack;
extern AVFrame *frame;
extern AVFrame *resized_frame;
extern int video_stream_idx;
extern struct SwsContext *swsctx;
extern TerminalResolution resol;
extern TerminalCanvas *canv;

void init_video();
void init_decoder();
void init_display();
void init_frames();
void init_sws();

void set_player_url(char *url);
void init_player();

void reinit_player(TerminalResolution res);

void deinit_player();

#endif