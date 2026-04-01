// Plik konfiguracyjny - przechowuje parametry programu oraz obsluguje debugowanie

#ifndef CONFIG_H
#define CONFIG_H

// Nazwa i wersja
#define APP_NAME "projekt_grupowy"
#define APP_VERSION "0.3"

// UART
#define UART_DEVICE "/dev/ttyAMA0" // urzadzenie UART
#define UART_BAUDRATE B9600          // predkosc transmisji
#define BAUDRATE_SPEED_VALUE 9600    // predkosc do wyswietlania w debugu

// Debugowanie [1 - DEBUG ON / 0 - DEBUG OFF]
#define DEBUG 1

#if DEBUG
    #include <stdio.h>
    #define DEBUG_PRINT(fmt, ...) \
        fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif

#define PIN_DIR_X 17
#define PIN_STEP_X  27

#define PIN_DIR_Y 5
#define PIN_STEP_Y 6

#define SHOOT_COOLDOWN_MS 300

// --- Przykładowe piny GPIO ---
#define PIN_X       0   // GPIO17 (wiringPi 0)
#define PIN_Y       1   // GPIO18 (wiringPi 1)
#define PIN_SHOOT   2   // GPIO27 (wiringPi 2)
#define PIN_MODE    3   // GPIO22 (wiringPi 3)

#define MID 512
#define DEADZONE 40
#define THRESHOLD 120

typedef enum
{
    WORK_MODE_MANUAL = 1,
    WORK_MODE_AUTO = -1

} work_mode_t;

extern volatile work_mode_t WORK_MODE;

void set_work_mode(work_mode_t mode);

#endif
