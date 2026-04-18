#pragma once
#include <stdint.h>

void storage_write_all_formats(
    float lat,
    float lon,
    float altitude,
    float hdop,
    uint8_t sats,
    const char* tag,
    const char* note);
