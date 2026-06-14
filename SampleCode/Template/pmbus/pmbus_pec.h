#ifndef PMBUS_PEC_H
#define PMBUS_PEC_H

unsigned char pmbus_pec_update(unsigned char crc, unsigned char data_byte);

#endif
