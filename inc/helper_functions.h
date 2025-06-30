#pragma once

#include <stdio.h>
#include <stdint.h>

#define MAX_DATA_PER_PACKET 1440

// Imprime o buffer em hexadecimal
void print_hex(const char *label, const unsigned char *buffer, int size);

// Imprime os dados como string
void print_data_as_string(const uint8_t *data, uint16_t len);

// Imprime o uuid
void print_uuid(const __uint128_t uuid);