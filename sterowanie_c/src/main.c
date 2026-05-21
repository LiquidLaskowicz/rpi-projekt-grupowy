#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "config.h"
#include "control.h"
#include "read_yolo.h"
#include "i2c.h" 

// Globalny tryb pracy
volatile work_mode_t WORK_MODE = WORK_MODE_MANUAL_NO_SHOOT;

// Funkcja pomocnicza do usypiania (mikrosekundy)
static inline void sleep_us(long us) {
    struct timespec ts;
    ts.tv_sec  = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

int main(void) {
    printf("%s v%s (I2C Master Mode)\n", APP_NAME, APP_VERSION);

    // 1. Inicjalizacja I2C
    int i2c_fd = i2c_init(I2C_BUS);
    if (i2c_fd < 0) {
        fprintf(stderr, "Błąd krytyczny: Nie można otworzyć magistrali I2C\n");
        return 1;
    }

    // Zmienne pomocnicze
    char i2c_buffer[64];
    int yolo_timeout_counter = 0;
    int debug_counter = 0;
    const int MAX_YOLO_TIMEOUT = 50; // ok. 500ms (przy pętli 100Hz)

    // Zmienne dla auto-strzału
    struct timespec last_shot_time = {0, 0};
    const long COOLDOWN_MS = 1000; // 1 sekunda przerwy między strzałami w AUTO

    DEBUG_PRINT("System wystartował. Czekam na dane...");

    // --- PĘTLA GŁÓWNA (Dostosowana do pada ESP32 7-int) ---
    while(1) {
        // Zmienne do odebrania surowych danych z ESP32
        int gora = 0, dol = 0, prawo = 0, lewo = 0;
        int esp_bezp = 1, esp_tryb = 1, esp_strzal = 1;

        float final_vx = 0.0f;
        float final_vy = 0.0f;
        int final_shoot = 0;

        // 1️⃣ ODCZYT KONTROLERA (Adres 0x10)
        if (i2c_set_address(i2c_fd, ADDR_CONTROLLER) == 0) {
            if (read(i2c_fd, i2c_buffer, sizeof(i2c_buffer) - 1) > 0) {
                
                // Parsujemy 7 wartości wysyłanych przez ESP32
                if (sscanf(i2c_buffer, "%d,%d,%d,%d,%d,%d,%d", 
                           &gora, &dol, &prawo, &lewo, &esp_bezp, &esp_tryb, &esp_strzal) == 7) {
                    

                    debug_counter++;
                    if (debug_counter % 40 == 0) {
                        DEBUG_PRINT("Odebrano z ESP32 -> G:%d D:%d P:%d L:%d | Bezp:%d Tryb:%d Strzal:%d",
                                    gora, dol, prawo, lewo, esp_bezp, esp_tryb, esp_strzal);
                        
                        // Możesz też od razu podejrzeć, co wyliczyliśmy dla silników:
                        DEBUG_PRINT("Wysyłka do silników -> vx:%.2f vy:%.2f shoot:%d mode:%d",
                                    final_vx, final_vy, final_shoot, (int)WORK_MODE);
                        printf("--------------------------------------------------\n");
                    }

                    // --- TŁUMACZENIE TRYBÓW Z ESP32 NA RPi ---
                    work_mode_t nowy_tryb;

                    if (esp_tryb == -1) {
                        // Tryb AUTO (YOLO)
                        nowy_tryb = WORK_MODE_AUTO; 
                    } 
                    else {
                        // Tryb MANUAL (Tryb był 1)
                        if (esp_bezp == 1) {
                            nowy_tryb = WORK_MODE_MANUAL_NO_SHOOT; // Bezpieczny
                        } else {
                            nowy_tryb = WORK_MODE_MANUAL_SHOOT;    // Odbezpieczony
                        }
                    }

                    // Jeśli nastąpiła zmiana trybu, resetujemy PID
                    if (nowy_tryb != WORK_MODE) {
                    WORK_MODE = nowy_tryb;
                    control_reset();
                    yolo_timeout_counter = 0;
                    DEBUG_PRINT("Zmiana trybu na RPi: %d", WORK_MODE);
}
                }
            }
        }

        // 2️⃣ LOGIKA STEROWANIA W ZALEŻNOŚCI OD WORK_MODE
        
        // --- TRYBY RĘCZNE (0: Manual Safe, 1: Manual Fire) ---
        if (WORK_MODE == WORK_MODE_MANUAL_NO_SHOOT || WORK_MODE == WORK_MODE_MANUAL_SHOOT) {
            
            // Mapowanie przycisków cyfrowych na pełną prędkość [-1.0, 1.0]
            // Oś X (Prawo / Lewo)
            if (prawo == 1 && lewo == 0)       final_vx = 1.0f;
            else if (lewo == 1 && prawo == 0)  final_vx = -1.0f;
            else                               final_vx = 0.0f;

            // Oś Y (Góra / Dół)
            if (gora == 1 && dol == 0)         final_vy = 1.0f;
            else if (dol == 1 && gora == 0)    final_vy = -1.0f;
            else                               final_vy = 0.0f;

            // Strzał: Aktywny tylko w trybie MANUAL_SHOOT i gdy ESP wysyła -1
            if (WORK_MODE == WORK_MODE_MANUAL_SHOOT && esp_strzal == -1) {
                final_shoot = 1;
            } else {
                final_shoot = 0; 
            }
        }

        // --- TRYB AUTOMATYCZNY (-1: YOLO) ---
        else if (WORK_MODE == WORK_MODE_AUTO) {
            float err_x = 0.0f, err_y = 0.0f;

            // Zamiast starego wywołania, wywołujemy z 2 argumentami:
        if (read_yolo_state(&err_x, &err_y)) {
            yolo_timeout_counter = 0; // Jeśli funkcja zwróciła true, cel jest wykryty!

            velocity_t out = control_update((velocity_t){err_x, err_y});
            final_vx = out.vx;
            final_vy = out.vy;

            // Logika auto-strzału z cooldownem (gdy cel na środku)
            if (fabs(err_x) < 0.05f && fabs(err_y) < 0.05f) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);

                long elapsed = (now.tv_sec - last_shot_time.tv_sec) * 1000 + 
                            (now.tv_nsec - last_shot_time.tv_nsec) / 1000000;

                if (elapsed >= COOLDOWN_MS) {
                    final_shoot = 1;
                    last_shot_time = now;
                    DEBUG_PRINT("AUTO: Cel namierzony - STRZAŁ!");
                }
            }
        } else {
            // Jeśli zwróciła false, tzn. brak celu lub brak danych z YOLO
            yolo_timeout_counter++;
            if (yolo_timeout_counter > MAX_YOLO_TIMEOUT) {
                final_vx = 0.0f; final_vy = 0.0f;
                final_shoot = 0;
            } else {
                goto wait_next_iter; 
            }
        }
        }

        // 3️⃣ WYSYŁKA ROZKAZÓW DO SILNIKÓW (Arduino 0x08)
        // Format dla silników zostaje bez zmian: "vx,vy,shoot,mode\n"
        char out_buf[64];
        snprintf(out_buf, sizeof(out_buf), "%.2f,%.2f,%d,%d\n", 
                 final_vx, final_vy, final_shoot, (int)WORK_MODE);

        if (i2c_set_address(i2c_fd, ADDR_MOTORS) == 0) {
            write(i2c_fd, out_buf, strlen(out_buf));
        }

    wait_next_iter:
        sleep_us(10000); // 100Hz
    }

    i2c_close(i2c_fd);
    return 0;
}