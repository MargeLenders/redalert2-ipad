#ifndef MOBILE_TOUCH_HUD_H
#define MOBILE_TOUCH_HUD_H

#include <stdint.h>

#ifndef VK_CONTROL
#define VK_CONTROL 0x11
#endif
#ifndef VK_MENU
#define VK_MENU    0x12
#endif

typedef enum {
    HUD_BTN_NONE = -1,
    HUD_BTN_TOGGLE = 0,
    HUD_BTN_FORCE_ATTACK,
    HUD_BTN_FORCE_MOVE,
    HUD_BTN_DEPLOY,
    HUD_BTN_GROUP_1,
    HUD_BTN_GROUP_2,
    HUD_BTN_GROUP_3,
    HUD_BTN_GROUP_4,
    HUD_BTN_STOP,
    HUD_BTN_SCATTER,
    HUD_BTN_COUNT
} MobileTouchHudButtonId;

typedef enum {
    HUD_ACTION_NONE = 0,
    HUD_ACTION_FORCE_ATTACK_CHANGED,
    HUD_ACTION_FORCE_MOVE_CHANGED,
    HUD_ACTION_GROUP_SELECT,
    HUD_ACTION_GROUP_ASSIGN,
    HUD_ACTION_GROUP_CENTER,
    HUD_ACTION_KEY_PULSE
} MobileTouchHudActionType;

typedef struct {
    MobileTouchHudActionType type;
    int vk;
    int param;
} MobileTouchHudAction;

typedef struct {
    int collapsed;
    int active_button;
    int64_t active_finger;
    int finger_inside;
    uint32_t down_tick;
    int long_press_triggered;
    int force_attack_held;
    int force_attack_latched;
    int force_move_held;
    int force_move_latched;
    int group_assigned[4];
    uint32_t last_group_tap_tick[4];
    int last_tapped_group;
    int screen_w;
    int screen_h;
    int margin_x;
    int safe_area_left;
} MobileTouchHUD;

static inline void MobileTouchHud_Init(MobileTouchHUD *hud) {
    if (!hud) return;
    hud->collapsed = 0;
    hud->active_button = HUD_BTN_NONE;
    hud->active_finger = 0;
    hud->finger_inside = 0;
    hud->down_tick = 0;
    hud->long_press_triggered = 0;
    hud->force_attack_held = 0;
    hud->force_attack_latched = 0;
    hud->force_move_held = 0;
    hud->force_move_latched = 0;
    for (int i = 0; i < 4; ++i) {
        hud->group_assigned[i] = 0;
        hud->last_group_tap_tick[i] = 0;
    }
    hud->last_tapped_group = -1;
    hud->screen_w = 1280;
    hud->screen_h = 960;
    hud->margin_x = 0;
    hud->safe_area_left = 0;
}

static inline void MobileTouchHud_SetScreenSize(MobileTouchHUD *hud, int w, int h) {
    if (!hud) return;
    if (w > 0) hud->screen_w = w;
    if (h > 0) hud->screen_h = h;
}

static inline void MobileTouchHud_SetLayoutEx(MobileTouchHUD *hud, int w, int h, int margin_x, int safe_area_left) {
    if (!hud) return;
    if (w > 0) hud->screen_w = w;
    if (h > 0) hud->screen_h = h;
    hud->margin_x = margin_x > 0 ? margin_x : 0;
    hud->safe_area_left = safe_area_left > 0 ? safe_area_left : 0;
}

static inline void MobileTouchHud_SetLayout(MobileTouchHUD *hud, int w, int h, int margin_x) {
    MobileTouchHud_SetLayoutEx(hud, w, h, margin_x, 0);
}

static inline int MobileTouchHud_GetButtonRect(
    MobileTouchHUD const *hud,
    int button_id,
    int *x,
    int *y,
    int *w,
    int *h)
{
    if (!hud || button_id < 0 || button_id >= HUD_BTN_COUNT) return 0;

    int sh = hud->screen_h > 0 ? hud->screen_h : 960;
    int sw = hud->screen_w > 0 ? hud->screen_w : 1280;
    int ui_scale = (sh >= 1400 || (sw > 2000 && sh > 1000)) ? 2 : 1;

    if (hud->collapsed) {
        if (button_id == HUD_BTN_TOGGLE) {
            int bw = 34 * ui_scale;
            int bh = 52 * ui_scale;
            int bx;
            if (hud->margin_x > 0) bx = (hud->margin_x - bw) / 2;
            else if (hud->safe_area_left > 0) bx = hud->safe_area_left + 6 * ui_scale;
            else bx = 10 * ui_scale;
            if (x) *x = bx;
            if (y) *y = (sh - bh) / 2;
            if (w) *w = bw;
            if (h) *h = bh;
            return 1;
        }
        return 0;
    }

    int bw = 48 * ui_scale;
    int bh = 38 * ui_scale;
    int pad = 5 * ui_scale;
    int total_h = HUD_BTN_COUNT * bh + (HUD_BTN_COUNT - 1) * pad;
    int start_y = (sh - total_h) / 2;
    if (start_y < 12 * ui_scale) start_y = 12 * ui_scale;

    int bx;
    if (hud->margin_x > 0) bx = (hud->margin_x - bw) / 2;
    else if (hud->safe_area_left > 0) bx = hud->safe_area_left + 8 * ui_scale;
    else bx = 12 * ui_scale;
    int by = start_y + button_id * (bh + pad);

    if (x) *x = bx;
    if (y) *y = by;
    if (w) *w = bw;
    if (h) *h = bh;
    return 1;
}

