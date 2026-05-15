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
    const int MAX_YOLO_TIMEOUT = 50; // ok. 500ms (przy pętli 100Hz)

    // Zmienne dla danych z kontrolera (pad)
    int ctrl_x = MID, ctrl_y = MID, ctrl_shoot = 0, ctrl_mode_raw = 0;

    // Zmienne dla auto-strzału
    struct timespec last_shot_time = {0, 0};
    const long COOLDOWN_MS = 1000; // 1 sekunda przerwy między strzałami w AUTO

    DEBUG_PRINT("System wystartował. Czekam na dane...");

    // --- GŁÓWNA PĘTLA PROGRAMU ---
    while(1) {
        float final_vx = 0.0f;
        float final_vy = 0.0f;
        int final_shoot = 0;

        // 1️⃣ ODCZYT DANYCH Z KONTROLERA (Arduino 0x09)
        if (i2c_set_address(i2c_fd, ADDR_CONTROLLER) == 0) {
            if (i2c_read_string(i2c_fd, i2c_buffer, sizeof(i2c_buffer)) > 0) {
                if (sscanf(i2c_buffer, "%d,%d,%d,%d", &ctrl_x, &ctrl_y, &ctrl_shoot, &ctrl_mode_raw) == 4) {
                    
                    // Jeśli nastąpiła zmiana trybu na padzie
                    if ((work_mode_t)ctrl_mode_raw != WORK_MODE) {
                        WORK_MODE = (work_mode_t)ctrl_mode_raw;
                        control_reset();      // Resetujemy PID
                        yolo_timeout_counter = 0;
                        DEBUG_PRINT("Zmiana trybu na: %d", WORK_MODE);
                    }
                }
            }
        }

        // 2️⃣ LOGIKA WYBORU STEROWANIA
        
        // --- TRYBY RĘCZNE (0: Manual Safe, 1: Manual Fire) ---
        if (WORK_MODE == WORK_MODE_MANUAL_NO_SHOOT || WORK_MODE == WORK_MODE_MANUAL_SHOOT) {
            // Obliczanie prędkości z joysticka
            if (abs(ctrl_x - MID) > DEADZONE)
                final_vx = (float)(ctrl_x - MID) / (MID - DEADZONE);
            if (abs(ctrl_y - MID) > DEADZONE)
                final_vy = (float)(ctrl_y - MID) / (MID - DEADZONE);

            // Strzał: tylko w trybie 1 przekazujemy stan przycisku
            if (WORK_MODE == WORK_MODE_MANUAL_SHOOT) {
                final_shoot = ctrl_shoot;
            } else {
                final_shoot = 0; // W trybie 0 strzał zawsze zablokowany
            }
        }

        // --- TRYB AUTOMATYCZNY (-1: YOLO Tracking) ---
        else if (WORK_MODE == WORK_MODE_AUTO) {
            float err_x = 0.0f, err_y = 0.0f;
            int status = 0;

            if (read_yolo_state(&status, &err_x, &err_y)) {
                yolo_timeout_counter = 0; // Reset failsafe
                
                if (status == 1) { // Cel wykryty
                    velocity_t out = control_update((velocity_t){err_x, err_y});
                    final_vx = out.vx;
                    final_vy = out.vy;

                    // Logika auto-strzału z cooldownem
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
                }
            } else {
                // Brak danych z YOLO - czekamy chwilę zanim się zatrzymamy
                yolo_timeout_counter++;
                if (yolo_timeout_counter > MAX_YOLO_TIMEOUT) {
                    final_vx = 0.0f; final_vy = 0.0f;
                    final_shoot = 0;
                } else {
                    // Pomijamy wysyłkę, kontynuujemy poprzedni ruch
                    goto wait_next;
                }
            }
        }

        // 3️⃣ WYSYŁKA ROZKAZÓW DO SILNIKÓW (Arduino 0x08)
        char out_buf[64];
        snprintf(out_buf, sizeof(out_buf), "%.2f,%.2f,%d,%d\n", 
                 final_vx, final_vy, final_shoot, (int)WORK_MODE);

        if (i2c_set_address(i2c_fd, ADDR_MOTORS) == 0) {
            i2c_write_string(i2c_fd, out_buf);
        }

    wait_next:
        sleep_us(10000); // Częstotliwość pętli 100Hz
    }

    i2c_close(i2c_fd);
    return 0;
}