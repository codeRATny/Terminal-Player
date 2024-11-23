#include "TerminalPlayer.h"
#include "Pallete.h"
#include <sys/time.h>
#include "thpool.h"

void sig_int(int sig)
{
    (void)!system("clear");
    deinit_player();
    exit(1);
}

uint64_t get_current_time() {
	struct timeval t;
	gettimeofday(&t, 0);
	return t.tv_sec * 1000000 + t.tv_usec;
}

void task(void *arg)
{
	TaskParams *params = (TaskParams*)arg;

    for (size_t line = params->line_offset; line < params->line_offset + params->line_count; line++)
    {
        for (size_t dot = 0; dot < resized_frame->linesize[0]; dot += 3)
        {
            if (dot < resol.x * 3)
            {
                int r = resized_frame->data[0][resized_frame->linesize[0] * line + dot];
                int g = resized_frame->data[0][resized_frame->linesize[0] * line + dot + 1];
                int b = resized_frame->data[0][resized_frame->linesize[0] * line + dot + 2];
                int idd = search_nearest_color(r, g, b);

                terminal_canvas_add_pixel_rgb(painter_canvases[params->canv_idx], idd);
            }
        }
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_int);

    int ret = 0;
    char *av_err = NULL;

    ret = init_player(argc, argv);
    if (ret != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init player error: %d\n", __FUNCTION__, __LINE__, ret);
        goto exit;
    }

    threadpool thpool = thpool_init(painter_threads);
    if (thpool == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "<%s:%d> Init threadpool error\n", __FUNCTION__, __LINE__);
        ret = -1;
        goto exit;
    }

    (void)!system("clear");

    pallete_init();

    terminal_canvas_reset(canv);

    while (av_read_frame(input_ctx, pack) >= 0)
    {
        if (pack->stream_index != video_stream_idx)
        {
            av_packet_unref(pack);
            continue;
        }

        uint64_t start = get_current_time();

        ret = avcodec_send_packet(codec_ctx, pack);
        if (ret != 0)
        {
            av_err = av_err2str(ret);
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Error: %s(%d)\n", __FUNCTION__, __LINE__, av_err, ret);
            goto exit;
        }

        if (avcodec_receive_frame(codec_ctx, frame))
            continue;

        double fps = av_q2d(input_ctx->streams[video_stream_idx]->r_frame_rate);
        uint64_t sleep_time = (1.0 / (double)fps) * 1000000.0;

        TerminalResolution cur_resol = terminal_resolution();

        if (cur_resol.x != resol.x || cur_resol.y != resol.y)
        {
            ret = reinit_player(cur_resol);
            if (ret != 0)
            {
                av_log(NULL, AV_LOG_ERROR, "<%s:%d> Reinit player error: %d\n", __FUNCTION__, __LINE__, ret);
                goto exit;
            }
        }

        sws_scale(swsctx,
                  (const unsigned char *const *)frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  resized_frame->data,
                  resized_frame->linesize);

        terminal_canvas_reset(canv);

        if (rgb_flag)
        {
            int lines = resol.y;
            int line_per_thread = lines / painter_threads;
            int offset_per_thread;
            for (int i = 0; i < painter_threads; i++)
            {
                task_params[i].line_count = line_per_thread;
                task_params[i].line_offset = line_per_thread * i;
            }

            lines -= line_per_thread * painter_threads;

            if (lines > 0)
            {
                task_params[painter_threads - 1].line_count += lines;
            }

            for (int i = 0; i < painter_threads; i++)
            {
                task_params[i].canv_idx = i;

		        ret = thpool_add_work(thpool, task, (void*)&task_params[i]);
                if (ret != 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "<%s:%d> Add work to the job queue error: %d\n", __FUNCTION__, __LINE__, ret);
                    goto exit;
                }
	        };

            thpool_wait(thpool);

            for (int i = 0; i < painter_threads; i++)
            {
                memcpy(canv->data + canv->pos, painter_canvases[i]->data, painter_canvases[i]->pos);
                canv->pos += painter_canvases[i]->pos;
                terminal_canvas_reset(painter_canvases[i]);
	        };
        }
        else
        {
            for (size_t line = 0; line < resol.y; line++)
            {
                for (size_t dot = 0; dot < resized_frame->linesize[0]; dot++)
                {
                    if (dot < resol.x)
                    {
                        size_t finded = find_closest_color(resized_frame->data[0][resized_frame->linesize[0] * line + dot]);
                        terminal_canvas_add_pixel(canv, finded);
                    }
                }
            }
        }

        terminal_canvas_fini(canv);
        ret = terminal_seek_coord(1,1);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Seek coord error: %d\n", __FUNCTION__, __LINE__, ret);
            goto exit;
        }

        uint64_t stop = get_current_time();
        uint64_t elapsed = stop - start;

        if (no_file_flag && realtime_flag)
        {
            if (sleep_time > elapsed)
                usleep((size_t)(sleep_time - elapsed));
        }

        ret = write(STDOUT_FILENO, canv->data, strlen(canv->data) + 1);
        av_packet_unref(pack);

        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "<%s:%d> Stdout write error: %d\n", __FUNCTION__, __LINE__, ret);
            goto exit;
        }
    }

exit:
    deinit_player();
    return ret;
}