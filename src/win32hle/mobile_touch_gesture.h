#ifndef MOBILE_TOUCH_GESTURE_H
#define MOBILE_TOUCH_GESTURE_H

#include <stdint.h>

typedef enum {
    MOBILE_TOUCH_MOUSE_MOVE,
    MOBILE_TOUCH_LEFT_DOWN,
    MOBILE_TOUCH_LEFT_UP,
    MOBILE_TOUCH_RIGHT_DOWN,
    MOBILE_TOUCH_RIGHT_UP
} MobileTouchGestureEventType;

typedef struct {
    MobileTouchGestureEventType type;
    int x;
    int y;
    int update_cursor;
} MobileTouchGestureEvent;

typedef struct {
    int count;
    MobileTouchGestureEvent events[4];
} MobileTouchGestureOutput;

typedef struct {
    int primary_active;
    int secondary_active;
    int left_down;
    int dragging;
    int long_press_sent;
    int tap_cancelled;
    int64_t primary_finger;
    int64_t secondary_finger;
    int start_x;
    int start_y;
    int last_x;
    int last_y;
    uint32_t down_tick;
    int secondary_start_x;
    int secondary_start_y;
    int secondary_last_x;
    int secondary_last_y;
    int two_finger_candidate;
    int two_finger_panned;
} MobileTouchGesture;

static inline int MobileTouchGesture_Abs(int value) {
    return value < 0 ? -value : value;
}

static inline int MobileTouchGesture_MovedPastSlop(MobileTouchGesture const *touch, int x, int y) {
    static const int drag_slop = 8;
    return MobileTouchGesture_Abs(x - touch->start_x) > drag_slop ||
           MobileTouchGesture_Abs(y - touch->start_y) > drag_slop;
}

static inline void MobileTouchGesture_ClearOutput(MobileTouchGestureOutput *out) {
    if (out) {
        out->count = 0;
    }
}

static inline void MobileTouchGesture_Add(
    MobileTouchGestureOutput *out,
    MobileTouchGestureEventType type,
    int x,
    int y,
    int update_cursor)
{
    if (!out || out->count >= (int)(sizeof(out->events) / sizeof(out->events[0]))) {
        return;
    }
    out->events[out->count].type = type;
    out->events[out->count].x = x;
    out->events[out->count].y = y;
    out->events[out->count].update_cursor = update_cursor;
    out->count++;
}

static inline void MobileTouchGesture_Init(MobileTouchGesture *touch) {
    if (!touch) return;
    touch->primary_active = 0;
    touch->secondary_active = 0;
    touch->left_down = 0;
    touch->dragging = 0;
    touch->long_press_sent = 0;
    touch->tap_cancelled = 0;
    touch->primary_finger = 0;
    touch->secondary_finger = 0;
    touch->start_x = 0;
    touch->start_y = 0;
    touch->last_x = 0;
    touch->last_y = 0;
    touch->down_tick = 0;
    touch->secondary_start_x = 0;
    touch->secondary_start_y = 0;
    touch->secondary_last_x = 0;
    touch->secondary_last_y = 0;
    touch->two_finger_candidate = 0;
    touch->two_finger_panned = 0;
}

static inline void MobileTouchGesture_CancelTap(MobileTouchGesture *touch) {
    if (touch) {
        touch->tap_cancelled = 1;
    }
}

static inline void MobileTouchGesture_NotifyPan(MobileTouchGesture *touch) {
    if (touch) {
        touch->two_finger_panned = 1;
        touch->two_finger_candidate = 0;
    }
}

static inline void MobileTouchGesture_NotifyPanMotion(MobileTouchGesture *touch, int64_t finger, int x, int y) {
    if (!touch) return;
    static const int pan_slop = 18;
    if (finger == touch->primary_finger) {
        if (MobileTouchGesture_Abs(x - touch->start_x) > pan_slop ||
            MobileTouchGesture_Abs(y - touch->start_y) > pan_slop) {
            MobileTouchGesture_NotifyPan(touch);
        }
        touch->last_x = x;
        touch->last_y = y;
    } else if (finger == touch->secondary_finger) {
        if (MobileTouchGesture_Abs(x - touch->secondary_start_x) > pan_slop ||
            MobileTouchGesture_Abs(y - touch->secondary_start_y) > pan_slop) {
            MobileTouchGesture_NotifyPan(touch);
        }
        touch->secondary_last_x = x;
        touch->secondary_last_y = y;
    }
}

static inline void MobileTouchGesture_Begin(
    MobileTouchGesture *touch,
    int64_t finger,
    int x,
    int y,
    uint32_t tick,
    MobileTouchGestureOutput *out)
{
    MobileTouchGesture_ClearOutput(out);
    if (!touch) return;

    if (!touch->primary_active) {
        touch->primary_active = 1;
        touch->primary_finger = finger;
        touch->start_x = x;
        touch->start_y = y;
        touch->last_x = x;
        touch->last_y = y;
        touch->down_tick = tick;
        touch->left_down = 0;
        touch->dragging = 0;
        touch->long_press_sent = 0;
        touch->tap_cancelled = 0;
        touch->two_finger_candidate = 0;
        touch->two_finger_panned = 0;
        return;
    }

    if (!touch->secondary_active) {
        int was_minor_drag = (MobileTouchGesture_Abs(touch->last_x - touch->start_x) <= 18 &&
                              MobileTouchGesture_Abs(touch->last_y - touch->start_y) <= 18);
        touch->secondary_active = 1;
        touch->secondary_finger = finger;
        touch->secondary_start_x = x;
        touch->secondary_start_y = y;
        touch->secondary_last_x = x;
        touch->secondary_last_y = y;
        touch->tap_cancelled = 1;
        if (touch->left_down) {
            MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, touch->last_x, touch->last_y, 0);
            touch->left_down = 0;
            touch->dragging = 0;
        }
        if (was_minor_drag && tick >= touch->down_tick && (tick - touch->down_tick <= 350)) {
            touch->two_finger_candidate = 1;
            touch->two_finger_panned = 0;
        } else {
            touch->two_finger_candidate = 0;
        }
    }
}

