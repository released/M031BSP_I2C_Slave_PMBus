#ifndef PMBUS_DISPATCH_H
#define PMBUS_DISPATCH_H

#include "board_config.h"
#include "pmbus_protocol.h"

typedef enum
{
    PMBUS_PROTOCOL_UNKNOWN = 0,
    PMBUS_PROTOCOL_SEND_BYTE,
    PMBUS_PROTOCOL_WRITE_BYTE,
    PMBUS_PROTOCOL_WRITE_WORD,
    PMBUS_PROTOCOL_READ_BYTE,
    PMBUS_PROTOCOL_READ_WORD,
    PMBUS_PROTOCOL_READ_DWORD,
    PMBUS_PROTOCOL_BLOCK_WRITE,
    PMBUS_PROTOCOL_BLOCK_READ,
    PMBUS_PROTOCOL_PROCESS_CALL,
    PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL
} pmbus_dispatch_protocol_t;

typedef struct
{
    unsigned char command;
    unsigned char payload[PMBUS_MAX_BLOCK_SIZE + 1U];
    unsigned char data_len;
    unsigned char repeated_start;
    unsigned char pec_present;
    unsigned char pec_valid;
    pmbus_dispatch_protocol_t protocol;
} pmbus_dispatch_transaction_t;

pmbus_dispatch_protocol_t pmbus_dispatch_detect_protocol(unsigned char command, unsigned char data_len, unsigned char *payload, unsigned char repeated_start);
unsigned char pmbus_dispatch_is_known_command(unsigned char command);
unsigned char pmbus_dispatch_execute(pmbus_dispatch_transaction_t *transaction, unsigned char *tx_buffer, unsigned char *tx_length);
void pmbus_dispatch_prepare_error_response(unsigned char command, unsigned char *tx_buffer, unsigned char *tx_length);

#endif
