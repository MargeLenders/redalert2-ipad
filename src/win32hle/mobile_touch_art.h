#ifndef MOBILE_TOUCH_ART_H
#define MOBILE_TOUCH_ART_H

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include "mobile_touch_hud.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r, g, b, a;
} RAColor;

static const RAColor RA_CHASSIS_BG          = { 22,  26,  32, 230 };
static const RAColor RA_CHASSIS_BG_GRAIN    = { 28,  33,  40, 230 };
static const RAColor RA_CHASSIS_BORDER_DARK = { 12,  15,  18, 255 };
static const RAColor RA_CHASSIS_HIGHLIGHT   = { 108, 116, 126, 220 };
static const RAColor RA_CHASSIS_SHADOW      = { 18,  21,  25, 255 };

static const RAColor RA_STEEL_OUTLINE       = { 14,  17,  20, 255 };
static const RAColor RA_STEEL_LIGHT_OUTER   = { 220, 226, 232, 255 };
static const RAColor RA_STEEL_LIGHT_INNER   = { 170, 178, 186, 255 };
static const RAColor RA_STEEL_SHADOW_OUTER  = { 22,  25,  28, 255 };
static const RAColor RA_STEEL_SHADOW_INNER  = { 48,  54,  60, 255 };
static const RAColor RA_STEEL_FACE_A        = { 115, 122, 130, 255 };
static const RAColor RA_STEEL_FACE_B        = { 126, 134, 142, 255 };
static const RAColor RA_STEEL_FACE_C        = { 102, 109, 117, 255 };
static const RAColor RA_STEEL_PRESSED_FACE  = { 72,  77,  84, 255 };

static const RAColor RA_GOLD_SHEEN          = { 255, 240, 160, 255 };
static const RAColor RA_GOLD_HIGHLIGHT      = { 240, 196,  64, 255 };
static const RAColor RA_GOLD_BODY           = { 190, 142,  36, 255 };
static const RAColor RA_GOLD_SHADOW         = { 110,  78,  18, 255 };
static const RAColor RA_GOLD_OUTLINE        = { 42,   28,   6, 255 };

static const RAColor RA_ALERT_RED           = { 225,  32,  24, 255 };
static const RAColor RA_ALERT_RED_GLOW      = { 255,  80,  70, 255 };
static const RAColor RA_ALERT_RED_PEAK      = { 255, 200, 195, 255 };

static const RAColor RA_LED_GREEN_ON        = { 36,  224,  58, 255 };
static const RAColor RA_LED_GREEN_CORE      = { 180, 255, 190, 255 };
static const RAColor RA_LED_GREEN_OFF       = { 26,   42,  30, 240 };

static const RAColor RA_TACTICAL_AMBER      = { 255, 198,  20, 255 };
static const RAColor RA_TACTICAL_CYAN       = { 90,  210, 255, 255 };
static const RAColor RA_TACTICAL_WHITE      = { 242, 246, 250, 255 };
static const RAColor RA_TACTICAL_SHADOW     = { 16,   19,  22, 255 };

static inline void MobileArt_SetColor(SDL_Renderer *renderer, RAColor c) {
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
}

static inline void MobileArt_DrawThickLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int thickness) {
    if (thickness <= 1) {
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        return;
    }
    int half = thickness / 2;
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            if (dx * dx + dy * dy <= half * half + 1) {
                SDL_RenderDrawLine(renderer, x1 + dx, y1 + dy, x2 + dx, y2 + dy);
            }
        }
    }
}

