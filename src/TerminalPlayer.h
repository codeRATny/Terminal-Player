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
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#include "Colors.h"
#include "TerminalUtils.h"

typedef struct TaskParams
{
    size_t line_offset;
    size_t line_count;

    size_t canv_idx;

} TaskParams;

extern TaskParams *task_params;
extern TerminalCanvas **painter_canvases;
extern AVFormatContext *input_ctx;
extern AVCodecContext *codec_ctx;
extern AVPacket *pack;
extern AVFrame *frame;
extern AVFrame *resized_frame;
extern int video_stream_idx;
extern struct SwsContext *swsctx;
extern TerminalResolution resol;
extern TerminalCanvas *canv;
extern uint8_t no_file_flag;

extern int painter_threads;
extern int rgb_flag;
extern int realtime_flag;

int init_player(int argc, char **argv);

int reinit_player(TerminalResolution res);

void deinit_player();

#endif