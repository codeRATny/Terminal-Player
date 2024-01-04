#include "TerminalPlayer.h"

AVFormatContext *input_ctx = NULL;
AVCodecContext *codec_ctx = NULL;

struct SwsContext *swsctx = NULL;

AVPacket *pack = NULL;

AVFrame *frame = NULL;
AVFrame *resized_frame = NULL;

int video_stream_idx = -1;

TerminalResolution resol = {.x = -1, .y = -1};
TerminalCanvas *canv = NULL;

char *video_url = NULL;

uint8_t no_file_flag = 0;

void init_video()
{
    input_ctx = avformat_alloc_context();
    avformat_open_input(&input_ctx, video_url, NULL, NULL);
    avformat_find_stream_info(input_ctx, NULL);
    no_file_flag = !input_ctx->iformat->read_play;

    for (int i = 0; i < input_ctx->nb_streams; i++)
    {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = i;
            break;
        }
    }
}

void init_decoder()
{
    AVCodec *codec = avcodec_find_decoder(input_ctx->streams[video_stream_idx]->codecpar->codec_id);
    codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, input_ctx->streams[video_stream_idx]->codecpar);
    avcodec_open2(codec_ctx, codec, NULL);
}

void init_display()
{
    resol = terminal_resolution();
    canv = terminal_canvas_alloc2(resol);
}

void init_frames()
{
    pack = av_packet_alloc();

    frame = av_frame_alloc();

    resized_frame = av_frame_alloc();
    av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, AV_PIX_FMT_GRAY8, 16);
    resized_frame->format = AV_PIX_FMT_GRAY8;
    resized_frame->width = resol.x;
    resized_frame->height = resol.y;
}

void init_sws()
{
    swsctx = sws_getContext(input_ctx->streams[video_stream_idx]->codecpar->width,
                            input_ctx->streams[video_stream_idx]->codecpar->height,
                            AV_PIX_FMT_YUV420P,
                            resol.x,
                            resol.y,
                            AV_PIX_FMT_GRAY8,
                            SWS_BICUBIC,
                            NULL,
                            NULL,
                            NULL);
}


void set_player_url(char *url)
{
    video_url = url;
}

void init_player()
{
    av_log_set_level(AV_LOG_QUIET);
    terminal_cursor_off();
    init_video();
    init_decoder();
    init_display();
    init_frames();
    init_sws();
    if (no_file_flag)
        av_read_play(input_ctx);
}

void reinit_player(TerminalResolution res)
{
    terminal_canvas_free(&canv);
    canv = terminal_canvas_alloc();

    resol = res;

    av_free(resized_frame->data[0]);
    av_frame_free(&resized_frame);

    resized_frame = av_frame_alloc();

    av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, AV_PIX_FMT_GRAY8, 16);
    resized_frame->format = AV_PIX_FMT_GRAY8;
    resized_frame->width = resol.x;
    resized_frame->height = resol.y;

    sws_freeContext(swsctx);
    swsctx = sws_getContext(input_ctx->streams[video_stream_idx]->codecpar->width,
                            input_ctx->streams[video_stream_idx]->codecpar->height,
                            AV_PIX_FMT_YUV420P,
                            resol.x,
                            resol.y,
                            AV_PIX_FMT_GRAY8,
                            SWS_BICUBIC,
                            NULL,
                            NULL,
                            NULL);
}

void deinit_player()
{
    avformat_close_input(&input_ctx);
    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);
    sws_freeContext(swsctx);
    terminal_canvas_free(&canv);
    terminal_cursor_on();
}