static inline void MobileArt_DrawRivet(SDL_Renderer *renderer, int cx, int cy, int radius) {
    if (radius < 2) radius = 2;
    MobileArt_SetColor(renderer, RA_STEEL_OUTLINE);
    SDL_Rect r_base = { cx - radius, cy - radius, radius * 2 + 1, radius * 2 + 1 };
    SDL_RenderFillRect(renderer, &r_base);

    MobileArt_SetColor(renderer, RA_STEEL_FACE_A);
    SDL_Rect r_body = { cx - radius + 1, cy - radius + 1, (radius - 1) * 2 + 1, (radius - 1) * 2 + 1 };
    SDL_RenderFillRect(renderer, &r_body);

    MobileArt_SetColor(renderer, RA_STEEL_LIGHT_OUTER);
    SDL_RenderDrawLine(renderer, cx - radius + 1, cy - radius + 1, cx + radius - 1, cy - radius + 1);
    SDL_RenderDrawLine(renderer, cx - radius + 1, cy - radius + 1, cx - radius + 1, cy + radius - 1);

    MobileArt_SetColor(renderer, RA_STEEL_SHADOW_OUTER);
    SDL_RenderDrawLine(renderer, cx - radius + 1, cy + radius - 1, cx + radius - 1, cy + radius - 1);
    SDL_RenderDrawLine(renderer, cx + radius - 1, cy - radius + 1, cx + radius - 1, cy + radius - 1);

    MobileArt_SetColor(renderer, RA_STEEL_OUTLINE);
    int slot_len = radius - 1;
    if (slot_len < 1) slot_len = 1;
    SDL_RenderDrawLine(renderer, cx - slot_len, cy, cx + slot_len, cy);
}

static inline void MobileArt_DrawDockChassis(SDL_Renderer *renderer, SDL_Rect dock, int scale) {
    int bevel = (scale >= 3) ? 2 : 1;
    int rivet_rad = (scale >= 3) ? 3 : 2;
    int rivet_inset = rivet_rad + bevel + 1;

    MobileArt_SetColor(renderer, RA_CHASSIS_BORDER_DARK);
    SDL_RenderDrawRect(renderer, &dock);

    SDL_Rect inside = { dock.x + 1, dock.y + 1, dock.w - 2, dock.h - 2 };
    MobileArt_SetColor(renderer, RA_CHASSIS_BG);
    SDL_RenderFillRect(renderer, &inside);

    MobileArt_SetColor(renderer, RA_CHASSIS_BG_GRAIN);
    for (int y = inside.y; y < inside.y + inside.h; y += 3) {
        SDL_RenderDrawLine(renderer, inside.x, y, inside.x + inside.w - 1, y);
    }

    MobileArt_SetColor(renderer, RA_CHASSIS_HIGHLIGHT);
    for (int b = 0; b < bevel; ++b) {
        SDL_RenderDrawLine(renderer, inside.x + b, inside.y + b, inside.x + inside.w - 1 - b, inside.y + b);
        SDL_RenderDrawLine(renderer, inside.x + inside.w - 1 - b, inside.y + b, inside.x + inside.w - 1 - b, inside.y + inside.h - 1 - b);
    }
    MobileArt_SetColor(renderer, RA_CHASSIS_SHADOW);
    for (int b = 0; b < bevel; ++b) {
        SDL_RenderDrawLine(renderer, inside.x + b, inside.y + inside.h - 1 - b, inside.x + inside.w - 1 - b, inside.y + inside.h - 1 - b);
        SDL_RenderDrawLine(renderer, inside.x + b, inside.y + b, inside.x + b, inside.y + inside.h - 1 - b);
    }

    MobileArt_DrawRivet(renderer, dock.x + rivet_inset, dock.y + rivet_inset, rivet_rad);
    MobileArt_DrawRivet(renderer, dock.x + dock.w - rivet_inset, dock.y + rivet_inset, rivet_rad);
    MobileArt_DrawRivet(renderer, dock.x + rivet_inset, dock.y + dock.h - rivet_inset, rivet_rad);
    MobileArt_DrawRivet(renderer, dock.x + dock.w - rivet_inset, dock.y + dock.h - rivet_inset, rivet_rad);
}

