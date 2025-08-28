// nmea.c — minimal RMC/GGA parser (parameters used to avoid -Werror)
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static float nmea_coord_to_dec(const char* s, bool is_lat) {
    if(!s || !*s) return 0.f;
    int deg_len = is_lat ? 2 : 3;
    char degbuf[4] = {0};
    strncpy(degbuf, s, deg_len);
    int deg = atoi(degbuf);
    float min = (float)atof(s + deg_len);
    return (float)deg + (min / 60.0f);
}

static const char* field_at(const char* line, int index, char* out, size_t out_sz) {
    if(!line) return NULL;
    const char* p = line;
    int commas = 0;
    const char* first_comma = strchr(p, ',');
    if(!first_comma) return NULL;
    p = first_comma + 1;

    while(commas < index && (p = strchr(p, ','))) {
        ++commas; ++p;
    }
    if(commas != index || !p) return NULL;

    const char* q = strchr(p, ',');
    size_t len = (q ? (size_t)(q - p) : strlen(p));
    if(out && out_sz) {
        if(len >= out_sz) len = out_sz - 1;
        memcpy(out, p, len);
        out[len] = '\0';
        return out;
    }
    return NULL;
}

void nmea_parse_line(const char* line, bool* has_fix, float* lat, float* lon, float* hdop, uint8_t* sats) {
    if(has_fix) *has_fix = false;
    if(lat) *lat = 0.f;
    if(lon) *lon = 0.f;
    if(hdop) *hdop = 99.9f;
    if(sats) *sats = 0;
    if(!line || line[0] != '$') return;

    const char* type = line + 3; // after "$GP"
    if(strncmp(type, "RMC", 3) == 0) {
        char buf[16];
        if(field_at(line, 1, buf, sizeof buf)) {
            if(has_fix) *has_fix = (buf[0] == 'A');
        }
        if(lat && lon) {
            char lat_s[16]={0}, ns[4]={0}, lon_s[16]={0}, ew[4]={0};
            if(field_at(line, 2, lat_s, sizeof lat_s) && field_at(line, 3, ns, sizeof ns) &&
               field_at(line, 4, lon_s, sizeof lon_s) && field_at(line, 5, ew, sizeof ew)) {
                float _lat = nmea_coord_to_dec(lat_s, true);
                float _lon = nmea_coord_to_dec(lon_s, false);
                if(ns[0] == 'S') _lat = -_lat;
                if(ew[0] == 'W') _lon = -_lon;
                *lat = _lat; *lon = _lon;
            }
        }
    } else if(strncmp(type, "GGA", 3) == 0) {
        char fix[4]={0}, lat_s[16]={0}, ns[4]={0}, lon_s[16]={0}, ew[4]={0}, sats_s[8]={0}, hdop_s[16]={0};
        (void)field_at(line, 5, fix, sizeof fix);
        if(has_fix) *has_fix = (fix[0] == '1' || fix[0] == '2');

        if(field_at(line, 1, lat_s, sizeof lat_s) && field_at(line, 2, ns, sizeof ns) &&
           field_at(line, 3, lon_s, sizeof lon_s) && field_at(line, 4, ew, sizeof ew)) {
            float _lat = nmea_coord_to_dec(lat_s, true);
            float _lon = nmea_coord_to_dec(lon_s, false);
            if(ns[0] == 'S') _lat = -_lat;
            if(ew[0] == 'W') _lon = -_lon;
            if(lat) *lat = _lat;
            if(lon) *lon = _lon;
        }
        if(field_at(line, 6, sats_s, sizeof sats_s) && sats) *sats = (uint8_t)atoi(sats_s);
        if(field_at(line, 7, hdop_s, sizeof hdop_s) && hdop) *hdop = (float)atof(hdop_s);
    }
}
