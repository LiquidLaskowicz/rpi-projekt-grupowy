// Obsluga portu szeregowego

#include "uart.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

static const speed_t BAUDRATE = UART_BAUDRATE; // static ze tylko w tym pliku ta funkcja obowiazuje

static inline void sleep_us(long us) // inline oznacza ze kompilator w miejsce wywolania tej funkcji wrzuca ten kod caly a nie odwoluje sie do niej i to jest szybsze
{
    struct timespec ts;
    ts.tv_sec  = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR); // jesli przyjdzie przerwanie to w ts zapisany zoastanie pozostaly czas snu i on dospi tyle ile mu zostalo po obsludze przerwania
}

// Inicjalizacja UART (otwiera port, konfiguruje go, zwraca deskryptor pliku)

int uart_init(const char *device)
{
    int file_desc = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    // O_RDWR - odczyt i zapis
    // O_NOCTTY - UART nie stanie sie terminalem
    // O_NDELAY - nie blokuj przy otwieraniu

    if (file_desc < 0)
    {
        DEBUG_PRINT("UART otwarcie nie powiodlo sie: %s", strerror(errno)); // errno to globalna zmienna ktora kernel sam ustawi na wystapiony blad a strerror zmieni to na czytelny string
        return -1;
    } // obsluga bledu systemowego

    struct termios opcje; // konfiguracja termios czyli ustawienia portu szeregowego

    tcgetattr(file_desc, &opcje);  // pobiera aktualne ustawienia UART
    cfsetispeed(&opcje, BAUDRATE); // ustawia predkosc wejsciowa
    cfsetospeed(&opcje, BAUDRATE); // ustawia predkosc wyjsciowa

    opcje.c_cflag |= (CLOCAL | CREAD);
    // CLOCAL - ignoruj sygnaly modemu
    // CREAD - wlacz odbiornik
    
    // standard 8N1 [format ramki]
    opcje.c_cflag &= ~PARENB; // brak parzystosci
    opcje.c_cflag &= ~CSTOPB; //  bit stopu
    opcje.c_cflag &= ~CSIZE;  // czyszczenie maski dlugosci slowa
    opcje.c_cflag |= CS8;     // 8 bitow danych

    opcje.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    // wylaczenie: trybu kanonicznego, echa i sygnalow terminala
    opcje.c_iflag &= ~(IXON | IXOFF | IXANY);
    // wylacza programowa kontrole przeplywu
    opcje.c_oflag &= ~OPOST;
    // brak obrobki wyjscia czyli nie dodawane sa bity konca linii np itd

    tcsetattr(file_desc, TCSANOW, &opcje);
    // zastosowanie ustawien natychmiast

    DEBUG_PRINT("UART zainicjalizowany (%s, %d baud)", device, BAUDRATE_SPEED_VALUE);
    return file_desc;
}

// Funkcja czyta jedna linie tekstu z UART znak po znaku do \n
int uart_czytaj_linie(int file_desc, char *buffor, int max_length)
{
    // licznik znakow i bufor na 1 bajt
    int i = 0;
    char c;
    
    while (i < max_length - 1) // -1 bo jeszcze znak \0 na koncu
    {
        int n = read(file_desc, &c, 1); // proba odczytu 1 znaku
        if (n > 0) // jesli cos przyszlo \n jest koncem linii, jesli nie to dopisz do bufora
        {
            if (c == '\n')
                break;
            buffor[i++] = c;

        }
        else
        {
            sleep_us(1000);
        } // gdy nic nie przyszlo poczekaj 1 ms
    }

    // zakonczenie stringa i zwrot dlugosci
    buffor[i] = '\0'; // poprawne zakonczenie stringa
    return i;
}

// funkcja zamyka port uart
void uart_close(int file_desc)
{
    close(file_desc);
}