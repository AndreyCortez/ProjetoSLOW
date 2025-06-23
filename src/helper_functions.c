#include "helper_functions.h"

void print_hex(const char *label, const unsigned char *buffer, int size)
{
    printf("%s (%d bytes):\n", label, size);

    for (int i = 0; i < size; i++)
    {
        printf("%02X ", buffer[i]);

        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }

    printf("\n\n");
}

void print_data_as_string(const uint8_t *data, uint16_t len)
{
    printf("Dados como String: \"");
    for (int i = 0; i < len; i++)
    {
        if (data[i] >= 32 && data[i] <= 126)
        {
            putchar(data[i]);
        }
        else
        {
            putchar('.');
        }
    }

    printf("\"\n\n");
}

void print_uuid(const __uint128_t uuid)
{
    const uint8_t *bytes = (const uint8_t *)&uuid;
    for (int i = 0; i < 16; i++)
    {
        printf("%02X", bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9)
        {
            printf("-");
        }
    }
}