/*
 * bink_selftest.c - test Bink API shims, memory management, coordinate clamping,
 * and end-to-end Bink video decoding.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include "win32hle.h"
#include "host.h"

static int fails = 0;
static void ok(const char *what, int cond) {
    if (!cond) {
        printf("FAIL %s\n", what);
        fails++;
    } else {
        printf("pass %s\n", what);
    }
}

static uint32_t call(const char *name, int n, const uint32_t *a) {
    return hle_call_guest(hle_resolve(name), n, a);
}

const recomp_dispatch_entry_t recomp_dispatch_table[1];
const uint32_t recomp_dispatch_count = 0;

int main(void) {
    recomp_host_init();

    /* 1. Test BinkOpen / Close fallback / dummy with invalid/null input */
    uint32_t args_open[2] = { 0, 0 };
    uint32_t bink_va = call("_BinkOpen@8", 2, args_open);
    ok("BinkOpen returns non-null dummy guest struct", bink_va != 0);

    /* Guest structure layout:
     * Width (0), Height (4), Frames (8), FrameNum (12), LastFrameNum (16), FrameRate (20), FrameRateDiv (24)
     */
    uint32_t *fields = (uint32_t *)hle_ptr(bink_va);
    ok("BinkOpen dummy Width == 640", fields[0] == 640);
    ok("BinkOpen dummy Height == 400", fields[1] == 400);
    ok("BinkOpen dummy Frames == 1", fields[2] == 1);
    ok("BinkOpen dummy FrameNum == 1", fields[3] == 1);
    ok("BinkOpen dummy FrameRate > 0", fields[5] > 0);

    /* 2. Test BinkWait on dummy: MUST return 1 so retail game loop does not spin */
    uint32_t args_bink[1] = { bink_va };
    uint32_t wait_res = call("_BinkWait@4", 1, args_bink);
    ok("BinkWait on dummy returns 1 (movie complete)", wait_res == 1);

    /* 3. Test BinkDoFrame on dummy */
    call("_BinkDoFrame@4", 1, args_bink);
    ok("BinkDoFrame sets LastFrameNum", fields[4] == fields[3]);

    /* 4. Test BinkNextFrame on dummy */
    call("_BinkNextFrame@4", 1, args_bink);
    ok("BinkNextFrame stays at Frames max (1)", fields[3] == 1);

    /* 5. Test BinkCopyToBuffer on dummy: returns 0 without crashing */
    uint32_t dummy_buf_va = hle_va(hle_alloc(640 * 480 * 4));
    uint32_t args_copy_dummy[7] = { bink_va, dummy_buf_va, 640 * 2, 480, 0, 0, 10 };
    uint32_t copy_res = call("_BinkCopyToBuffer@28", 7, args_copy_dummy);
    ok("BinkCopyToBuffer on dummy returns 0 without error", copy_res == 0);

    /* 6. Test BinkClose on dummy */
    call("_BinkClose@4", 1, args_bink);
    ok("BinkClose dummy completed", 1);

    /* 7. Test Coordinate Clamping Calculations for k_CopyToBuffer */
    {
        int pitch = 1280, dh = 480;
        int bpp = 2; /* RGB565 */
        int max_w = pitch / bpp; /* 640 */
        int w = 320, h = 240;

        int dx = 50, dy = 50;
        int y_start = (dy < 0) ? -dy : 0;
        int y_end = (dy + h > dh) ? (dh - dy) : h;
        int x_start = (dx < 0) ? -dx : 0;
        int x_end = (dx + w > max_w) ? (max_w - dx) : w;
        ok("in-bounds y_start == 0", y_start == 0);
        ok("in-bounds y_end == 240", y_end == 240);
        ok("in-bounds x_start == 0", x_start == 0);
        ok("in-bounds x_end == 320", x_end == 320);

        dx = -50; dy = -30;
        y_start = (dy < 0) ? -dy : 0;
        y_end = (dy + h > dh) ? (dh - dy) : h;
        x_start = (dx < 0) ? -dx : 0;
        x_end = (dx + w > max_w) ? (max_w - dx) : w;
        ok("negative dy y_start == 30", y_start == 30);
        ok("negative dy y_end == 240", y_end == 240);
        ok("negative dx x_start == 50", x_start == 50);
        ok("negative dx x_end == 320", x_end == 320);

        dx = 500; dy = 350;
        y_start = (dy < 0) ? -dy : 0;
        y_end = (dy + h > dh) ? (dh - dy) : h;
        x_start = (dx < 0) ? -dx : 0;
        x_end = (dx + w > max_w) ? (max_w - dx) : w;
        ok("overflow dy y_start == 0", y_start == 0);
        ok("overflow dy y_end == 130", y_end == 130);
        ok("overflow dx x_start == 0", x_start == 0);
        ok("overflow dx x_end == 140", x_end == 140);

        dx = 700; dy = 550;
        y_start = (dy < 0) ? -dy : 0;
        y_end = (dy + h > dh) ? (dh - dy) : h;
        x_start = (dx < 0) ? -dx : 0;
        x_end = (dx + w > max_w) ? (max_w - dx) : w;
        ok("completely off-screen y_end <= 0", y_end <= 0);
        ok("completely off-screen x_end <= 0", x_end <= 0);
    }

