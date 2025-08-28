#include "nmea.h"
#include <stdlib.h>
#include <string.h>

static float nmea_coord_to_dec(const char* s, bool is_lat) {
    if(!s || !*s) return 0.f;
    char degbuf[4] = {0};
    int deg_len = is_lat ? 2 : 3;
    strncpy(degbuf, s, deg_len);
    int deg = atoi(degbuf);
    float min = atof(s + deg_len);
    return deg + (min/60.0f);
}

void nmea_parse_line(const char* line, bool* has_fix, float* lat, float* lon, float* hdop, uint8_t* sats) {
    if(!line || line[0] != '$') return;
    // Minimal RMC/GGA parser (simplifié)
    if(strncmp(line+3, "RMC", 3) == 0) {
        *has_fix = strstr(line, ",A,") != NULL;
    } else if(strncmp(line+3, "GGA", 3) == 0) {
        *has_fix = strstr(line, ",1,") != NULL || strstr(line, ",2,") != NULL;
    }
}