static inline int MobileTouchHud_HitTest(MobileTouchHUD const *hud, int x, int y) {
    if (!hud) return HUD_BTN_NONE;

    int sh = hud->screen_h > 0 ? hud->screen_h : 960;
    int ui_scale = (sh >= 1400) ? 2 : 1;

    if (hud->collapsed) {
        int bx, by, bw, bh;
        if (MobileTouchHud_GetButtonRect(hud, HUD_BTN_TOGGLE, &bx, &by, &bw, &bh)) {
            int slop = 16 * ui_scale;
            if (x >= bx - slop && x < bx + bw + slop && y >= by - slop && y < by + bh + slop) {
                return HUD_BTN_TOGGLE;
            }
        }
        return HUD_BTN_NONE;
    }

    int slop = 10 * ui_scale;
    for (int id = 0; id < HUD_BTN_COUNT; ++id) {
        int bx, by, bw, bh;
        if (MobileTouchHud_GetButtonRect(hud, id, &bx, &by, &bw, &bh)) {
            if (x >= bx - slop && x < bx + bw + slop && y >= by - slop && y < by + bh + slop) {
                return id;
            }
        }
    }
    return HUD_BTN_NONE;
}

static inline int MobileTouchHud_IsFingerActive(MobileTouchHUD const *hud, int64_t finger) {
    return hud && hud->active_button != HUD_BTN_NONE && hud->active_finger == finger;
}

static inline int MobileTouchHud_IsForceAttackActive(MobileTouchHUD const *hud) {
    return hud && (hud->force_attack_held || hud->force_attack_latched);
}

static inline int MobileTouchHud_IsForceMoveActive(MobileTouchHUD const *hud) {
    return hud && (hud->force_move_held || hud->force_move_latched);
}

static inline void MobileTouchHud_ClearForceAttackLatch(MobileTouchHUD *hud) {
    if (hud) hud->force_attack_latched = 0;
}

static inline void MobileTouchHud_ClearForceMoveLatch(MobileTouchHUD *hud) {
    if (hud) hud->force_move_latched = 0;
}

static inline void MobileTouchHud_FingerDown(
    MobileTouchHUD *hud,
    int64_t finger,
    int x,
    int y,
    uint32_t tick,
    MobileTouchHudAction *action)
{
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (!hud) return;

    int hit = MobileTouchHud_HitTest(hud, x, y);
    if (hit == HUD_BTN_NONE) return;

    if (hit == HUD_BTN_TOGGLE) {
        hud->collapsed = !hud->collapsed;
        hud->active_button = hit;
        hud->active_finger = finger;
        hud->finger_inside = 1;
        hud->down_tick = tick;
        hud->long_press_triggered = 0;
        return;
    }

    hud->active_button = hit;
    hud->active_finger = finger;
    hud->finger_inside = 1;
    hud->down_tick = tick;
    hud->long_press_triggered = 0;
}

static inline void MobileTouchHud_FingerMove(MobileTouchHUD *hud, int64_t finger, int x, int y) {
    if (!hud || hud->active_button == HUD_BTN_NONE || hud->active_finger != finger) return;
    int hit = MobileTouchHud_HitTest(hud, x, y);
    hud->finger_inside = (hit == hud->active_button);
}

static inline void MobileTouchHud_Update(MobileTouchHUD *hud, uint32_t tick, MobileTouchHudAction *action) {
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (!hud || hud->active_button == HUD_BTN_NONE || hud->long_press_triggered) return;

    int btn = hud->active_button;

    if (btn == HUD_BTN_FORCE_ATTACK) {
        if (tick - hud->down_tick > 300) {
            hud->force_attack_held = 1;
            hud->force_attack_latched = 0;
            hud->long_press_triggered = 1;
            if (action) action->type = HUD_ACTION_FORCE_ATTACK_CHANGED;
        }
    } else if (btn == HUD_BTN_FORCE_MOVE) {
        if (tick - hud->down_tick > 300) {
            hud->force_move_held = 1;
            hud->force_move_latched = 0;
            hud->long_press_triggered = 1;
            if (action) action->type = HUD_ACTION_FORCE_MOVE_CHANGED;
        }
    } else if (btn >= HUD_BTN_GROUP_1 && btn <= HUD_BTN_GROUP_4) {
        if (tick - hud->down_tick >= 300) {
            int grp = btn - HUD_BTN_GROUP_1 + 1;
            hud->group_assigned[grp - 1] = 1;
            hud->long_press_triggered = 1;
            if (action) {
                action->type = HUD_ACTION_GROUP_ASSIGN;
                action->param = grp;
            }
        }
    }
}

