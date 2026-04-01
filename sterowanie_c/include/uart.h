// Plik naglowkowy portu szeregowego

#ifndef UART_H
#define UART_H

// Inicjalizacja UART
int uart_init(const char *device);
// Odbior linii
int uart_czytaj_linie(int file_desc, char *buffor, int max_length);
// Zamkniecie UART
void uart_close(int file_desc);

#endif