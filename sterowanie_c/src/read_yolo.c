#include "read_yolo.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// 1. ZMIANA: Nagłówek funkcji przyjmuje teraz tylko dwa argumenty (współrzędne błędu)
bool read_yolo_state(float *error_x, float *error_y)
{
    static int fifo_fd = -1;
    char buf[128];
    char last_valid_line[128] = {0};
    bool found_data = false;

    // 1. Inicjalizacja/Otwarcie deskryptora (surowy deskryptor jest lepszy dla O_NONBLOCK)
    if (fifo_fd < 0)
    {
        fifo_fd = open("/tmp/yolo_fifo", O_RDONLY | O_NONBLOCK);
        if (fifo_fd < 0)
        {
            // Nie spamujemy błędem, jeśli Pythona jeszcze nie ma
            return false;
        }
    }

    // 2. Czytamy WSZYSTKO co jest w rurze, żeby dojść do najnowszych danych
    while (true)
    {
        ssize_t n = read(fifo_fd, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            // Szukamy ostatniej pełnej linii w buforze (zakończonej \n)
            char *newline = strrchr(buf, '\n');
            if (newline) {
                *newline = '\0';
                // Szukamy początku tej ostatniej linii
                char *start = strrchr(buf, '\n');
                if (!start) start = buf; else start++;
                
                snprintf(last_valid_line, sizeof(last_valid_line), "%s", start);
                found_data = true;
            }
        }
        else
        {
            if (n == 0) // EOF - Python zamknął rurę
            {
                close(fifo_fd);
                fifo_fd = -1;
            }
            break; // Brak więcej danych w tej chwili (EAGAIN)
        }
    }

    // 3. Parsowanie ostatniej znalezionej klatki
    if (found_data)
    {
        int local_status = 0; // 2. ZMIANA: Tworzymy zwykłą, lokalną zmienną int na status z YOLO
        
        // Parsujemy dane do naszej zmiennej lokalnej oraz przez wskaźniki error_x, error_y
        if (sscanf(last_valid_line, "%d,%f,%f", &local_status, error_x, error_y) == 3)
        {
            // 3. ZMIANA: Zwracamy true tylko wtedy, gdy ramka jest poprawna ORAZ status==1 (cel wykryty)
            return (local_status == 1);
        }
    }

    return false;
}