static inline void MobileTouchHud_FingerUp(
    MobileTouchHUD *hud,
    int64_t finger,
    int x,
    int y,
    uint32_t tick,
    MobileTouchHudAction *action)
{
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (!hud || hud->active_button == HUD_BTN_NONE || hud->active_finger != finger) return;

    int btn = hud->active_button;
    int inside = hud->finger_inside || (MobileTouchHud_HitTest(hud, x, y) == btn);

    if (btn == HUD_BTN_FORCE_ATTACK) {
        if (hud->force_attack_held) {
            hud->force_attack_held = 0;
            if (action) action->type = HUD_ACTION_FORCE_ATTACK_CHANGED;
        } else if (inside && !hud->long_press_triggered) {
            hud->force_attack_latched = !hud->force_attack_latched;
            if (action) action->type = HUD_ACTION_FORCE_ATTACK_CHANGED;
        }
    } else if (btn == HUD_BTN_FORCE_MOVE) {
        if (hud->force_move_held) {
            hud->force_move_held = 0;
            if (action) action->type = HUD_ACTION_FORCE_MOVE_CHANGED;
        } else if (inside && !hud->long_press_triggered) {
            hud->force_move_latched = !hud->force_move_latched;
            if (action) action->type = HUD_ACTION_FORCE_MOVE_CHANGED;
        }
    } else if (btn == HUD_BTN_DEPLOY) {
        if (inside) {
            if (action) {
                action->type = HUD_ACTION_KEY_PULSE;
                action->vk = 'D';
            }
        }
    } else if (btn == HUD_BTN_STOP) {
        if (inside) {
            if (action) {
                action->type = HUD_ACTION_KEY_PULSE;
                action->vk = 'S';
            }
        }
    } else if (btn == HUD_BTN_SCATTER) {
        if (inside) {
            if (action) {
                action->type = HUD_ACTION_KEY_PULSE;
                action->vk = 'X';
            }
        }
    } else if (btn >= HUD_BTN_GROUP_1 && btn <= HUD_BTN_GROUP_4) {
        if (inside && !hud->long_press_triggered) {
            int grp = btn - HUD_BTN_GROUP_1 + 1;
            if (hud->last_tapped_group == grp && (tick - hud->last_group_tap_tick[grp - 1] < 400)) {
                if (action) {
                    action->type = HUD_ACTION_GROUP_CENTER;
                    action->param = grp;
                }
                hud->last_tapped_group = -1;
            } else {
                if (action) {
                    action->type = HUD_ACTION_GROUP_SELECT;
                    action->param = grp;
                }
                hud->last_tapped_group = grp;
                hud->last_group_tap_tick[grp - 1] = tick;
            }
        }
    }

    hud->active_button = HUD_BTN_NONE;
    hud->active_finger = 0;
    hud->finger_inside = 0;
    hud->down_tick = 0;
    hud->long_press_triggered = 0;
}

static inline void MobileTouchHud_OnAttackFired(MobileTouchHUD *hud, MobileTouchHudAction *action) {
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (hud && hud->force_attack_latched) {
        hud->force_attack_latched = 0;
        if (action) action->type = HUD_ACTION_FORCE_ATTACK_CHANGED;
    }
}

static inline void MobileTouchHud_OnMoveFired(MobileTouchHUD *hud, MobileTouchHudAction *action) {
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (hud && hud->force_move_latched) {
        hud->force_move_latched = 0;
        if (action) action->type = HUD_ACTION_FORCE_MOVE_CHANGED;
    }
}

static inline void MobileTouchHud_Cancel(MobileTouchHUD *hud, MobileTouchHudAction *action) {
    if (action) {
        action->type = HUD_ACTION_NONE;
        action->vk = 0;
        action->param = 0;
    }
    if (!hud) return;
    int had_attack = MobileTouchHud_IsForceAttackActive(hud);
    int had_move = MobileTouchHud_IsForceMoveActive(hud);
    hud->force_attack_held = 0;
    hud->force_attack_latched = 0;
    hud->force_move_held = 0;
    hud->force_move_latched = 0;
    hud->active_button = HUD_BTN_NONE;
    hud->active_finger = 0;
    hud->finger_inside = 0;
    hud->down_tick = 0;
    hud->long_press_triggered = 0;
    if (had_attack && action) {
        action->type = HUD_ACTION_FORCE_ATTACK_CHANGED;
    } else if (had_move && action) {
        action->type = HUD_ACTION_FORCE_MOVE_CHANGED;
    }
}

#endif
