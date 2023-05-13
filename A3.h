#ifndef A3_H
#define A3_H

void error_checked_read(int fd, void *ptr, size_t size, char string[]);

void error_checked_write(int fd, void *ptr, size_t size, char string[]);

#endif