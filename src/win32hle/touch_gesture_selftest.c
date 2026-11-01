#include <stdio.h>
#include <assert.h>
#include "recomp_types.h"
#include "mobile_touch_gesture.h"
#include "mobile_rubber_band.h"
#include "mobile_pan.h"
#include "mobile_touch_hud.h"

const recomp_dispatch_entry_t recomp_dispatch_table[] = { { 0, 0 } };
const uint32_t recomp_dispatch_count = 0;

static void test_tap_clicks_on_release(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 1, 100, 120, 1000, &out);
    assert(out.count == 0);

    MobileTouchGesture_End(&touch, 1, 100, 120, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
    assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
    assert(out.events[0].x == 100);
    assert(out.events[0].y == 120);
    assert(out.events[0].update_cursor == 0);
    assert(out.events[1].update_cursor == 0);
}

static void test_tap_end_detection_rejects_non_taps(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 2, 100, 120, 1000, &out);
    assert(MobileTouchGesture_IsTapEnd(&touch, 2, 103, 123));
    assert(!MobileTouchGesture_IsTapEnd(&touch, 3, 103, 123));
    assert(!MobileTouchGesture_IsTapEnd(&touch, 2, 120, 120));

    MobileTouchGesture_Move(&touch, 2, 120, 120, &out);
    assert(!MobileTouchGesture_IsTapEnd(&touch, 2, 120, 120));
}

static void test_drag_holds_left_button_after_slop(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 4, 50, 60, 2000, &out);
    MobileTouchGesture_Move(&touch, 4, 70, 60, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
    assert(out.events[0].x == 50);
    assert(out.events[0].y == 60);
    assert(out.events[0].update_cursor == 1);
    assert(out.events[1].type == MOBILE_TOUCH_MOUSE_MOVE);
    assert(out.events[1].x == 70);
    assert(out.events[1].y == 60);

    MobileTouchGesture_End(&touch, 4, 75, 65, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_MOUSE_MOVE);
    assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
    assert(out.events[1].update_cursor == 0);
}

static void test_two_finger_pan_releases_drag_without_lingering_cursor(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 5, 50, 60, 2500, &out);
    MobileTouchGesture_Move(&touch, 5, 70, 60, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
    assert(out.events[0].update_cursor == 1);

    MobileTouchGesture_Begin(&touch, 6, 80, 90, 2510, &out);
    assert(out.count == 1);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_UP);
    assert(out.events[0].update_cursor == 0);
}

static void test_long_press_is_right_click(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 7, 30, 40, 3000, &out);
    MobileTouchGesture_Update(&touch, 3650, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_RIGHT_DOWN);
    assert(out.events[1].type == MOBILE_TOUCH_RIGHT_UP);
    assert(out.events[0].update_cursor == 0);
    assert(out.events[1].update_cursor == 0);

    MobileTouchGesture_End(&touch, 7, 30, 40, &out);
    assert(out.count == 0);
}

static void test_delayed_drag_after_long_press_starts_selection(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 11, 30, 40, 6000, &out);
    MobileTouchGesture_Update(&touch, 6650, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_RIGHT_DOWN);
    assert(out.events[1].type == MOBILE_TOUCH_RIGHT_UP);

    MobileTouchGesture_Move(&touch, 11, 90, 110, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
    assert(out.events[0].x == 30);
    assert(out.events[0].y == 40);
    assert(out.events[0].update_cursor == 1);
    assert(out.events[1].type == MOBILE_TOUCH_MOUSE_MOVE);
    assert(out.events[1].x == 90);
    assert(out.events[1].y == 110);

    MobileTouchGesture_End(&touch, 11, 100, 120, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_MOUSE_MOVE);
    assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
}

