#include "i2c.h"
#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <errno.h>

int i2c_init(const char *device) {
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        DEBUG_PRINT("Błąd otwarcia I2C: %s", strerror(errno));
        return -1;
    }
    return fd;
}

int i2c_set_address(int fd, int addr) {
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        return -1;
    }
    return 0;
}

int i2c_write_string(int fd, const char *buf) {
    int len = strlen(buf);
    int n = write(fd, buf, len);
    return (n == len) ? 0 : -1;
}

int i2c_read_string(int fd, char *buf, int max_len) {
    // W I2C Master wymusza odczyt konkretnej liczby bajtów.
    // Czytamy max_len - 1, żeby zostawić miejsce na '\0'
    int n = read(fd, buf, max_len - 1);
    if (n > 0) {
        buf[n] = '\0';
        
        // Opcjonalnie: czyścimy znaki końca linii, jeśli Arduino je wysłało
        char *p = strpbrk(buf, "\r\n");
        if (p) *p = '\0';
        
        return n;
    }
    return -1;
}

void i2c_close(int fd) {
    if (fd >= 0) close(fd);
}