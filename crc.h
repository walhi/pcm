#ifndef CRC_H
#define CRC_H

#include <cstdint>

uint16_t Calculate_CRC_CCITT(const uint8_t* buffer, int size);

#endif