static void test_small_motion_does_not_hover(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 8, 10, 20, 4000, &out);
    MobileTouchGesture_Move(&touch, 8, 13, 23, &out);
    assert(out.count == 0);

    MobileTouchGesture_End(&touch, 8, 13, 23, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_LEFT_DOWN);
    assert(out.events[0].update_cursor == 0);
    assert(out.events[1].type == MOBILE_TOUCH_LEFT_UP);
    assert(out.events[1].update_cursor == 0);
}

static void test_two_finger_pan_cancels_tap(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 9, 40, 50, 5000, &out);
    MobileTouchGesture_Begin(&touch, 10, 80, 90, 5010, &out);
    assert(out.count == 0);

    MobileTouchGesture_NotifyPan(&touch);

    MobileTouchGesture_End(&touch, 9, 42, 52, &out);
    assert(out.count == 0);

    MobileTouchGesture_Update(&touch, 5700, &out);
    assert(out.count == 0);

    MobileTouchGesture_End(&touch, 10, 82, 92, &out);
    assert(out.count == 0);
}

static void test_two_finger_tap_is_right_click(void) {
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobileTouchGesture_Init(&touch);
    MobileTouchGesture_Begin(&touch, 15, 100, 100, 7000, &out);
    MobileTouchGesture_Begin(&touch, 16, 120, 100, 7020, &out);
    assert(out.count == 0);

    // Minor pad wobble (< 18px) should not cancel two-finger tap
    MobileTouchGesture_NotifyPanMotion(&touch, 15, 108, 104);
    MobileTouchGesture_NotifyPanMotion(&touch, 16, 126, 104);
    assert(touch.two_finger_panned == 0);
    assert(touch.two_finger_candidate == 1);

    MobileTouchGesture_End(&touch, 15, 108, 104, &out);
    assert(out.count == 2);
    assert(out.events[0].type == MOBILE_TOUCH_RIGHT_DOWN);
    assert(out.events[0].x == 110);
    assert(out.events[0].y == 100);
    assert(out.events[1].type == MOBILE_TOUCH_RIGHT_UP);
    assert(out.events[1].x == 110);
    assert(out.events[1].y == 100);

    // Moving the remaining finger before lifting must NOT start a left-drag
    MobileTouchGesture_Move(&touch, 16, 145, 120, &out);
    assert(out.count == 0);

    MobileTouchGesture_End(&touch, 16, 145, 120, &out);
    assert(out.count == 0);
}

static void test_rubber_band_commit(void) {
    assert(MobileRubberBand_ShouldCommitCandidateOnRelease(0, 0, 1, 10, 10, 20, 20) == 1);
    assert(MobileRubberBand_ShouldCommitCandidateOnRelease(0, 0, 1, 10, 10, 12, 12) == 0);
}

static int g_pan_key_count = 0;
static int g_last_pan_key = 0;
static void pan_test_key_emit(void *ctx, int vk) {
    (void)ctx;
    g_pan_key_count++;
    g_last_pan_key = vk;
}

static void test_pan_accumulator(void) {
    MobilePanState pan;
    MobilePan_Init(&pan);
    g_pan_key_count = 0;

    // Small delta under 28 threshold: no pulse
    MobilePan_EmitFingerDelta(&pan, 0.015f, 0.0f, 1000, 1000, 1000, pan_test_key_emit, NULL);
    assert(g_pan_key_count == 0);

    // Delta exceeds 28: pulses VK_LEFT
    MobilePan_EmitFingerDelta(&pan, 0.035f, 0.0f, 1000, 1000, 1050, pan_test_key_emit, NULL);
    assert(g_pan_key_count == 1);
    assert(g_last_pan_key == VK_LEFT);
}

