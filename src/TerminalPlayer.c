#include "TerminalPlayer.h"

char video_url[256];

AVFormatContext *input_ctx = NULL;
AVCodecContext *codec_ctx = NULL;
struct SwsContext *swsctx = NULL;
int video_stream_idx = -1;
uint8_t no_file_flag = 0;

AVPacket *pack = NULL;
AVFrame *frame = NULL;
AVFrame *resized_frame = NULL;

TerminalResolution resol = {.x = -1, .y = -1};
TerminalCanvas *canv = NULL;

TaskParams *task_params = NULL;
TerminalCanvas **painter_canvases = NULL;

int painter_threads = 8;
char decoder_threads[16] = "4";
int rgb_flag = 1;
int realtime_flag = 1;
static int log_lvl = AV_LOG_ERROR;
int pix_fmt;
char *av_err = NULL;

FILE *log_file = NULL;

void custom_log_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
    if (level > log_lvl)
    {
        return;
    }

    if (log_file)
    {
        vfprintf(log_file, fmt, vargs);
        fflush(log_file);
    }
}

int parse_args(int argc, char **argv)
{
    int max_threads = get_nprocs();

    while (1)
    {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        static struct option long_options[] =
        {
            {"color", required_argument, 0, 'c'},
            {"realtime", required_argument, 0, 'r'},
            {"decoder-threads", required_argument, 0, 'd'},
            {"painter-threads", required_argument, 0, 'p'},
            {"input", required_argument, 0, 'i'},
            {"log_lvl", required_argument, 0, 'l'},
            {0,0,0,0}
        };

        int ret = getopt_long_only(argc, argv, "", long_options, &option_index);

        switch (ret)
        {
        case 'c':
            if (!strcmp(optarg, "rgb"))
            {
                rgb_flag = 1;
            }
            else if (!strcmp(optarg, "gray"))
            {
                rgb_flag = 0;
            }
            else
            {
                printf("Incorrect value for color: %s\n", optarg);
                return -1;
            }

            // printf("Value for color = %s\n", optarg);

            break;

        case 'r':
            if (!strcmp(optarg, "false") || !strcmp(optarg, "0"))
            {
                realtime_flag = 0;
            }
            else if (!strcmp(optarg, "true") || !strcmp(optarg, "1"))
            {
                realtime_flag = 1;
            }
            else
            {
                printf("Incorrect value for realtime: %s\n", optarg);
                return -1;
            }

            // printf("Value for realtime = %s\n", optarg);

            break;

        case 'd':
            for (size_t i = 0; i < strlen(optarg); i++)
            {
                if (!isdigit(optarg[i]))
                {
                    printf("Incorrect value for decoder-threads: %s\n", optarg);
                    return -1;
                }
            }

            int decoder_pool = atoi(optarg);

            if (decoder_pool <= 0)
            {
                printf("Incorrect value for decoder-threads: %s\n", optarg);
                return -1;
            }

            if (decoder_pool > max_threads)
            {
                printf("Max value for decoder-threads - %d\n", max_threads);
                return -1;
            }

            snprintf(decoder_threads, sizeof(decoder_threads), "%s", optarg);

            // printf("Value for decoder-threads = %s\n", optarg);

            break;

        case 'p':
            for (size_t i = 0; i < strlen(optarg); i++)
            {
                if (!isdigit(optarg[i]))
                {
                    printf("Incorrect value for painter-threads: %s\n", optarg);
                    return -1;
                }
            }

            painter_threads = atoi(optarg);

            if (decoder_pool <= 0)
            {
                printf("Incorrect value for painter-threads: %s\n", optarg);
                return -1;
            }

            if (painter_threads > max_threads)
            {
                printf("Max value for painter-threads - %d\n", painter_threads);
                return -1;
            }

            // printf("Value for decoder-threads = %s\n", optarg);

            break;

        case 'i':
            snprintf(video_url, sizeof(video_url), "%s", optarg);

            // printf("Value for input = %s\n", optarg);

            break;

        case 'l':
            if (!strcmp(optarg, "quiet"))
                log_lvl = AV_LOG_QUIET;
            else if (!strcmp(optarg, "panic"))
                log_lvl = AV_LOG_PANIC;
            else if (!strcmp(optarg, "warning"))
                log_lvl = AV_LOG_WARNING;
            else if (!strcmp(optarg, "info"))
                log_lvl = AV_LOG_INFO;
            else if (!strcmp(optarg, "verbose"))
                log_lvl = AV_LOG_VERBOSE;
            else if (!strcmp(optarg, "debug"))
                log_lvl = AV_LOG_DEBUG;
            else if (strcmp(optarg, "error"))
                fprintf(log_file, "<%s:%d> No such debug level: %s\n", __FUNCTION__, __LINE__, optarg);
            break;

        default:
            if (ret != -1)
                return -1;
            else
                return 0;
        }
    }

    return 0;
}

