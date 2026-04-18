#pragma once
#include <stdbool.h>
#include <stdint.h>

void nmea_parse_line(
    const char* line,
    bool* has_fix,
    float* lat,
    float* lon,
    float* hdop,
    uint8_t* sats,
    float* altitude);