static void test_hud_logic(void) {
    MobileTouchHUD hud;
    MobileTouchHud_Init(&hud);
    MobileTouchHud_SetScreenSize(&hud, 1280, 960);

    assert(hud.collapsed == 0);
    int x, y, w, h;
    assert(MobileTouchHud_GetButtonRect(&hud, HUD_BTN_TOGGLE, &x, &y, &w, &h));
    int hit_toggle = MobileTouchHud_HitTest(&hud, x + w / 2, y + h / 2);
    assert(hit_toggle == HUD_BTN_TOGGLE);

    MobileTouchHudAction act;
    // Tap toggle -> collapses
    MobileTouchHud_FingerDown(&hud, 1, x + w / 2, y + h / 2, 1000, &act);
    assert(hud.collapsed == 1);

    // Tap toggle again -> expands
    assert(MobileTouchHud_GetButtonRect(&hud, HUD_BTN_TOGGLE, &x, &y, &w, &h));
    MobileTouchHud_FingerDown(&hud, 1, x + w / 2, y + h / 2, 1100, &act);
    assert(hud.collapsed == 0);

    // Test Force Attack Latch (tap < 300ms)
    assert(MobileTouchHud_GetButtonRect(&hud, HUD_BTN_FORCE_ATTACK, &x, &y, &w, &h));
    MobileTouchHud_FingerDown(&hud, 2, x + w / 2, y + h / 2, 2000, &act);
    MobileTouchHud_FingerUp(&hud, 2, x + w / 2, y + h / 2, 2100, &act);
    assert(act.type == HUD_ACTION_FORCE_ATTACK_CHANGED);
    assert(MobileTouchHud_IsForceAttackActive(&hud) == 1);
    assert(hud.force_attack_latched == 1);

    // After attack fires, latch clears
    MobileTouchHud_OnAttackFired(&hud, &act);
    assert(act.type == HUD_ACTION_FORCE_ATTACK_CHANGED);
    assert(MobileTouchHud_IsForceAttackActive(&hud) == 0);

    // Test Group Assign (long press >= 350ms)
    assert(MobileTouchHud_GetButtonRect(&hud, HUD_BTN_GROUP_1, &x, &y, &w, &h));
    MobileTouchHud_FingerDown(&hud, 3, x + w / 2, y + h / 2, 3000, &act);
    MobileTouchHud_Update(&hud, 3200, &act);
    assert(act.type == HUD_ACTION_NONE);
    MobileTouchHud_Update(&hud, 3400, &act);
    assert(act.type == HUD_ACTION_GROUP_ASSIGN);
    assert(act.param == 1);
    assert(hud.group_assigned[0] == 1);
    MobileTouchHud_FingerUp(&hud, 3, x + w / 2, y + h / 2, 3500, &act);
}

static float g_test_smooth_dx = 0.0f;
static float g_test_smooth_dy = 0.0f;
static int g_test_smooth_call_count = 0;

static int test_smooth_delta_emit(void *ctx, float dx, float dy) {
    (void)ctx;
    g_test_smooth_dx += dx;
    g_test_smooth_dy += dy;
    g_test_smooth_call_count++;
    return 1;
}

static void test_smooth_pan_delta(void) {
    MobilePanState pan;
    MobilePan_Init(&pan);
    g_test_smooth_dx = 0.0f;
    g_test_smooth_dy = 0.0f;
    g_test_smooth_call_count = 0;

    MobilePan_BeginDrag(&pan, 1000);
    // Sub-pixel / small delta (3.5px) arrives: immediately forwarded to smooth hook
    MobilePan_OnMove(&pan, 3.5f, -2.0f, 1016, test_smooth_delta_emit, NULL, pan_test_key_emit);
    assert(g_test_smooth_call_count == 1);
    assert(g_test_smooth_dx == 3.5f);
    assert(g_test_smooth_dy == -2.0f);
}