static inline void MobileArt_DrawButtonPlate(SDL_Renderer *renderer, SDL_Rect rect, int pressed, int active, int scale) {
    int bevel = (scale >= 3) ? 2 : 1;

    MobileArt_SetColor(renderer, RA_STEEL_OUTLINE);
    SDL_RenderDrawRect(renderer, &rect);

    SDL_Rect body = { rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2 };

    if (pressed) {
        MobileArt_SetColor(renderer, RA_STEEL_PRESSED_FACE);
        SDL_RenderFillRect(renderer, &body);

        MobileArt_SetColor(renderer, RA_STEEL_SHADOW_OUTER);
        for (int b = 0; b < bevel; ++b) {
            SDL_RenderDrawLine(renderer, body.x + b, body.y + b, body.x + body.w - 1 - b, body.y + b);
            SDL_RenderDrawLine(renderer, body.x + b, body.y + b, body.x + b, body.y + body.h - 1 - b);
        }

        MobileArt_SetColor(renderer, RA_STEEL_FACE_A);
        for (int b = 0; b < bevel; ++b) {
            SDL_RenderDrawLine(renderer, body.x + b, body.y + body.h - 1 - b, body.x + body.w - 1 - b, body.y + body.h - 1 - b);
            SDL_RenderDrawLine(renderer, body.x + body.w - 1 - b, body.y + b, body.x + body.w - 1 - b, body.y + body.h - 1 - b);
        }
    } else {
        for (int y = body.y; y < body.y + body.h; ++y) {
            int grain = y % 3;
            if (grain == 0) MobileArt_SetColor(renderer, RA_STEEL_FACE_A);
            else if (grain == 1) MobileArt_SetColor(renderer, RA_STEEL_FACE_B);
            else MobileArt_SetColor(renderer, RA_STEEL_FACE_C);
            SDL_RenderDrawLine(renderer, body.x, y, body.x + body.w - 1, y);
        }

        MobileArt_SetColor(renderer, RA_STEEL_LIGHT_OUTER);
        SDL_RenderDrawLine(renderer, body.x, body.y, body.x + body.w - 1, body.y);
        SDL_RenderDrawLine(renderer, body.x, body.y, body.x, body.y + body.h - 1);

        MobileArt_SetColor(renderer, RA_STEEL_SHADOW_OUTER);
        SDL_RenderDrawLine(renderer, body.x, body.y + body.h - 1, body.x + body.w - 1, body.y + body.h - 1);
        SDL_RenderDrawLine(renderer, body.x + body.w - 1, body.y, body.x + body.w - 1, body.y + body.h - 1);
    }

    if (active) {
        MobileArt_SetColor(renderer, RA_TACTICAL_AMBER);
        SDL_Rect active_rect = { body.x + bevel, body.y + bevel, body.w - bevel * 2, body.h - bevel * 2 };
        SDL_RenderDrawRect(renderer, &active_rect);
    }
}

static inline void MobileArt_DrawWestwoodArrow(SDL_Renderer *renderer, SDL_Rect rect, int point_right, int pressed, int scale) {
    int cx = rect.x + rect.w / 2 + (pressed ? 1 : 0);
    int cy = rect.y + rect.h / 2 + (pressed ? 1 : 0);
    int aw = (rect.w * 9) / 20;
    int ah = (rect.h * 11) / 20;
    if (aw < 6) aw = 6;
    if (ah < 8) ah = 8;
    int half_w = aw / 2;
    int half_h = ah / 2;
    int line_thick = (scale >= 3) ? 2 : 1;

    if (point_right) {
        int tip_x = cx + half_w;
        int base_x = cx - half_w;
        int notch_x = cx - half_w / 3;

        MobileArt_SetColor(renderer, RA_GOLD_BODY);
        for (int y = -half_h; y <= half_h; ++y) {
            int span = (int)(((long long)(half_h - (y < 0 ? -y : y)) * aw) / half_h);
            int cur_tip = base_x + span;
            int cur_back = base_x + (notch_x - base_x) * (half_h - (y < 0 ? -y : y)) / half_h;
            if (cur_tip >= cur_back) {
                SDL_RenderDrawLine(renderer, cur_back, cy + y, cur_tip, cy + y);
            }
        }

        MobileArt_SetColor(renderer, RA_GOLD_SHEEN);
        MobileArt_DrawThickLine(renderer, base_x, cy - half_h, tip_x, cy, line_thick);
        MobileArt_SetColor(renderer, RA_GOLD_SHADOW);
        MobileArt_DrawThickLine(renderer, tip_x, cy, base_x, cy + half_h, line_thick);
    } else {
        int tip_x = cx - half_w;
        int base_x = cx + half_w;
        int notch_x = cx + half_w / 3;

        MobileArt_SetColor(renderer, RA_GOLD_BODY);
        for (int y = -half_h; y <= half_h; ++y) {
            int span = (int)(((long long)(half_h - (y < 0 ? -y : y)) * aw) / half_h);
            int cur_tip = base_x - span;
            int cur_back = base_x - (base_x - notch_x) * (half_h - (y < 0 ? -y : y)) / half_h;
            if (cur_back >= cur_tip) {
                SDL_RenderDrawLine(renderer, cur_tip, cy + y, cur_back, cy + y);
            }
        }

        MobileArt_SetColor(renderer, RA_GOLD_SHEEN);
        MobileArt_DrawThickLine(renderer, tip_x, cy, base_x, cy - half_h, line_thick);
        MobileArt_SetColor(renderer, RA_GOLD_SHADOW);
        MobileArt_DrawThickLine(renderer, base_x, cy + half_h, tip_x, cy, line_thick);
    }
}

