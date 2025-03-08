#ifndef _UTILS_H
#define _UTILS_H

void REG32_write(unsigned int base, unsigned int offset, unsigned int value);
unsigned int REG32_read(unsigned int base, unsigned int offset);
void REG32_write_masked(unsigned int base, unsigned int offset, unsigned int mask, unsigned int value);
#endif
