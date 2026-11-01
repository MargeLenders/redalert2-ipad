#ifndef MOBILE_PAN_H
#define MOBILE_PAN_H

#include <stdint.h>
#include <math.h>

#ifndef VK_LEFT
#define VK_LEFT   0x25
#define VK_UP     0x26
#define VK_RIGHT  0x27
#define VK_DOWN   0x28
#endif

typedef struct {
    /* Accumulators for sub-pixel deltas or discrete fallback */
    float accumulator_x;
    float accumulator_y;
    uint32_t last_emit_tick;

    /* Drag tracking */
    int is_panning;
    float velocity_x;            /* game pixels per ms */
    float velocity_y;            /* game pixels per ms */
    uint32_t last_move_tick;

    /* Inertia coasting */
    int is_coasting;
    float inertia_vx;            /* game pixels per ms */
    float inertia_vy;            /* game pixels per ms */
    uint32_t last_inertia_tick;
    uint32_t inertia_start_tick;
} MobilePanState;

typedef int  (*MobilePanEmitDelta)(void *context, float dx, float dy);
typedef void (*MobilePanEmitKey)(void *context, int vk);

static inline void MobilePan_Init(MobilePanState *pan) {
    if (!pan) return;
    pan->accumulator_x = 0.0f;
    pan->accumulator_y = 0.0f;
    pan->last_emit_tick = 0;
    pan->is_panning = 0;
    pan->velocity_x = 0.0f;
    pan->velocity_y = 0.0f;
    pan->last_move_tick = 0;
    pan->is_coasting = 0;
    pan->inertia_vx = 0.0f;
    pan->inertia_vy = 0.0f;
    pan->last_inertia_tick = 0;
    pan->inertia_start_tick = 0;
}

static inline void MobilePan_Reset(MobilePanState *pan) {
    MobilePan_Init(pan);
}

static inline void MobilePan_StopInertia(MobilePanState *pan) {
    if (!pan) return;
    pan->is_coasting = 0;
    pan->inertia_vx = 0.0f;
    pan->inertia_vy = 0.0f;
    pan->velocity_x = 0.0f;
    pan->velocity_y = 0.0f;
}

static inline int MobilePan_IsCoasting(const MobilePanState *pan) {
    return pan ? pan->is_coasting : 0;
}

static inline void MobilePan_BeginDrag(MobilePanState *pan, uint32_t tick) {
    if (!pan) return;
    MobilePan_StopInertia(pan);
    pan->is_panning = 1;
    pan->accumulator_x = 0.0f;
    pan->accumulator_y = 0.0f;
    pan->velocity_x = 0.0f;
    pan->velocity_y = 0.0f;
    pan->last_move_tick = tick;
}

static inline void MobilePan_EmitKey(MobilePanEmitKey emit_key, void *context, int vk) {
    if (emit_key) {
        emit_key(context, vk);
    }
}

static inline void MobilePan_OnMove(
    MobilePanState *pan,
    float dx_game,
    float dy_game,
    uint32_t tick,
    MobilePanEmitDelta emit_delta,
    void *delta_context,
    MobilePanEmitKey emit_key)
{
    if (!pan) return;

    /* Velocity estimation using smoothed EWMA */
    uint32_t dt = (tick > pan->last_move_tick) ? (tick - pan->last_move_tick) : 0;
    if (dt > 0 && dt < 120) {
        float inst_vx = dx_game / (float)dt;
        float inst_vy = dy_game / (float)dt;
        pan->velocity_x = 0.45f * inst_vx + 0.55f * pan->velocity_x;
        pan->velocity_y = 0.45f * inst_vy + 0.55f * pan->velocity_y;
    } else {
        pan->velocity_x = dx_game / 16.0f;
        pan->velocity_y = dy_game / 16.0f;
    }
    pan->last_move_tick = tick;
    pan->is_panning = 1;

    /* Direct smooth delta emission to the game viewport hook */
    if (emit_delta && emit_delta(delta_context, dx_game, dy_game)) {
        return;
    }

    /* Fallback discrete pulse logic if no smooth hook handled it */
    static const float threshold = 28.0f;
    static const uint32_t min_interval_ms = 45;

    pan->accumulator_x += dx_game;
    pan->accumulator_y += dy_game;

    float max_acc = threshold * 2.0f;
    if (pan->accumulator_x > max_acc) pan->accumulator_x = max_acc;
    if (pan->accumulator_x < -max_acc) pan->accumulator_x = -max_acc;
    if (pan->accumulator_y > max_acc) pan->accumulator_y = max_acc;
    if (pan->accumulator_y < -max_acc) pan->accumulator_y = -max_acc;

    if (tick > 0 && pan->last_emit_tick > 0 && (tick - pan->last_emit_tick < min_interval_ms)) {
        return;
    }

    int emitted = 0;
    if (pan->accumulator_x >= threshold) {
        MobilePan_EmitKey(emit_key, delta_context, VK_RIGHT);
        pan->accumulator_x -= threshold;
        emitted = 1;
    } else if (pan->accumulator_x <= -threshold) {
        MobilePan_EmitKey(emit_key, delta_context, VK_LEFT);
        pan->accumulator_x += threshold;
        emitted = 1;
    }

    if (pan->accumulator_y >= threshold) {
        MobilePan_EmitKey(emit_key, delta_context, VK_DOWN);
        pan->accumulator_y -= threshold;
        emitted = 1;
    } else if (pan->accumulator_y <= -threshold) {
        MobilePan_EmitKey(emit_key, delta_context, VK_UP);
        pan->accumulator_y += threshold;
        emitted = 1;
    }

    if (emitted) {
        pan->last_emit_tick = tick;
    }
}