static inline void MobileArt_DrawPixelGlyph(SDL_Renderer *renderer, int ox, int oy, const unsigned char matrix[7], RAColor face_color, int pixel_size) {
    int shadow_offset = (pixel_size >= 2) ? 2 : 1;

    MobileArt_SetColor(renderer, RA_TACTICAL_SHADOW);
    for (int r = 0; r < 7; ++r) {
        unsigned char row = matrix[r];
        for (int c = 0; c < 5; ++c) {
            if (row & (1 << (4 - c))) {
                SDL_Rect sp = { ox + c * pixel_size + shadow_offset, oy + r * pixel_size + shadow_offset, pixel_size, pixel_size };
                SDL_RenderFillRect(renderer, &sp);
            }
        }
    }

    MobileArt_SetColor(renderer, face_color);
    for (int r = 0; r < 7; ++r) {
        unsigned char row = matrix[r];
        for (int c = 0; c < 5; ++c) {
            if (row & (1 << (4 - c))) {
                SDL_Rect fp = { ox + c * pixel_size, oy + r * pixel_size, pixel_size, pixel_size };
                SDL_RenderFillRect(renderer, &fp);
            }
        }
    }
}

static inline void MobileArt_DrawSquadDigit(SDL_Renderer *renderer, SDL_Rect rect, int digit, int assigned, int active, int pressed, int scale) {
    static const unsigned char GLYPH_1[7] = { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E };
    static const unsigned char GLYPH_2[7] = { 0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F };
    static const unsigned char GLYPH_3[7] = { 0x1E, 0x01, 0x01, 0x06, 0x01, 0x01, 0x1E };
    static const unsigned char GLYPH_4[7] = { 0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01 };

    const unsigned char *matrix = GLYPH_1;
    if (digit == 2) matrix = GLYPH_2;
    else if (digit == 3) matrix = GLYPH_3;
    else if (digit == 4) matrix = GLYPH_4;

    int pixel_size = rect.h / 14;
    if (pixel_size < 2) pixel_size = (scale >= 3) ? 3 : (scale == 2 ? 2 : 1);
    int gw = 5 * pixel_size;
    int gh = 7 * pixel_size;
    int ox = rect.x + (rect.w - gw) / 2 + (pressed ? 1 : 0);
    int oy = rect.y + (rect.h - gh) / 2 + (pressed ? 1 : 0);

    RAColor color = active ? RA_TACTICAL_AMBER : RA_TACTICAL_WHITE;
    MobileArt_DrawPixelGlyph(renderer, ox, oy, matrix, color, pixel_size);

    int led_sz = (rect.h >= 60) ? 10 : ((scale >= 3) ? 6 : 4);
    int led_x = rect.x + rect.w - led_sz - (rect.h >= 60 ? 8 : 4) + (pressed ? 1 : 0);
    int led_y = rect.y + (rect.h >= 60 ? 8 : 4) + (pressed ? 1 : 0);

    MobileArt_SetColor(renderer, RA_STEEL_OUTLINE);
    SDL_Rect led_socket = { led_x - 1, led_y - 1, led_sz + 2, led_sz + 2 };
    SDL_RenderDrawRect(renderer, &led_socket);

    if (assigned) {
        MobileArt_SetColor(renderer, RA_LED_GREEN_ON);
        SDL_Rect led_core = { led_x, led_y, led_sz, led_sz };
        SDL_RenderFillRect(renderer, &led_core);

        MobileArt_SetColor(renderer, RA_LED_GREEN_CORE);
        SDL_Rect led_pip = { led_x + 1, led_y + 1, led_sz / 2, led_sz / 2 };
        SDL_RenderFillRect(renderer, &led_pip);
    } else {
        MobileArt_SetColor(renderer, RA_LED_GREEN_OFF);
        SDL_Rect led_dark = { led_x, led_y, led_sz, led_sz };
        SDL_RenderFillRect(renderer, &led_dark);
    }
}

