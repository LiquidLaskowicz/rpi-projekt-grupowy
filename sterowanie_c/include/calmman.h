#ifndef KALMAN_H
#define KALMAN_H

#include "control.h"

// ================= KALMAN =================
// Funkcja przyjmuje odchylenie obiektu od środka kamery
// i zwraca prędkości vx, vy dla silników
velocity_t kalman_update(int dx, int dy);

#endif