static inline void MobilePan_EndDrag(MobilePanState *pan, uint32_t tick) {
    if (!pan) return;
    pan->is_panning = 0;

    /* If finger paused on glass (> 90ms) before lifting, do not fling */
    if (tick > pan->last_move_tick && (tick - pan->last_move_tick > 90)) {
        pan->velocity_x = 0.0f;
        pan->velocity_y = 0.0f;
        pan->is_coasting = 0;
        return;
    }

    float speed = sqrtf(pan->velocity_x * pan->velocity_x + pan->velocity_y * pan->velocity_y);
    static const float min_flick_speed = 0.15f;  /* 150 px/s */
    static const float max_flick_speed = 3.50f;  /* 3500 px/s */

    if (speed >= min_flick_speed) {
        if (speed > max_flick_speed) {
            float scale = max_flick_speed / speed;
            pan->velocity_x *= scale;
            pan->velocity_y *= scale;
        }
        pan->is_coasting = 1;
        pan->inertia_vx = pan->velocity_x;
        pan->inertia_vy = pan->velocity_y;
        pan->last_inertia_tick = tick;
        pan->inertia_start_tick = tick;
    } else {
        pan->is_coasting = 0;
    }
}

static inline int MobilePan_UpdateInertia(
    MobilePanState *pan,
    uint32_t tick,
    MobilePanEmitDelta emit_delta,
    void *delta_context)
{
    if (!pan || !pan->is_coasting) return 0;
    if (tick <= pan->last_inertia_tick) return 1;

    uint32_t dt = tick - pan->last_inertia_tick;
    if (dt > 100) dt = 100;
    pan->last_inertia_tick = tick;

    /* Exponential friction decay: ~0.92 per 16ms frame */
    float decay = powf(0.92f, (float)dt / 16.0f);
    float step_x = pan->inertia_vx * (float)dt;
    float step_y = pan->inertia_vy * (float)dt;

    pan->inertia_vx *= decay;
    pan->inertia_vy *= decay;

    if (emit_delta) {
        emit_delta(delta_context, step_x, step_y);
    }

    float speed = sqrtf(pan->inertia_vx * pan->inertia_vx + pan->inertia_vy * pan->inertia_vy);
    static const float min_stop_speed = 0.03f;  /* 30 px/s */
    if (speed < min_stop_speed || (tick - pan->inertia_start_tick > 3000)) {
        pan->is_coasting = 0;
        pan->inertia_vx = 0.0f;
        pan->inertia_vy = 0.0f;
    }
    return 1;
}

/* Legacy wrapper for backwards compatibility */
static inline void MobilePan_EmitFingerDelta(
    MobilePanState *pan,
    float dx,
    float dy,
    int logical_width,
    int logical_height,
    uint32_t tick,
    MobilePanEmitKey emit_key,
    void *context)
{
    static const float threshold = 28.0f;
    static const uint32_t min_interval_ms = 45;

    if (!pan || logical_width <= 0 || logical_height <= 0) {
        return;
    }

    pan->accumulator_x += dx * (float)logical_width;
    pan->accumulator_y += dy * (float)logical_height;

    /* Clamp accumulator to prevent runaway bursts or latent scroll */
    float max_acc = threshold * 2.0f;
    if (pan->accumulator_x > max_acc) pan->accumulator_x = max_acc;
    if (pan->accumulator_x < -max_acc) pan->accumulator_x = -max_acc;
    if (pan->accumulator_y > max_acc) pan->accumulator_y = max_acc;
    if (pan->accumulator_y < -max_acc) pan->accumulator_y = -max_acc;

    /* Pacing: enforce smooth spacing between pulses so game animates smoothly */
    if (tick > 0 && pan->last_emit_tick > 0 && (tick - pan->last_emit_tick < min_interval_ms)) {
        return;
    }

    int emitted = 0;
    if (pan->accumulator_x >= threshold) {
        MobilePan_EmitKey(emit_key, context, VK_LEFT);
        pan->accumulator_x -= threshold;
        emitted = 1;
    } else if (pan->accumulator_x <= -threshold) {
        MobilePan_EmitKey(emit_key, context, VK_RIGHT);
        pan->accumulator_x += threshold;
        emitted = 1;
    }

    if (pan->accumulator_y >= threshold) {
        MobilePan_EmitKey(emit_key, context, VK_UP);
        pan->accumulator_y -= threshold;
        emitted = 1;
    } else if (pan->accumulator_y <= -threshold) {
        MobilePan_EmitKey(emit_key, context, VK_DOWN);
        pan->accumulator_y += threshold;
        emitted = 1;
    }

    if (emitted) {
        pan->last_emit_tick = tick;
    }
}

#endif
