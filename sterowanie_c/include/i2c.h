#ifndef I2C_H
#define I2C_H

// Inicjalizacja magistrali (np. "/dev/i2c-1")
int i2c_init(const char *device);

// Wybór urządzenia (0x08 lub 0x09)
int i2c_set_address(int fd, int addr);

// Wysyłanie tekstu do Arduino
int i2c_write_string(int fd, const char *buf);

// Odbieranie danych od Arduino
int i2c_read_string(int fd, char *buf, int max_len);

// Zamknięcie
void i2c_close(int fd);

#endif