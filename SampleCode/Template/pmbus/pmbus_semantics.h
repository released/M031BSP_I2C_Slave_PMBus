#ifndef PMBUS_SEMANTICS_H
#define PMBUS_SEMANTICS_H

void pmbus_semantics_record_write(unsigned char command,
                                  unsigned char protocol,
                                  const unsigned char *payload,
                                  unsigned char payload_len);
void pmbus_semantics_record_read_response(unsigned char command,
                                          unsigned char protocol,
                                          const unsigned char *data,
                                          unsigned char data_len,
                                          unsigned char pec_present);
void pmbus_semantics_background_task(void);

#endif