static inline void MobileArt_DrawForceAttackReticle(SDL_Renderer *renderer, SDL_Rect rect, int active, int pressed, int scale) {
    int cx = rect.x + rect.w / 2 + (pressed ? 1 : 0);
    int cy = rect.y + rect.h / 2 + (pressed ? 1 : 0);
    int r = (rect.h * 7) / 24;
    if (r < 5) r = 5;
    int line_thick = (rect.h >= 60) ? 3 : ((scale >= 3) ? 2 : 1);

    RAColor ring_color = active ? RA_ALERT_RED_GLOW : RA_STEEL_LIGHT_INNER;
    RAColor cross_color = active ? RA_ALERT_RED : RA_STEEL_FACE_A;
    RAColor core_color = active ? RA_ALERT_RED_PEAK : RA_ALERT_RED;

    MobileArt_SetColor(renderer, ring_color);
    MobileArt_DrawThickLine(renderer, cx - r, cy - r / 2, cx - r / 2, cy - r, line_thick);
    MobileArt_DrawThickLine(renderer, cx + r / 2, cy - r, cx + r, cy - r / 2, line_thick);
    MobileArt_DrawThickLine(renderer, cx + r, cy + r / 2, cx + r / 2, cy + r, line_thick);
    MobileArt_DrawThickLine(renderer, cx - r / 2, cy + r, cx - r, cy + r / 2, line_thick);

    MobileArt_SetColor(renderer, cross_color);
    int tick_len = r / 2;
    if (tick_len < 2) tick_len = 2;
    MobileArt_DrawThickLine(renderer, cx, cy - r - tick_len, cx, cy - r + 1, line_thick);
    MobileArt_DrawThickLine(renderer, cx, cy + r - 1, cx, cy + r + tick_len, line_thick);
    MobileArt_DrawThickLine(renderer, cx - r - tick_len, cy, cx - r + 1, cy, line_thick);
    MobileArt_DrawThickLine(renderer, cx + r - 1, cy, cx + r + tick_len, cy, line_thick);

    MobileArt_SetColor(renderer, core_color);
    int dot_sz = (rect.h >= 60) ? 5 : ((scale >= 3) ? 3 : 2);
    SDL_Rect dot = { cx - dot_sz / 2, cy - dot_sz / 2, dot_sz, dot_sz };
    SDL_RenderFillRect(renderer, &dot);
}

