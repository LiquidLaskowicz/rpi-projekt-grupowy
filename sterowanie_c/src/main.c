// Glowna petla programu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "control.h"
#include "uart.h"
#include "calmman.h"

#include <time.h>
#include <errno.h>

#include "read_yolo.h"

typedef struct
{
    int x;
    int y;
    int strzal;
    work_mode_t work_mode;
} kierunek_vector;

volatile work_mode_t WORK_MODE = WORK_MODE_MANUAL;

void set_work_mode(work_mode_t mode)
{
    WORK_MODE = mode;
    DEBUG_PRINT("Zmiana trybu na: %d", mode);
}

static inline void sleep_us(long us)
{
    struct timespec ts;
    ts.tv_sec  = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

int main(void)
{
    printf("%s v%s\n", APP_NAME, APP_VERSION);

    DEBUG_PRINT("This metal gear is pretty solid");

    int uart_file_desc; // deskryptor pliku UART
    char buffor[64]; // buffor na dane z UART  // 64 bajty to w sumie overkill int ma 2 to wystarczy dla pewnosci 16 ale zostawie na razie, jednak nie bo wysyla vx i vy do espa
    kierunek_vector kierunek = {0, 0, 0, 0}; // struktura przechowujace kierunek i zmienną strzał

    uart_file_desc = uart_init(UART_DEVICE); // inicjalizacja UART (otwarcie urzadzenia i ustawienie predkosci transmisji)

    if (uart_file_desc < 0)
    {
        DEBUG_PRINT("Blad inicjalizacji UART");
        return 1;
    } // sprawdzenie czy UART uruchomil sie poprawnie

    int uart_out_desc;

    uart_out_desc = uart_init(UART_DEVICE_OUT);
    if (uart_out_desc < 0)
    {
        DEBUG_PRINT("Blad inicjalizacji UART OUT");
        return 1;
    }

    DEBUG_PRINT("Oczekiwanie na dane z ESP");

    float vx_prev = 0.0f, vy_prev = 0.0f;
    const float Kp = 1.0f; // wzmocnienie P regulatora
    const float ALPHA = 0.2f;

    while(1)
    {
        // 1️⃣ odczyt trybu
        int mode_val = read_gpio_mode();   // 0 = manual, 1 = auto
        if ((work_mode_t)mode_val != WORK_MODE)
        {
            set_work_mode(mode_val);
            control_reset();  // 🔥 ważne
        }

        if (WORK_MODE == WORK_MODE_MANUAL)
        {
            float x = read_gpio_x();      // już z deadzone
            float y = read_gpio_y();      // już z deadzone
            int strzal = read_gpio_shoot(); // 0 lub 1

            // 4️⃣ wysyłka po UART do Arduino
            char out_buf[64];
            snprintf(out_buf, sizeof(out_buf), "%.2f,%.2f,%d,%d",
                    x, y, (int)strzal, WORK_MODE);
            uart_write(uart_out_desc, out_buf, strlen(out_buf));
            uart_write(uart_out_desc, "\n", 1);
        }

        else if (WORK_MODE == WORK_MODE_AUTO)
        {
            float error_x = 0.0f, error_y = 0.0f;
            int status = 0;

            // --- próbujemy pobrać dane z YOLO ---
            if (!read_yolo_state(&status, &error_x, &error_y))
            {
                // brak danych → wysyłamy STOP do Arduino
                char stop_buf[64];
                snprintf(stop_buf, sizeof(stop_buf), "0.00,0.00,0,%d\n", WORK_MODE);
                uart_write(uart_out_desc, stop_buf, strlen(stop_buf));

                DEBUG_PRINT("AUTO: Brak danych z YOLO - wysłano STOP");

                sleep_us(5000);
                continue; // przechodzimy do kolejnej iteracji pętli
            }

            velocity_t input = { error_x, error_y };
            velocity_t output;

            if (status == 1)
            {
                // normalne sterowanie
                output = control_update(input);
            }
            else
            {
                // brak celu → reset regulatora i zatrzymanie
                control_reset();
                output.vx = 0.0f;
                output.vy = 0.0f;
            }

            // przygotowanie i wysyłka do Arduino
            char out_buf[64];
            snprintf(out_buf, sizeof(out_buf), "%.2f,%.2f,%d,%d\n",
                    output.vx, output.vy, 0, WORK_MODE);

            uart_write(uart_out_desc, out_buf, strlen(out_buf));

            DEBUG_PRINT("AUTO: vx=%.2f vy=%.2f", output.vx, output.vy);
        }
        sleep_us(5000); // np. 5 ms, zależnie od częstotliwości
    }



    uart_close(uart_file_desc); // generalnie to program dziala w petli nieskonczonej
                                // wiec UART sie nie zamknie chyba ze dodac funkcje do tego
    control_close();             // tutaj to samo moze dla picu dodac cos takiego ze jak jakis blad wyrzuci to ma byc wyjsice z glownej petli i wywolanie tych funkcji
    return 0;
}

// moze zrobic tak zeby wejscie w jeden tryb pracy zamykalo rownolegly watek drugiego trybu ale nwm