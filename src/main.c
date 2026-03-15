// Glowna petla programu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "control.h"
#include "uart.h"

#include <time.h>
#include <errno.h>

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
    char buffor[32]; // buffor na dane z UART  // 64 bajty to w sumie overkill int ma 2 to wystarczy dla pewnosci 16 ale zostawie na razie
    kierunek_vector kierunek = {0, 0, 0, 0}; // struktura przechowujace kierunek i zmienną strzał

    uart_file_desc = uart_init(UART_DEVICE); // inicjalizacja UART (otwarcie urzadzenia i ustawienie predkosci transmisji)

    if (uart_file_desc < 0)
    {
        DEBUG_PRINT("Blad inicjalizacji UART");
        return 1;
    } // sprawdzenie czy UART uruchomil sie poprawnie

    if (control_init() < 0)
    {
    DEBUG_PRINT("Blad inicjalizacji GPIO");
    return 1;
    } // sprawdzenie czy uruchomil sie onterfejs GPIO

    DEBUG_PRINT("Oczekiwanie na dane z ESP");

    while(1)
{
    int length = uart_czytaj_linie(uart_file_desc, buffor, sizeof(buffor));
    if (length <= 0) continue;

    // Usunięcie znaków nowej linii
    buffor[strcspn(buffor, "\r\n")] = 0;

    int x, y, strzal;
    work_mode_t mode;

    if (sscanf(buffor, "%d,%d,%d,%d", &x, &y, &strzal, (int*)&mode) == 4)
    {
        // Zmiana trybu
        if (mode != WORK_MODE)
            set_work_mode(mode);

        // Sterowanie osiami w trybie manual
        if (WORK_MODE == WORK_MODE_MANUAL)
        {
            kierunek.x = x;
            kierunek.y = y;
            set_dir_x(kierunek.x);
            set_dir_y(kierunek.y);
            set_move_x(kierunek.x != 0);
            set_move_y(kierunek.y != 0);
        }

        // Sterowanie strzalem
        kierunek.strzal = strzal;
        set_shoot(kierunek.strzal);

        DEBUG_PRINT("Kierunek: X=%d Y=%d Strzal=%d Mode=%d",
                    (WORK_MODE==WORK_MODE_MANUAL? x : 0), 
                    (WORK_MODE==WORK_MODE_MANUAL? y : 0), 
                    kierunek.strzal, 
                    WORK_MODE);
    }
    else
    {
        DEBUG_PRINT("Nieprawidlowe dane: %s", buffor);
    }

}

    uart_close(uart_file_desc); // generalnie to program dziala w petli nieskonczonej
                                // wiec UART sie nie zamknie chyba ze dodac funkcje do tego
    control_close();             // tutaj to samo moze dla picu dodac cos takiego ze jak jakis blad wyrzuci to ma byc wyjsice z glownej petli i wywolanie tych funkcji
    return 0;
}

// moze zrobic tak zeby wejscie w jeden tryb pracy zamykalo rownolegly watek drugiego trybu ale nwm