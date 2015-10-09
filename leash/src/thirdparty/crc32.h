#ifndef CRC32_H_
#define CRC32_H_

#include <stdint.h>

unsigned long long crc32 (uint8_t* data, int len, unsigned int crc);

#endif
