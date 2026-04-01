#include "gpio.h"
#include <wiringPi.h>
#include <stdio.h>
#include <math.h>
#include "config.h"

// --- inicjalizacja ---
bool gpio_init(void)
{
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "Błąd inicjalizacji wiringPi\n");
        return false;
    }

    pinMode(PIN_X, INPUT);
    pinMode(PIN_Y, INPUT);
    pinMode(PIN_SHOOT, INPUT);
    pinMode(PIN_MODE, INPUT);

    pullUpDnControl(PIN_X, PUD_OFF);
    pullUpDnControl(PIN_Y, PUD_OFF);
    pullUpDnControl(PIN_SHOOT, PUD_UP); // domyślnie 1
    pullUpDnControl(PIN_MODE, PUD_UP);

    return true;
}

// --- odczyt analogowy GPIO
// na potrzeby przykładu zakładamy, że masz już ADC lub potencjometr
// funkcja powinna zwrócić float [-1,1]
// jeśli GPIO cyfrowe to -1 lub 1
static float read_axis(int channel)
{
    int raw = adc_read(channel);
    int diff = raw - MID;

    if (abs(diff) < DEADZONE)
        return 0.0f;

    if (diff > THRESHOLD)
        return 1.0f;

    if (diff < -THRESHOLD)
        return -1.0f;

    return 0.0f;
}

float read_gpio_x(void)
{
    return read_axis(PIN_X);  // już z deadzone
}

float read_gpio_y(void)
{
    return read_axis(PIN_Y);  // już z deadzone
}

int read_gpio_shoot(void)
{
    int val = digitalRead(PIN_SHOOT);
    float fval = (val > 0) ? 1.0f : 0.0f;

    // deadzone / próg strzału
    if (fval < DEADZONE) fval = 0.0f;

    return (int)fval;
}

int read_gpio_mode(void)
{
    int val = digitalRead(PIN_MODE);
    return (val > 0) ? 1 : 0;
}