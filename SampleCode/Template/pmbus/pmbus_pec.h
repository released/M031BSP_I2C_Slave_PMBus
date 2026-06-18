#ifndef PMBUS_PEC_H
#define PMBUS_PEC_H

#include "pmbus_cfg_user.h"

#if (PMBUS_PEC_BACKEND == PMBUS_PEC_BACKEND_HW_CRC)
void pmbus_pec_init(void);
#else
#define pmbus_pec_init() do { } while (0)
#endif

unsigned char pmbus_pec_update(unsigned char crc, unsigned char data_byte);

#endif