int init_video()
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    int ret = 0;
    input_ctx = avformat_alloc_context();

    if (input_ctx == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> AVFormatContext allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    ret = avformat_open_input(&input_ctx, video_url, NULL, NULL);
    if (ret != 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    ret = avformat_find_stream_info(input_ctx, NULL);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    no_file_flag = !input_ctx->iformat->read_play;

    for (int i = 0; i < input_ctx->nb_streams; i++)
    {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = i;
            break;
        }
    }

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int init_decoder()
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    int ret = 0;

    AVDictionary *dict = NULL;

    AVCodec *codec = avcodec_find_decoder(input_ctx->streams[video_stream_idx]->codecpar->codec_id);
    if (codec == NULL)
    {
        ret = AVERROR_DECODER_NOT_FOUND;
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Codec allocation error\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    ret = avcodec_parameters_to_context(codec_ctx, input_ctx->streams[video_stream_idx]->codecpar);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }


    ret = av_dict_set(&dict, "threads", decoder_threads, 0);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    ret = avcodec_open2(codec_ctx, codec, &dict);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int init_display()
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    int ret = 0;

    resol = terminal_resolution();
    canv = terminal_canvas_alloc2(resol);
    if (canv == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Terminal canvas allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    if (canv->data == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Terminal canvas data allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }
exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int init_frames()
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    int ret = 0;

    pack = av_packet_alloc();
    if (pack == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Packet allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    frame = av_frame_alloc();
    if (frame == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Frame allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    resized_frame = av_frame_alloc();
    if (resized_frame == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Frame allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    if (rgb_flag)
        pix_fmt = AV_PIX_FMT_RGB24;
    else
        pix_fmt = AV_PIX_FMT_GRAY8;

    ret = av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, pix_fmt, 16);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    resized_frame->format = pix_fmt;
    resized_frame->width = resol.x;
    resized_frame->height = resol.y;

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    ret = 0;
    return ret;
}

int init_sws()
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    int ret = 0;

    swsctx = sws_getContext(input_ctx->streams[video_stream_idx]->codecpar->width,
                            input_ctx->streams[video_stream_idx]->codecpar->height,
                            AV_PIX_FMT_YUV420P,
                            resol.x,
                            resol.y,
                            pix_fmt,
                            SWS_BICUBIC,
                            NULL,
                            NULL,
                            NULL);
    if (swsctx == NULL)
    {
        ret = -1;
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
        goto exit;
    }

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int init_player(int argc, char **argv)
{
    int ret = 0;

    log_file = fopen("ffmpeg_logs.txt", "w");
    if (!log_file)
    {
        ret = -1;
        goto exit;
    }

    if (parse_args(argc, argv))
    {
        ret = -1;
        goto exit;
    }

    av_log_set_callback(custom_log_callback);
    av_log_set_level(log_lvl);

    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s\n", __FUNCTION__);

    task_params = malloc(sizeof(TaskParams) * painter_threads);
    if (task_params == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    painter_canvases = malloc(__SIZEOF_POINTER__ * painter_threads);
    if (painter_canvases == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    for (int i = 0; i < painter_threads; i++)
    {
        painter_canvases[i] = malloc(sizeof(TerminalCanvas));
        if (painter_canvases[i] == NULL)
        {
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
            ret = -1;
            goto exit;
        }

        ret = terminal_canvas_alloc_data(painter_canvases[i]);
        if (ret != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
            goto exit;
        }
    }

    ret = terminal_cursor_off();
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Cursor off error\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    ret = init_video();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init video error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    ret = init_decoder();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init decoder error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    ret = init_display();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init display error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    ret = init_frames();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init frames error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    ret = init_sws();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init sws error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    if (no_file_flag)
    {
        av_read_play(input_ctx);
    }

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

int reinit_player(TerminalResolution res)
{
    av_log(NULL, AV_LOG_DEBUG, "Invoked: %s", __FUNCTION__);

    int ret = 0;
    terminal_canvas_free(&canv);
    canv = terminal_canvas_alloc();
    if (canv == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Terminal canvas allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    if (canv->data == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Terminal canvas data allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    for (int i = 0; i < painter_threads; i++)
    {
        if (painter_canvases[i] != NULL)
            terminal_canvas_delete_data(painter_canvases[i]);

        ret = terminal_canvas_alloc_data(painter_canvases[i]);
        if (ret != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Memory allocation error\n", __FUNCTION__, __LINE__);
            goto exit;
        }
    }

    resol = res;

    if (resized_frame->data[0] != NULL)
        av_free(resized_frame->data[0]);

    if (resized_frame != NULL)
        av_frame_free(&resized_frame);

    resized_frame = av_frame_alloc();
    if (resized_frame == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Frame allocation error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    ret = av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, pix_fmt, 16);
    if (ret < 0)
    {
        av_err = av_err2str(ret);
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
        goto exit;
    }

    resized_frame->format = pix_fmt;
    resized_frame->width = resol.x;
    resized_frame->height = resol.y;

    sws_freeContext(swsctx);

    ret = init_sws();
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init sws error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

exit:
    av_log(NULL, AV_LOG_DEBUG, "Done: %s, ret: %d\n", __FUNCTION__, ret);
    return ret;
}

void deinit_player()
{
    if (input_ctx != NULL)
        avformat_close_input(&input_ctx);
    if (codec_ctx != NULL)
    {
        avcodec_close(codec_ctx);
        avcodec_free_context(&codec_ctx);
    }
    if (swsctx != NULL)
        sws_freeContext(swsctx);
    if (canv != NULL)
        terminal_canvas_free(&canv);

    terminal_cursor_on();

    if (task_params != NULL)
        free(task_params);

    if (painter_canvases != NULL)
        free(painter_canvases);
}