static void test_inertia_flick_and_decay(void) {
    MobilePanState pan;
    MobilePan_Init(&pan);
    g_test_smooth_dx = 0.0f;
    g_test_smooth_dy = 0.0f;
    g_test_smooth_call_count = 0;

    MobilePan_BeginDrag(&pan, 1000);
    // Fast flick movement: 80 pixels over 32ms (~2.5 px/ms)
    MobilePan_OnMove(&pan, 40.0f, 0.0f, 1016, test_smooth_delta_emit, NULL, pan_test_key_emit);
    MobilePan_OnMove(&pan, 40.0f, 0.0f, 1032, test_smooth_delta_emit, NULL, pan_test_key_emit);

    // Release immediately -> starts inertia coasting
    MobilePan_EndDrag(&pan, 1032);
    assert(MobilePan_IsCoasting(&pan) == 1);
    assert(pan.inertia_vx > 1.0f);

    // Coast for several frames
    float prev_vx = pan.inertia_vx;
    uint32_t tick = 1048;
    int coast_frames = 0;
    while (MobilePan_IsCoasting(&pan) && coast_frames < 200) {
        MobilePan_UpdateInertia(&pan, tick, test_smooth_delta_emit, NULL);
        if (MobilePan_IsCoasting(&pan)) {
            // Deceleration check
            assert(pan.inertia_vx < prev_vx);
            prev_vx = pan.inertia_vx;
        }
        tick += 16;
        coast_frames++;
    }

    // Must have coasted and naturally reached a smooth halt
    assert(coast_frames > 5);
    assert(!MobilePan_IsCoasting(&pan));
}

static void test_pause_before_release_cancels_fling(void) {
    MobilePanState pan;
    MobilePan_Init(&pan);

    MobilePan_BeginDrag(&pan, 2000);
    // Fast movement
    MobilePan_OnMove(&pan, 50.0f, 0.0f, 2016, test_smooth_delta_emit, NULL, pan_test_key_emit);

    // User stops moving and rests finger on screen for 150ms before lifting
    MobilePan_EndDrag(&pan, 2166);
    assert(MobilePan_IsCoasting(&pan) == 0);
}

static void test_tap_to_stop_braking(void) {
    MobilePanState pan;
    MobileTouchGesture touch;
    MobileTouchGestureOutput out;

    MobilePan_Init(&pan);
    MobileTouchGesture_Init(&touch);

    // Fast flick to get into coasting state
    MobilePan_BeginDrag(&pan, 3000);
    MobilePan_OnMove(&pan, 60.0f, 0.0f, 3016, test_smooth_delta_emit, NULL, pan_test_key_emit);
    MobilePan_EndDrag(&pan, 3016);
    assert(MobilePan_IsCoasting(&pan) == 1);

    // While coasting, user taps screen to brake
    uint32_t tap_tick = 3050;
    int was_coasting = MobilePan_IsCoasting(&pan);
    if (was_coasting) {
        MobilePan_StopInertia(&pan);
    }
    assert(!MobilePan_IsCoasting(&pan));

    // Finger touches down
    MobileTouchGesture_Begin(&touch, 99, 200, 200, tap_tick, &out);
    if (was_coasting) {
        MobileTouchGesture_CancelTap(&touch);
    }
    assert(touch.tap_cancelled == 1);
    assert(out.count == 0);

    // Finger lifts without moving past slop
    MobileTouchGesture_End(&touch, 99, 200, 200, &out);
    // Must produce NO click events (tap was safely cancelled by the brake)
    assert(out.count == 0);
}

int main(void) {
    test_tap_clicks_on_release();
    test_tap_end_detection_rejects_non_taps();
    test_drag_holds_left_button_after_slop();
    test_two_finger_pan_releases_drag_without_lingering_cursor();
    test_long_press_is_right_click();
    test_delayed_drag_after_long_press_starts_selection();
    test_small_motion_does_not_hover();
    test_two_finger_pan_cancels_tap();
    test_two_finger_tap_is_right_click();
    test_rubber_band_commit();
    test_pan_accumulator();
    test_hud_logic();
    test_smooth_pan_delta();
    test_inertia_flick_and_decay();
    test_pause_before_release_cancels_fling();
    test_tap_to_stop_braking();

    printf("All mobile touch gesture tests passed successfully!\n");
    return 0;
}
