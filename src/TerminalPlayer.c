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

TaskParams *task_params;
TerminalCanvas **painter_canvases;

int painter_threads = 8;
char decoder_threads[16] = "4";
int rgb_flag = 1;
int realtime_flag = 1;

int pix_fmt;

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
            if (!strcmp(optarg, "false"))
            {
                realtime_flag = 0;
            }
            else if (!strcmp(optarg, "true"))
            {
                rgb_flag = 1;
            }
            else if (!strcmp(optarg, "0"))
            {
                rgb_flag = 0;
            }
            else if (!strcmp(optarg, "1"))
            {
                rgb_flag = 1;
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

            strcpy(decoder_threads, optarg);

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
            strcpy(video_url, optarg);

            // printf("Value for input = %s\n", optarg);

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

    AVDictionary *dict = NULL;
    av_dict_set(&dict, "threads", decoder_threads, 0);

    avcodec_open2(codec_ctx, codec, &dict);
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

    if (rgb_flag)
        pix_fmt = AV_PIX_FMT_RGB24;
    else
        pix_fmt = AV_PIX_FMT_GRAY8;

    av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, pix_fmt, 16);
    resized_frame->format = pix_fmt;
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
                            pix_fmt,
                            SWS_BICUBIC,
                            NULL,
                            NULL,
                            NULL);
}

int init_player(int argc, char **argv)
{
    av_log_set_level(AV_LOG_QUIET);

    if (parse_args(argc, argv))
        return -1;

    task_params = malloc(sizeof(TaskParams) * painter_threads);
    painter_canvases = malloc(__SIZEOF_POINTER__ * painter_threads);

    for (int i = 0; i < painter_threads; i++)
    {
        painter_canvases[i] = malloc(sizeof(TerminalCanvas));
        terminal_canvas_alloc_data(painter_canvases[i]);
    }

    terminal_cursor_off();
    init_video();
    init_decoder();
    init_display();
    init_frames();
    init_sws();
    if (no_file_flag)
        av_read_play(input_ctx);

    return 0;
}

void reinit_player(TerminalResolution res)
{
    terminal_canvas_free(&canv);
    canv = terminal_canvas_alloc();

    for (int i = 0; i < painter_threads; i++)
    {
        terminal_canvas_delete_data(painter_canvases[i]);
        terminal_canvas_alloc_data(painter_canvases[i]);
    }

    resol = res;

    av_free(resized_frame->data[0]);
    av_frame_free(&resized_frame);

    resized_frame = av_frame_alloc();

    av_image_alloc(resized_frame->data, resized_frame->linesize, resol.x, resol.y, pix_fmt, 16);
    resized_frame->format = pix_fmt;
    resized_frame->width = resol.x;
    resized_frame->height = resol.y;

    sws_freeContext(swsctx);
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
}

void deinit_player()
{
    avformat_close_input(&input_ctx);
    avcodec_close(codec_ctx);
    avcodec_free_context(&codec_ctx);
    sws_freeContext(swsctx);
    terminal_canvas_free(&canv);
    terminal_cursor_on();

    free(task_params);
    free(painter_canvases);
}

