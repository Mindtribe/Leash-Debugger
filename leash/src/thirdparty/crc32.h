#ifndef CRC32_H_
#define CRC32_H_

#include <stdint.h>

//CRC function used for checking for data corruption.
unsigned int crc32(uint8_t *data, int len, unsigned int crc);

#endif
