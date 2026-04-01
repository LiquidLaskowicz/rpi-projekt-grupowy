#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>

// inicjalizacja GPIO
bool gpio_init(void);

// odczyt osi x/y [-1,1]
float read_gpio_x(void);
float read_gpio_y(void);

// odczyt strzału (0 lub 1)
int read_gpio_shoot(void);

// odczyt trybu (0 = manual, 1 = auto)
int read_gpio_mode(void);

#endif // GPIO_H