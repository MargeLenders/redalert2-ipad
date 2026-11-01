/*
 * bink.c - BINKW32 (RAD's Bink movie player, the 1.x API) on ffmpeg's Bink
 * decoder.
 *
 * A game links binkw32.dll and calls it by its decorated names
 * (_BinkOpen@8, ...); the DLL's code is not lifted, so its API is answered
 * here: libavformat reads the .bik (a file name, or with BINKFILEHANDLE a
 * file handle the game positioned itself, at a movie inside an archive),
 * libavcodec decodes its video and audio, BinkCopyToBuffer converts a frame
 * into the game's surface, and the sound goes to the DirectSound mixer as a
 * stream (dsound.c), started with the movie and paced by its frame rate.
 *
 * The handle the game gets is a BINK struct whose leading fields are
 * Bink's own (Width, Height, Frames, FrameNum, LastFrameNum, FrameRate,
 * FrameRateDiv, ...): games read them directly. Without HLE_WITH_FFMPEG,
 * BinkOpen fails, as with a movie that is not there.
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "win32hle.h"

#define BINKFILEHANDLE 0x00800000u

static const char *g_error = "";
static void k_GetError(void) {                           /* a copy in guest memory, made once per message */
    static const char *copied_from;
    static uint32_t va;
    if (copied_from != g_error) va = hle_guest_strdup(g_error), copied_from = g_error;
    RET(va, 0);
}
static void k_SetSoundSystem(void) { RET(1, 2); }        /* (open, param): DirectSound, which is ours */
static void k_OpenDirectSound(void) { RET(0, 1); }
/* BinkDDSurfaceType(surface): the BINKSURFACE type of a DirectDraw surface */
static void k_DDSurfaceType(void) {
    uint8_t *px;
    int w, h, pitch, bpp;
    if (!hle_dd_surface_pixels(A32(0), &px, &w, &h, &pitch, &bpp)) RET(0xFFFFFFFFu, 1);
    RET(bpp == 16 ? 10u /* 565 */ : bpp == 32 ? 3u /* 32 */ : bpp == 24 ? 1u : 0u, 1);
}

/* The guest-visible struct: Bink's leading fields in 32-bit guest memory */
typedef struct {
    uint32_t Width, Height, Frames, FrameNum, LastFrameNum, FrameRate, FrameRateDiv, ReadError, OpenFlags, BinkType,
             Size, FrameSize, SndSize;
    uint32_t pad[64];
    uint32_t host_handle;         /* HLE_H_BINK_MOVIE handle pointing to bink_host_t */
} bink_guest_t;

#ifndef HLE_WITH_FFMPEG
static void k_Open(void) {
    /* If ffmpeg is absent, return a dummy 1-frame movie in guest memory.
     * Retail RA2 unconditionally dereferences the returned Bink pointer
     * (sub_005A1D30 line 9708: mov ecx, [esi+4]; mov edx, [ecx]), so returning
     * NULL crashes the game with SIGBUS on the arena guard page. */
    bink_guest_t *b = (bink_guest_t *)hle_alloc(sizeof(bink_guest_t));
    if (b) {
        memset(b, 0, sizeof(*b));
        b->Width = 640;
        b->Height = 400;
        b->Frames = 1;
        b->FrameNum = 1;
        b->LastFrameNum = 1;
        b->FrameRate = 15;
        b->FrameRateDiv = 1;
        RETP(b, 2);
    } else {
        RETV(2);
    }
}
static void k_Close(void) {
    bink_guest_t *b = (bink_guest_t *)APTR(0);
    if (b) hle_free(b);
    RETV(1);
}
static void k_NextFrame(void) {
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    if (g && g->FrameNum < g->Frames) g->FrameNum++;
    RETV(1);
}
static void k_Wait(void) {
    /* Return 1 so game loop (sub_00430600) exits instead of spinning in an infinite loop */
    RET(1, 1);
}
static void k_DoFrame(void) {
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    if (g) g->LastFrameNum = g->FrameNum;
    RET(0, 1);
}
static void k_ret0_1(void) { RET(0, 1); }
static void k_ret0_2(void) { RET(0, 2); }
static void k_ret0_3(void) { RET(0, 3); }
static void k_ret0_7(void) { RET(0, 7); }
int hle_bink_fullscreen_movie_playing(void) { return 0; }
const win32hle_shim win32hle_bink[] = {
    { "_BinkOpen@8", k_Open }, { "_BinkClose@4", k_Close }, { "_BinkDoFrame@4", k_DoFrame },
    { "_BinkNextFrame@4", k_NextFrame }, { "_BinkWait@4", k_Wait }, { "_BinkCopyToBuffer@28", k_ret0_7 },
    { "_BinkGoto@12", k_ret0_3 }, { "_BinkPause@8", k_ret0_2 }, { "_BinkSetVolume@8", k_ret0_2 },
    { "_BinkGetError@0", k_GetError }, { "_BinkSetSoundSystem@8", k_SetSoundSystem },
    { "_BinkOpenDirectSound@4", k_OpenDirectSound }, { "_BinkDDSurfaceType@4", k_DDSurfaceType },
    { 0, 0 } };
