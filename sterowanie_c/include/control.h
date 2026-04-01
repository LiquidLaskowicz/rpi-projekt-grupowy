// Plik naglowkowy obslugi ruchu silnikow

#ifndef CONTROL_H
#define CONTROL_H

#include "config.h"

// ================= VELOCITY =================

typedef struct {
    float vx;
    float vy;
} velocity_t;

// ================= CONTROL =================

velocity_t control_update(velocity_t input);

#endif