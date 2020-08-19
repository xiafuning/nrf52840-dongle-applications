#include <stdio.h>
#include "utils.h"

void print_coefficients (uint8_t* coeff_buf, uint32_t length)
{
    printf ("coefficients: ");
    for (uint32_t i = 0; i < length; i++)
        printf ("%02x ", *(coeff_buf + i));
    printf ("\n");
}

void print_payload (uint8_t* payload, uint32_t length)
{
    printf ("payload: ");
    for (uint32_t i = 0; i < length; ++i)
        printf ("%02x ", *(payload + i));
    printf ("\n");
}