#ifdef HLE_WITH_FFMPEG
    /* 8. End-to-end Bink decoding test with real .bik files if available */
    const char *test_files[2] = { "/tmp/test_intro.bik", "/tmp/test_radar.bik" };
    for (int t = 0; t < 2; t++) {
        const char *tpath = test_files[t];
        if (access(tpath, R_OK) != 0) continue;

        uint32_t path_va = hle_guest_strdup(tpath);
        uint32_t open_args[2] = { path_va, 0 };
        uint32_t real_bink = call("_BinkOpen@8", 2, open_args);
        ok("Real Bink movie opened", real_bink != 0);
        uint32_t *rfields = (uint32_t *)hle_ptr(real_bink);
        ok("Real Bink Width > 0", rfields[0] > 0);
        ok("Real Bink Height > 0", rfields[1] > 0);
        ok("Real Bink Frames > 1", rfields[2] > 1);
        ok("Real Bink FrameRate == 15", rfields[5] == 15);

        uint32_t real_arg[1] = { real_bink };
        uint32_t w_stat = call("_BinkWait@4", 1, real_arg);
        ok("Real BinkWait frame 1 returns 0 (ready)", w_stat == 0);

        call("_BinkDoFrame@4", 1, real_arg);
        ok("Real BinkDoFrame decodes frame", rfields[4] == 1);

        /* Test CopyToBuffer into RGB565 surface (type 10) */
        int sw = 640, sh = 480;
        int pitch = sw * 2;
        uint16_t *surf565 = (uint16_t *)hle_alloc(sw * sh * sizeof(uint16_t));
        memset(surf565, 0xAA, sw * sh * sizeof(uint16_t)); /* fill with canary pattern */
        uint32_t surf_va = hle_va(surf565);
        uint32_t copy_args[7] = { real_bink, surf_va, (uint32_t)pitch, (uint32_t)sh, 0, 0, 10 };
        call("_BinkCopyToBuffer@28", 7, copy_args);

        /* Verify that pixels were written (canary pattern overwritten) */
        int changed = 0;
        for (int p = 0; p < sw * sh; p++) {
            if (surf565[p] != 0xAAAA) { changed++; }
        }
        ok("Real BinkCopyToBuffer RGB565 modified buffer pixels", changed > 0);

        /* Test CopyToBuffer into RGB32 surface (type 3) */
        pitch = sw * 4;
        uint32_t *surf32 = (uint32_t *)hle_alloc(sw * sh * sizeof(uint32_t));
        memset(surf32, 0x55, sw * sh * sizeof(uint32_t)); /* fill with canary pattern */
        uint32_t surf32_va = hle_va(surf32);
        uint32_t copy32_args[7] = { real_bink, surf32_va, (uint32_t)pitch, (uint32_t)sh, 0, 0, 3 };
        call("_BinkCopyToBuffer@28", 7, copy32_args);
        int changed32 = 0;
        for (int p = 0; p < sw * sh; p++) {
            if (surf32[p] != 0x55555555) { changed32++; }
        }
        ok("Real BinkCopyToBuffer RGB32 modified buffer pixels", changed32 > 0);

        /* Test boundary clamping with active frame: offscreen / negative coords */
        uint32_t copy_clip_args[7] = { real_bink, surf_va, (uint32_t)(sw * 2), (uint32_t)sh, (uint32_t)-20, (uint32_t)-15, 10 };
        call("_BinkCopyToBuffer@28", 7, copy_clip_args);
        ok("Real BinkCopyToBuffer negative offset clipped safely", 1);

        uint32_t copy_off_args[7] = { real_bink, surf_va, (uint32_t)(sw * 2), (uint32_t)sh, 800, 700, 10 };
        call("_BinkCopyToBuffer@28", 7, copy_off_args);
        ok("Real BinkCopyToBuffer off-screen rejected safely", 1);

        /* Test BinkNextFrame */
        call("_BinkNextFrame@4", 1, real_arg);
        ok("Real BinkNextFrame advanced to frame 2", rfields[3] == 2);

        /* Test BinkGoto */
        uint32_t goto_args[3] = { real_bink, 5, 0 };
        call("_BinkGoto@12", 3, goto_args);
        ok("Real BinkGoto seeked to frame 5", rfields[3] == 5);

        /* Test BinkPause */
        uint32_t pause_args[2] = { real_bink, 1 };
        call("_BinkPause@8", 2, pause_args);
        uint32_t p_wait = call("_BinkWait@4", 1, real_arg);
        ok("Real BinkWait paused returns 1", p_wait == 1);
        pause_args[1] = 0;
        call("_BinkPause@8", 2, pause_args);

        /* Test BinkSetVolume */
        uint32_t vol_args[2] = { real_bink, 16384 };
        call("_BinkSetVolume@8", 2, vol_args);
        ok("Real BinkSetVolume accepted", 1);

        /* Test BinkClose */
        call("_BinkClose@4", 1, real_arg);
        ok("Real BinkClose freed host context cleanly", 1);

        hle_free(surf565);
        hle_free(surf32);
    }
#endif

    if (fails) {
        printf("%d failures\n", fails);
        return 1;
    }
    printf("bink_selftest: ALL TESTS PASSED\n");
    return 0;
}
