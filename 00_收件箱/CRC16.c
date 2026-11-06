#include "CRC16.h"

uint16_t CRC16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xffff;
    uint8_t i, j;
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
            (crc & 0x0001?(crc = (crc >> 1) ^ 0xA001):(crc >>= 1));
    }
    return crc;
}