#include "TerminalPlayer.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Please specify video file\n");
        return -1;
    }

    set_player_url(argv[1]);
    init_player();

    system("clear");

    while (av_read_frame(input_ctx, pack) >= 0)
    {
        if (pack->stream_index != video_stream_idx)
        {
            av_packet_unref(pack);
            continue;
        }

        avcodec_send_packet(codec_ctx, pack);
        avcodec_receive_frame(codec_ctx, frame);

        double fps = av_q2d(input_ctx->streams[video_stream_idx]->r_frame_rate);
        double sleep_time = 1.0 / (double)fps;

        usleep(sleep_time * 1000 * 1000);

        TerminalResolution cur_resol = terminal_resolution();

        if (cur_resol.x != resol.x || cur_resol.y != resol.y)
        {
            reinit_player(cur_resol);
        }

        sws_scale(swsctx,
                  (const unsigned char *const *)frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  resized_frame->data,
                  resized_frame->linesize);

        terminal_canvas_reset(canv);
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

        terminal_canvas_fini(canv);
        system("clear");
        write(STDOUT_FILENO, canv->data, strlen(canv->data) + 1);
        av_packet_unref(pack);
    }

    deinit_player();

    return 0;
}