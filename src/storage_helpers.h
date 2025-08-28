#pragma once
#include <stdint.h>

void storage_write_all_formats(float lat, float lon, float hdop, uint8_t sats,
                               const char* k, const char* v, const char* note);
