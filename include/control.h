// Plik naglowkowy obslugi ruchu silnikow

#ifndef CONTROL_H
#define CONTROL_H

#include "config.h"

typedef struct
{
    int x;
    int y;
    int strzal;
    work_mode_t work_mode;
} kierunek_vector;

int control_init(void);

void set_dir_x(int dir);
void set_move_x(int enable);

void set_dir_y(int dir);
void set_move_y(int enable);

void set_shoot(int enable);

int get_shoot_flag(void);

void control_close(void);

#endif