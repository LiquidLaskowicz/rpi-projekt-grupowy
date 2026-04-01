#include "read_yolo.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

bool read_yolo_state(int *status, float *error_x, float *error_y)
{
    static FILE *fifo = NULL;
    char buf[64];

    if (!fifo)
    {
        fifo = fopen("/tmp/yolo_fifo", "r");
        if (!fifo)
        {
            perror("FIFO open");
            return false;
        }

        // non-blocking
        fcntl(fileno(fifo), F_SETFL, O_NONBLOCK);
    }

    if (fgets(buf, sizeof(buf), fifo) != NULL)
    {
        if (sscanf(buf, "%d,%f,%f", status, error_x, error_y) == 3)
            return true;
    }
    else
    {
        if (feof(fifo))
        {
            // prawdziwe EOF → zamknij
            fclose(fifo);
            fifo = NULL;
        }
        else if (errno != EAGAIN)
        {
            // realny błąd
            perror("fgets");
            fclose(fifo);
            fifo = NULL;
        }
        // jeśli EAGAIN → po prostu brak danych (NORMALNE)
    }

    return false;
}