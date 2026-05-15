#include "read_yolo.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

bool read_yolo_state(int *status, float *error_x, float *error_y)
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
    // FIFO może mieć zakolejkowane stare klatki. Chcemy tylko ostatnią.
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
                
                strncpy(last_valid_line, start, sizeof(last_valid_line)-1);
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
        if (sscanf(last_valid_line, "%d,%f,%f", status, error_x, error_y) == 3)
        {
            return true;
        }
    }

    return false;
}