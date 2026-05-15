#include "control.h"
#include "config.h"
#include <math.h>

// ================= PARAMETRY =================

#define KP             1.0f
#define ALPHA          0.2f
#define AUTO_DEADZONE  0.05f  // Zmieniona nazwa, żeby nie było konfliktu z config.h

// ================= STAN =================

static float vx_prev = 0.0f;
static float vy_prev = 0.0f;

// ================= CONTROL =================

velocity_t control_update(velocity_t input)
{
    velocity_t out;

    // 1. DEADZONE na wejściu (błąd z YOLO)
    // Jeśli błąd jest mniejszy niż 5%, uznajemy, że cel jest na środku
    if (fabs(input.vx) < AUTO_DEADZONE) input.vx = 0.0f;
    if (fabs(input.vy) < AUTO_DEADZONE) input.vy = 0.0f;

    // 2. Regulator P
    float vx = KP * input.vx;
    float vy = KP * input.vy;

    // 3. Filtr EMA (wygładzanie ruchu)
    out.vx = ALPHA * vx + (1.0f - ALPHA) * vx_prev;
    out.vy = ALPHA * vy + (1.0f - ALPHA) * vy_prev;

    vx_prev = out.vx;
    vy_prev = out.vy;

    // 4. Clamp (ograniczenie do zakresu -1.0 do 1.0)
    // To ważne, bo Arduino może nie spodziewać się większych wartości
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