static inline void MobileArt_DrawTacticalLetter(SDL_Renderer *renderer, SDL_Rect rect, char ch, int active, int pressed, int scale) {
    static const unsigned char GLYPH_S[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
    static const unsigned char GLYPH_X[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 };
    static const unsigned char GLYPH_D[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E };
    static const unsigned char GLYPH_M[7] = { 0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11 };

    const unsigned char *matrix = GLYPH_S;
    if (ch == 'X' || ch == 'x') matrix = GLYPH_X;
    else if (ch == 'D' || ch == 'd') matrix = GLYPH_D;
    else if (ch == 'M' || ch == 'm') matrix = GLYPH_M;

    int pixel_size = rect.h / 14;
    if (pixel_size < 2) pixel_size = (scale >= 3) ? 3 : (scale == 2 ? 2 : 1);
    int gw = 5 * pixel_size;
    int gh = 7 * pixel_size;
    int ox = rect.x + (rect.w - gw) / 2 + (pressed ? 1 : 0);
    int oy = rect.y + (rect.h - gh) / 2 + (pressed ? 1 : 0);

    RAColor color = active ? RA_TACTICAL_AMBER : RA_TACTICAL_WHITE;
    if (ch == 'X' || ch == 'x') color = active ? RA_TACTICAL_CYAN : RA_TACTICAL_CYAN;
    else if (ch == 'M' || ch == 'm') color = active ? RA_TACTICAL_AMBER : RA_GOLD_HIGHLIGHT;

    MobileArt_DrawPixelGlyph(renderer, ox, oy, matrix, color, pixel_size);
}

static inline void MobileTouchHud_Draw(SDL_Renderer *renderer, MobileTouchHUD const *hud, int scale) {
    if (!renderer || !hud) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (hud->collapsed) {
        int x, y, w, h;
        if (MobileTouchHud_GetButtonRect(hud, HUD_BTN_TOGGLE, &x, &y, &w, &h)) {
            SDL_Rect r = { x, y, w, h };
            int pressed = (hud->active_button == HUD_BTN_TOGGLE);
            MobileArt_DrawButtonPlate(renderer, r, pressed, 0, scale);
            MobileArt_DrawWestwoodArrow(renderer, r, 1, pressed, scale);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        return;
    }

    int x0, y0, w0, h0, x1, y1, w1, h1;
    if (MobileTouchHud_GetButtonRect(hud, 0, &x0, &y0, &w0, &h0) &&
        MobileTouchHud_GetButtonRect(hud, HUD_BTN_COUNT - 1, &x1, &y1, &w1, &h1)) {
        int dock_pad = (h0 >= 60) ? 12 : 6;
        SDL_Rect dock = {
            x0 - dock_pad,
            y0 - dock_pad,
            w0 + dock_pad * 2,
            (y1 + h1) - y0 + dock_pad * 2
        };
        MobileArt_DrawDockChassis(renderer, dock, scale);
    }

    for (int id = 0; id < HUD_BTN_COUNT; ++id) {
        int bx, by, bw, bh;
        if (!MobileTouchHud_GetButtonRect(hud, id, &bx, &by, &bw, &bh)) continue;

        SDL_Rect r = { bx, by, bw, bh };
        int pressed = (hud->active_button == id);
        int active = 0;

        if (id == HUD_BTN_FORCE_ATTACK) active = MobileTouchHud_IsForceAttackActive(hud);
        else if (id == HUD_BTN_FORCE_MOVE) active = MobileTouchHud_IsForceMoveActive(hud);

        MobileArt_DrawButtonPlate(renderer, r, pressed, active, scale);

        switch (id) {
            case HUD_BTN_TOGGLE:
                MobileArt_DrawWestwoodArrow(renderer, r, 0, pressed, scale);
                break;
            case HUD_BTN_FORCE_ATTACK:
                MobileArt_DrawForceAttackReticle(renderer, r, active, pressed, scale);
                break;
            case HUD_BTN_FORCE_MOVE:
                MobileArt_DrawTacticalLetter(renderer, r, 'M', active, pressed, scale);
                break;
            case HUD_BTN_DEPLOY:
                MobileArt_DrawTacticalLetter(renderer, r, 'D', 0, pressed, scale);
                break;
            case HUD_BTN_GROUP_1:
            case HUD_BTN_GROUP_2:
            case HUD_BTN_GROUP_3:
            case HUD_BTN_GROUP_4: {
                int grp = id - HUD_BTN_GROUP_1 + 1;
                int assigned = hud->group_assigned[grp - 1];
                MobileArt_DrawSquadDigit(renderer, r, grp, assigned, active, pressed, scale);
                break;
            }
            case HUD_BTN_STOP:
                MobileArt_DrawTacticalLetter(renderer, r, 'S', 0, pressed, scale);
                break;
            case HUD_BTN_SCATTER:
                MobileArt_DrawTacticalLetter(renderer, r, 'X', 0, pressed, scale);
                break;
            default:
                break;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

#ifdef __cplusplus
}
#endif

#endif /* MOBILE_TOUCH_ART_H */
