#ifndef CONFIG_H
#define CONFIG_H

// Nazwa i wersja
#define APP_NAME "projekt_grupowy_i2c"
#define APP_VERSION "0.4"

// --- KONFIGURACJA I2C ---
#define I2C_BUS             "/dev/i2c-1"
#define ADDR_CONTROLLER     0x09  // Arduino z joystickiem (Slave)
#define ADDR_MOTORS         0x08  // Arduino od silnikow (Slave)

// Debugowanie [1 - DEBUG ON / 0 - DEBUG OFF]
#define DEBUG 1

#if DEBUG
    #include <stdio.h>
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

// --- PARAMETRY STEROWANIA ---
#define MID          512
#define DEADZONE     40
#define THRESHOLD    120
#define SHOOT_COOLDOWN_MS 300

// --- GPIO (Jeśli RPi nadal czyta przyciski/tryby bezpośrednio) ---
#define PIN_X       0   // WiringPi 0
#define PIN_Y       1   // WiringPi 1
#define PIN_SHOOT   2   // WiringPi 2
#define PIN_MODE    3   // WiringPi 3

// --- LOGIKA TRYBÓW PRACY ---
typedef enum
{
    WORK_MODE_AUTO             = -1, // Tryb automatyczny (YOLO)
    WORK_MODE_MANUAL_NO_SHOOT  = 0,  // Ręczny - ruch jest, strzał zablokowany
    WORK_MODE_MANUAL_SHOOT     = 1   // Ręczny - ruch jest, strzał aktywny
} work_mode_t;

extern volatile work_mode_t WORK_MODE;

void set_work_mode(work_mode_t mode);

#endif