#else
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>

/* Host multimedia context: host-allocated, never touches guest memory directly */
static volatile int g_fullscreen_movie_count = 0;
int hle_bink_fullscreen_movie_playing(void) { return g_fullscreen_movie_count > 0; }

typedef struct {
    bink_guest_t *guest;
    int fd;
    int own_fd;                   /* 1 if host opened the fd, 0 if borrowed via BINKFILEHANDLE */
    int is_fullscreen;            /* 1 if movie resolution qualifies as fullscreen cutscene */
    int64_t base;                 /* where the movie starts in the file */
    AVFormatContext *fmt;
    AVIOContext *io;
    AVCodecContext *vdec, *adec;
    int vs, as;
    AVFrame *frame, *aframe;
    AVPacket *pkt;
    SwrContext *swr;
    int stream;                   /* the sound's mixer stream */
    int paused, have_frame, done;
    uint32_t start_ms, paused_at;
    float volume;
} bink_host_t;

static inline bink_host_t *bink_get_host(void *ptr) {
    if (!ptr) return NULL;
    bink_guest_t *g = (bink_guest_t *)ptr;
    return (bink_host_t *)hle_handle_obj(g->host_handle, HLE_H_BINK_MOVIE);
}

static int io_read(void *opaque, uint8_t *buf, int n) {
    bink_host_t *b = (bink_host_t *)opaque;
    ssize_t r = read(b->fd, buf, (size_t)n);
    return r > 0 ? (int)r : AVERROR_EOF;
}
static int64_t io_seek(void *opaque, int64_t off, int whence) {
    bink_host_t *b = (bink_host_t *)opaque;
    if (whence == AVSEEK_SIZE) return -1;
    whence &= ~AVSEEK_FORCE;
    if (whence == SEEK_SET) off += b->base;
    off_t r = lseek(b->fd, (off_t)off, whence);
    return r < 0 ? -1 : (int64_t)r - b->base;
}

static void close_bink(bink_host_t *b) {
    if (!b) return;
    if (b->is_fullscreen) {
        if (g_fullscreen_movie_count > 0) g_fullscreen_movie_count--;
        b->is_fullscreen = 0;
    }
    if (b->stream >= 0) hle_audio_stream_close(b->stream);
    if (b->swr) swr_free(&b->swr);
    if (b->vdec) avcodec_free_context(&b->vdec);
    if (b->adec) avcodec_free_context(&b->adec);
    if (b->fmt) avformat_close_input(&b->fmt);
    if (b->io) { av_freep(&b->io->buffer); avio_context_free(&b->io); }
    av_frame_free(&b->frame);
    av_frame_free(&b->aframe);
    av_packet_free(&b->pkt);
    if (b->fd >= 0 && b->own_fd) close(b->fd);
    if (b->guest) {
        if (b->guest->host_handle) hle_handle_close(b->guest->host_handle);
        hle_free(b->guest);
    }
    free(b);
}

static void make_dummy_guest(bink_guest_t *g, uint32_t flags) {
    memset(g, 0, sizeof(*g));
    g->Width = 640;
    g->Height = 400;
    g->Frames = 1;
    g->FrameNum = 1;
    g->LastFrameNum = 1;
    g->FrameRate = 15;
    g->FrameRateDiv = 1;
    g->OpenFlags = flags;
    g->host_handle = 0;
}

static void push_audio(bink_host_t *b) {
    if (b->stream < 0 || !b->swr) return;
    int16_t out[8192 * 2];
    int n = swr_convert(b->swr, (uint8_t **)&(uint8_t *){ (uint8_t *)out }, 8192, (const uint8_t **)b->aframe->extended_data,
                        b->aframe->nb_samples);
    if (n > 0) hle_audio_stream_push(b->stream, out, n);
}

/* Read packets until the next video frame is decoded; sound met on the way
 * goes to the mixer. */
