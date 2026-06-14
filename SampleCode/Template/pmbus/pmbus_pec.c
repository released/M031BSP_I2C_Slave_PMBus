#include "pmbus_types.h"
#include "pmbus_pec.h"

uint8_t pmbus_pec_update(uint8_t crc, uint8_t data_byte)
{
    uint8_t bit_index;

    crc = (uint8_t)(crc ^ data_byte);

    for (bit_index = 0U; bit_index < 8U; bit_index++)
    {
        if ((crc & 0x80U) != 0U)
        {
            crc = (uint8_t)((crc << 1) ^ 0x07U);
        }
        else
        {
            crc = (uint8_t)(crc << 1);
        }
    }

    return crc;
}