static inline void MobileTouchGesture_Move(
    MobileTouchGesture *touch,
    int64_t finger,
    int x,
    int y,
    MobileTouchGestureOutput *out)
{
    MobileTouchGesture_ClearOutput(out);
    if (!touch) return;

    if (touch->secondary_active) {
        MobileTouchGesture_NotifyPanMotion(touch, finger, x, y);
        return;
    }

    if (!touch->primary_active || finger != touch->primary_finger) {
        return;
    }

    if (!touch->left_down && !touch->secondary_active && !touch->tap_cancelled && MobileTouchGesture_MovedPastSlop(touch, x, y)) {
        touch->left_down = 1;
        touch->dragging = 1;
        touch->tap_cancelled = 1;
        MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_DOWN, touch->start_x, touch->start_y, 1);
    }
    if (touch->left_down) {
        MobileTouchGesture_Add(out, MOBILE_TOUCH_MOUSE_MOVE, x, y, 1);
    }

    touch->last_x = x;
    touch->last_y = y;
}

static inline void MobileTouchGesture_Update(
    MobileTouchGesture *touch,
    uint32_t tick,
    MobileTouchGestureOutput *out)
{
    static const uint32_t long_press_ms = 650;
    static const uint32_t two_finger_tap_ms = 450;

    MobileTouchGesture_ClearOutput(out);
    if (!touch) return;

    if (touch->two_finger_candidate && (tick - touch->down_tick > two_finger_tap_ms)) {
        touch->two_finger_candidate = 0;
    }

    if (!touch->primary_active || touch->secondary_active ||
        touch->left_down || touch->long_press_sent || touch->tap_cancelled) {
        return;
    }
    if (MobileTouchGesture_MovedPastSlop(touch, touch->last_x, touch->last_y)) {
        return;
    }
    if ((uint32_t)(tick - touch->down_tick) < long_press_ms) {
        return;
    }

    MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_DOWN, touch->last_x, touch->last_y, 0);
    MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_UP, touch->last_x, touch->last_y, 0);
    touch->long_press_sent = 1;
}

static inline int MobileTouchGesture_IsTapEnd(
    MobileTouchGesture const *touch,
    int64_t finger,
    int x,
    int y)
{
    return touch && touch->primary_active && finger == touch->primary_finger &&
           !touch->secondary_active && !touch->left_down && !touch->long_press_sent &&
           !touch->tap_cancelled && !MobileTouchGesture_MovedPastSlop(touch, x, y);
}

static inline void MobileTouchGesture_End(
    MobileTouchGesture *touch,
    int64_t finger,
    int x,
    int y,
    MobileTouchGestureOutput *out)
{
    MobileTouchGesture_ClearOutput(out);
    if (!touch) return;

    if (touch->secondary_active && (finger == touch->primary_finger || finger == touch->secondary_finger)) {
        if (touch->two_finger_candidate && !touch->two_finger_panned) {
            int target_x = (touch->start_x + touch->secondary_start_x) / 2;
            int target_y = (touch->start_y + touch->secondary_start_y) / 2;
            MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_DOWN, target_x, target_y, 0);
            MobileTouchGesture_Add(out, MOBILE_TOUCH_RIGHT_UP, target_x, target_y, 0);
            touch->two_finger_candidate = 0;
            touch->two_finger_panned = 1;
        }

        if (finger == touch->secondary_finger) {
            touch->secondary_active = 0;
            touch->secondary_finger = 0;
        } else {
            touch->primary_finger = touch->secondary_finger;
            touch->start_x = touch->secondary_start_x;
            touch->start_y = touch->secondary_start_y;
            touch->last_x = touch->secondary_last_x;
            touch->last_y = touch->secondary_last_y;
            touch->secondary_active = 0;
            touch->secondary_finger = 0;
        }
        touch->tap_cancelled = 1;
        return;
    }

    if (!touch->primary_active || finger != touch->primary_finger) {
        if (!touch->primary_active && !touch->secondary_active) {
            MobileTouchGesture_Init(touch);
        }
        return;
    }

    if (touch->left_down) {
        if (x != touch->last_x || y != touch->last_y) {
            MobileTouchGesture_Add(out, MOBILE_TOUCH_MOUSE_MOVE, x, y, 1);
        }
        MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, x, y, 0);
    } else if (MobileTouchGesture_IsTapEnd(touch, finger, x, y)) {
        MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_DOWN, x, y, 0);
        MobileTouchGesture_Add(out, MOBILE_TOUCH_LEFT_UP, x, y, 0);
    }

    MobileTouchGesture_Init(touch);
}

static inline int MobileTouchGesture_IsPanFinger(MobileTouchGesture const *touch, int64_t finger) {
    return touch && touch->secondary_active &&
           (finger == touch->primary_finger || finger == touch->secondary_finger);
}

#endif