static int decode_frame(bink_host_t *b) {
    for (;;) {
        int r = avcodec_receive_frame(b->vdec, b->frame);
        if (r == 0) return 1;
        if (r != AVERROR(EAGAIN)) return 0;
        if (av_read_frame(b->fmt, b->pkt) < 0) { avcodec_send_packet(b->vdec, NULL); b->done = 1; continue; }
        if (b->pkt->stream_index == b->vs) avcodec_send_packet(b->vdec, b->pkt);
        else if (b->adec && b->pkt->stream_index == b->as && avcodec_send_packet(b->adec, b->pkt) == 0)
            while (avcodec_receive_frame(b->adec, b->aframe) == 0) push_audio(b);
        av_packet_unref(b->pkt);
        if (b->done) return 0;
    }
}

static void k_Open(void) {                               /* (name or handle, flags) */
    uint32_t flags = A32(1);
    bink_guest_t *g = (bink_guest_t *)hle_alloc(sizeof *g); /* the guest reads its leading fields */
    if (!g) { g_error = "cannot allocate bink structure"; RET(0, 2); return; }
    bink_host_t *b = (bink_host_t *)calloc(1, sizeof *b);
    if (!b) {
        /* Even if host alloc fails, return dummy guest struct so retail RA2 does not crash with SIGBUS in sub_005A1D30 */
        g_error = "cannot allocate host bink context";
        make_dummy_guest(g, flags);
        RETP(g, 2);
        return;
    }
    b->guest = g;
    g->host_handle = hle_handle_alloc(HLE_H_BINK_MOVIE, b);
    b->stream = -1, b->volume = 1.0f, b->fd = -1;
    g->OpenFlags = flags;
    if (flags & BINKFILEHANDLE) {
        b->fd = hle_file_fd(A32(0));
        b->own_fd = 0;
        off_t cur = b->fd >= 0 ? lseek(b->fd, 0, SEEK_CUR) : -1;
        b->base = cur >= 0 ? (int64_t)cur : 0;
    } else if (A32(0)) {
        char path[1024];
        if (hle_host_path(ASTR(0), path, sizeof path)) {
            b->fd = open(path, O_RDONLY | O_CLOEXEC);
            b->own_fd = 1;
        }
    }
    if (b->fd < 0) {
        g_error = "cannot open the movie";
        goto fail;
    }
    uint8_t *io_buf = (uint8_t *)av_malloc(65536);
    if (!io_buf) {
        g_error = "cannot allocate io buffer";
        goto fail;
    }
    b->io = avio_alloc_context(io_buf, 65536, 0, b, io_read, NULL, io_seek);
    if (!b->io) {
        av_free(io_buf);
        g_error = "cannot allocate avio context";
        goto fail;
    }
    b->fmt = avformat_alloc_context();
    if (!b->fmt) {
        g_error = "cannot allocate format context";
        goto fail;
    }
    b->fmt->pb = b->io;
    b->fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
    const AVInputFormat *bik = av_find_input_format("bink");
    if (avformat_open_input(&b->fmt, NULL, bik, NULL) < 0 || avformat_find_stream_info(b->fmt, NULL) < 0) {
        g_error = "not a Bink movie";
        goto fail;
    }
    b->vs = av_find_best_stream(b->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    b->as = av_find_best_stream(b->fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (b->vs < 0) { g_error = "no video in the movie"; goto fail; }
    AVStream *v = b->fmt->streams[b->vs];
    const AVCodec *vc = avcodec_find_decoder(v->codecpar->codec_id);
    b->vdec = avcodec_alloc_context3(vc);
    if (!b->vdec) { g_error = "cannot allocate video decoder context"; goto fail; }
    avcodec_parameters_to_context(b->vdec, v->codecpar);
    if (!vc || avcodec_open2(b->vdec, vc, NULL) < 0) { g_error = "no Bink video decoder"; goto fail; }
    if (b->as >= 0) {
        AVStream *a = b->fmt->streams[b->as];
        const AVCodec *ac = avcodec_find_decoder(a->codecpar->codec_id);
        b->adec = ac ? avcodec_alloc_context3(ac) : NULL;
        if (b->adec) {
            avcodec_parameters_to_context(b->adec, a->codecpar);
            if (avcodec_open2(b->adec, ac, NULL) < 0) avcodec_free_context(&b->adec);
        }
        if (b->adec) {
            int ch = b->adec->ch_layout.nb_channels > 1 ? 2 : 1;
            AVChannelLayout out = ch == 2 ? (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO : (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
            swr_alloc_set_opts2(&b->swr, &out, AV_SAMPLE_FMT_S16, b->adec->sample_rate, &b->adec->ch_layout,
                                b->adec->sample_fmt, b->adec->sample_rate, 0, NULL);
            if (b->swr && swr_init(b->swr) == 0) b->stream = hle_audio_stream_open(b->adec->sample_rate, ch);
        }
    }
    b->frame = av_frame_alloc(), b->aframe = av_frame_alloc(), b->pkt = av_packet_alloc();
    if (!b->frame || !b->aframe || !b->pkt) { g_error = "cannot allocate frame or packet"; goto fail; }
    g->Width = (uint32_t)v->codecpar->width, g->Height = (uint32_t)v->codecpar->height;
    g->Frames = v->nb_frames > 0 ? (uint32_t)v->nb_frames : (uint32_t)v->duration;
    g->FrameRate = (uint32_t)v->avg_frame_rate.num, g->FrameRateDiv = (uint32_t)(v->avg_frame_rate.den ? v->avg_frame_rate.den : 1);
    if (!g->FrameRate) g->FrameRate = 15, g->FrameRateDiv = 1;
    g->FrameNum = 1, g->LastFrameNum = 0;
    b->start_ms = hle_ticks_ms();
    /* Only classify movies with an audio track as fullscreen cutscenes.
     * In Red Alert 2 / Yuri's Revenge, all campaign cutscenes and briefings
     * have audio tracks (b->as >= 0), whereas the main menu background animation
     * (mainmenu.bik, 632x570) and in-mission radar videos (140x110) are silent.
     * Flagging menu animations as fullscreen would cause tap-to-skip to intercept
     * and swallow all touchscreen input in the main menu. */
    if (g->Width >= 320 && g->Height >= 200 && b->as >= 0) {
        b->is_fullscreen = 1;
        g_fullscreen_movie_count++;
    }
    fprintf(stderr, "[bink] %ux%u, %u frames at %u/%u fps%s%s\n", g->Width, g->Height, g->Frames, g->FrameRate,
            g->FrameRateDiv, b->stream >= 0 ? ", with sound" : "", b->is_fullscreen ? " [fullscreen cutscene]" : "");
    RETP(g, 2);
    return;

fail:
    if (b) {
        b->guest = NULL; /* detach guest so close_bink won't free it */
        close_bink(b);
    }
    make_dummy_guest(g, flags);
    RETP(g, 2);
}
static void k_Close(void) {
    if (A32(0)) {
        bink_host_t *b = bink_get_host(APTR(0));
        if (b) close_bink(b);
        else hle_free(APTR(0));
    }
    RETV(1);
}

static void k_DoFrame(void) {                            /* decode the current frame */
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    if (b && g) {
        b->have_frame = decode_frame(b);
        g->LastFrameNum = g->FrameNum;
    }
    RET(0, 1);
}
static void k_NextFrame(void) {
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    if (g && g->FrameNum < g->Frames) g->FrameNum++;
    RETV(1);
}
/* BinkWait: nonzero while it is not yet time for the next frame. */
static void k_Wait(void) {
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    if (!b || !g) { RET(1, 1); return; }
    if (b->paused || b->done || (g->Frames && g->FrameNum >= g->Frames)) { RET(1, 1); return; }
    uint32_t fps = g->FrameRate ? g->FrameRate : 15;
    uint32_t div = g->FrameRateDiv ? g->FrameRateDiv : 1;
    uint32_t frame_idx = g->FrameNum > 0 ? g->FrameNum - 1 : 0;
    uint64_t due = (uint64_t)frame_idx * 1000u * div / fps;
    RET(hle_ticks_ms() - b->start_ms < due ? 1u : 0u, 1);
}
static void k_Pause(void) {                              /* (bink, pause) -> the state */
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    if (!b || !g) { RET(0, 2); return; }
    int p = A32(1) != 0;
    if (p && !b->paused) b->paused_at = hle_ticks_ms();
    if (!p && b->paused) b->start_ms += hle_ticks_ms() - b->paused_at;
    b->paused = p;
    if (b->stream >= 0) hle_audio_stream_gain(b->stream, p ? 0.0f : b->volume);
    RET((uint32_t)p, 2);
}
static void k_SetVolume(void) {                          /* (bink, volume) 32768 is full */
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    if (!b || !g) { RETV(2); return; }
    b->volume = (float)(int32_t)A32(1) / 32768.0f;
    if (b->volume > 1.0f) b->volume = 1.0f;
    if (b->stream >= 0 && !b->paused) hle_audio_stream_gain(b->stream, b->volume);
    RETV(2);
}
static void k_Goto(void) {                               /* (bink, frame, flags): back to the start, then forward */
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    if (!b || !g) { RETV(3); return; }
    uint32_t to = A32(1) ? A32(1) : 1;
    av_seek_frame(b->fmt, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(b->vdec);
    if (b->adec) avcodec_flush_buffers(b->adec);
    b->done = 0, g->FrameNum = 1;
    while (g->FrameNum < to && decode_frame(b)) g->FrameNum++;
    uint32_t fps = g->FrameRate ? g->FrameRate : 15;
    uint32_t div = g->FrameRateDiv ? g->FrameRateDiv : 1;
    b->start_ms = hle_ticks_ms() - (uint32_t)((uint64_t)(g->FrameNum - 1) * 1000u * div / fps);
    RETV(3);
}

/* BinkCopyToBuffer(bink, dest, pitch, height, x, y, flags): the decoded frame
 * into a buffer of the type flags' low bits name (565, 555, 32-bit). */
static void k_CopyToBuffer(void) {
    bink_guest_t *g = (bink_guest_t *)APTR(0);
    bink_host_t *b = bink_get_host(g);
    uint8_t *dst = (uint8_t *)APTR(1);
    int pitch = (int)A32(2), dh = (int)A32(3), dx = (int)A32(4), dy = (int)A32(5), type = (int)(A32(6) & 0xF);
    if (!b || !b->have_frame || !dst || pitch <= 0 || dh <= 0) { RET(0, 7); return; }
    AVFrame *f = b->frame;
    if (!f || (f->format != AV_PIX_FMT_YUV420P && f->format != AV_PIX_FMT_YUVA420P)) { RET(0, 7); return; }
    if (!f->data[0] || !f->data[1] || !f->data[2]) { RET(0, 7); return; }

    int bpp = (type == 10 || type == 9) ? 2 : (type == 3 || type == 5) ? 4 : 0;
    if (bpp == 0) { RET(0, 7); return; }
    int max_w = pitch / bpp;
    if (max_w <= 0) { RET(0, 7); return; }

    int w = f->width, h = f->height;
    int y_start = (dy < 0) ? -dy : 0;
    int y_end = (dy + h > dh) ? (dh - dy) : h;
    if (y_start >= y_end || y_start >= h || y_end <= 0) { RET(0, 7); return; }

    int x_start = (dx < 0) ? -dx : 0;
    int x_end = (dx + w > max_w) ? (max_w - dx) : w;
    if (x_start >= x_end || x_start >= w || x_end <= 0) { RET(0, 7); return; }

    for (int y = y_start; y < y_end; y++) {
        int dest_y = dy + y;
        if (dest_y < 0 || dest_y >= dh) continue;
        const uint8_t *Y = f->data[0] + y * f->linesize[0];
        const uint8_t *U = f->data[1] + (y / 2) * f->linesize[1];
        const uint8_t *V = f->data[2] + (y / 2) * f->linesize[2];
        uint8_t *row = dst + (size_t)dest_y * (size_t)pitch;
        for (int x = x_start; x < x_end; x++) {
            int X = dx + x;
            if (X < 0 || X >= max_w) continue;
            int c = Y[x] - 16, d = U[x / 2] - 128, e = V[x / 2] - 128;
            int r = (298 * c + 409 * e + 128) >> 8, gg = (298 * c - 100 * d - 208 * e + 128) >> 8, bb = (298 * c + 516 * d + 128) >> 8;
            r = r < 0 ? 0 : r > 255 ? 255 : r, gg = gg < 0 ? 0 : gg > 255 ? 255 : gg, bb = bb < 0 ? 0 : bb > 255 ? 255 : bb;
            if (type == 10) ((uint16_t *)row)[X] = (uint16_t)((r >> 3) << 11 | (gg >> 2) << 5 | bb >> 3);
            else if (type == 9) ((uint16_t *)row)[X] = (uint16_t)((r >> 3) << 10 | (gg >> 3) << 5 | bb >> 3);
            else if (type == 3 || type == 5) ((uint32_t *)row)[X] = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)gg << 8 | (uint32_t)bb;
        }
    }
    RET(0, 7);
}

/* ---- BinkBuffer: where a GDI game has Bink put the picture ----
 * The game locks the buffer, has BinkCopyToBuffer fill it at its
 * SurfaceType, and blits it to its window at the offset it set: here a
 * 32-bit buffer, blitted onto the window surface (gdidc.c). The leading
 * fields are Bink's BINKBUFFER, which games read. */
typedef struct {
    uint32_t Width, Height, WindowWidth, WindowHeight, SurfaceType, Buffer;
    int32_t BufferPitch;
    uint32_t ClientOffsetX, ClientOffsetY, ScreenWidth, ScreenHeight, ScreenDepth;
    uint32_t pad[48];
    int destx, desty;                                    /* ours */
} binkbuf_t;
static void k_BufferOpen(void) {                         /* (hwnd, width, height, flags) */
    binkbuf_t *b = (binkbuf_t *)hle_alloc(sizeof *b);   /* the game reads its fields */
    int sw, sh;
    const uint8_t *px; int pitch, bpp;
    b->Width = b->WindowWidth = A32(1), b->Height = b->WindowHeight = A32(2);
    b->SurfaceType = 3;                                  /* BINKSURFACE32 */
    b->BufferPitch = (int32_t)(b->Width * 4);
    b->Buffer = hle_va(hle_alloc((b->Width * b->Height + 1) * 4u));
    if (hle_gdi_window_frame(&px, &sw, &sh, &pitch, &bpp, NULL)) b->ScreenWidth = (uint32_t)sw, b->ScreenHeight = (uint32_t)sh, b->ScreenDepth = (uint32_t)bpp;
    RETP(b, 4);
}
static void k_BufferClose(void) {
    binkbuf_t *b = (binkbuf_t *)APTR(0);
    if (b) hle_free(hle_ptr(b->Buffer)), hle_free(b);
    RETV(1);
}
static void k_BufferLock(void) { RET(A32(0) != 0, 1); }
static void k_BufferUnlock(void) { RET(1, 1); }
static void k_BufferSetOffset(void) {                    /* (buf, x, y) */
    binkbuf_t *b = (binkbuf_t *)APTR(0);
    if (b) b->destx = (int)A32(1), b->desty = (int)A32(2);
    RET(1, 3);
}
static void k_BufferBlit(void) {                         /* (buf, rects, n): the whole picture, whatever changed */
    binkbuf_t *b = (binkbuf_t *)APTR(0);
    const uint8_t *px;
    int sw, sh, pitch, bpp;
    if (b && hle_gdi_window_frame(&px, &sw, &sh, &pitch, &bpp, NULL) && bpp == 32) {
        for (int y = 0; y < (int)b->Height; y++) {
            int ty = b->desty + y;
            if (ty < 0 || ty >= sh) continue;
            for (int x = 0; x < (int)b->Width; x++) {
                int tx = b->destx + x;
                if (tx >= 0 && tx < sw)
                    ((uint32_t *)(px + (size_t)ty * pitch))[tx] = ((const uint32_t *)hle_ptr(b->Buffer))[(size_t)y * b->Width + x] & 0xFFFFFFu;
            }
        }
        hle_gdi_window_touched();
    }
    RETV(3);
}
static void k_GetRects(void) { RET(1, 2); }              /* (bink, flags): one rectangle, the whole frame */
static void k_SetIOSize(void) { RETV(1); }

const win32hle_shim win32hle_bink[] = {
    { "_BinkBufferOpen@16", k_BufferOpen }, { "_BinkBufferClose@4", k_BufferClose },
    { "_BinkBufferLock@4", k_BufferLock }, { "_BinkBufferUnlock@4", k_BufferUnlock },
    { "_BinkBufferSetOffset@12", k_BufferSetOffset }, { "_BinkBufferBlit@12", k_BufferBlit },
    { "_BinkGetRects@8", k_GetRects }, { "_BinkSetIOSize@4", k_SetIOSize },
    { "_BinkOpen@8", k_Open }, { "_BinkClose@4", k_Close }, { "_BinkDoFrame@4", k_DoFrame },
    { "_BinkNextFrame@4", k_NextFrame }, { "_BinkWait@4", k_Wait }, { "_BinkCopyToBuffer@28", k_CopyToBuffer },
    { "_BinkGoto@12", k_Goto }, { "_BinkPause@8", k_Pause }, { "_BinkSetVolume@8", k_SetVolume },
    { "_BinkGetError@0", k_GetError }, { "_BinkSetSoundSystem@8", k_SetSoundSystem },
    { "_BinkOpenDirectSound@4", k_OpenDirectSound }, { "_BinkDDSurfaceType@4", k_DDSurfaceType },
    { 0, 0 } };
#endif
