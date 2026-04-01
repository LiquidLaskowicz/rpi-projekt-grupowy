#include "control.h"
#include "config.h"

#include <math.h>

// ================= PARAMETRY =================

#define KP     1.0f
#define ALPHA  0.2f
#define DEADZONE 0.05f

// ================= STAN =================

static float vx_prev = 0.0f;
static float vy_prev = 0.0f;

// ================= CONTROL =================

velocity_t control_update(velocity_t input)
{
    velocity_t out;

    // DEADZONE na wejściu
    if (fabs(input.vx) < DEADZONE) input.vx = 0.0f;
    if (fabs(input.vy) < DEADZONE) input.vy = 0.0f;

    // regulator P
    float vx = KP * input.vx;
    float vy = KP * input.vy;

    // filtr
    out.vx = ALPHA * vx + (1.0f - ALPHA) * vx_prev;
    out.vy = ALPHA * vy + (1.0f - ALPHA) * vy_prev;

    vx_prev = out.vx;
    vy_prev = out.vy;

    // clamp
    if (out.vx > 1.0f)  out.vx = 1.0f;
    if (out.vx < -1.0f) out.vx = -1.0f;

    if (out.vy > 1.0f)  out.vy = 1.0f;
    if (out.vy < -1.0f) out.vy = -1.0f;

    return out;
}

void control_reset(void)
{
    vx_prev = 0.0f;
    vy_prev = 0.0f;
}