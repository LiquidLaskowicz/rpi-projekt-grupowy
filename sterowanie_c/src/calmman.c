#include "calmman.h"

#define SCALE 0.01f     // jak mocno reaguje (tuning!)
#define ALPHA 0.2f      // filtr (0–1), mniejsze = bardziej gładko

#define VX_MAX 1.0f  // maksymalna prędkość w osi X
#define VY_MAX 1.0f  // maksymalna prędkość w osi Y

velocity_t kalman_update(int dx, int dy)
{
    static float vx_prev = 0.0f;
    static float vy_prev = 0.0f;

    velocity_t v;

    float vx_raw = dx * SCALE;
    float vy_raw = dy * SCALE;

    // Ograniczenie do maksymalnej prędkości
    if (vx_raw > VX_MAX) vx_raw = VX_MAX;
    if (vx_raw < -VX_MAX) vx_raw = -VX_MAX;

    if (vy_raw > VY_MAX) vy_raw = VY_MAX;
    if (vy_raw < -VY_MAX) vy_raw = -VY_MAX;

    // Filtr wygładzający
    v.vx = ALPHA * vx_raw + (1.0f - ALPHA) * vx_prev;
    v.vy = ALPHA * vy_raw + (1.0f - ALPHA) * vy_prev;

    vx_prev = v.vx;
    vy_prev = v.vy;

    return v;
}