#pragma once
#include <furi.h>
#include <gui/modules/submenu.h>
#include <gui/view_dispatcher.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <expansion/expansion.h>

#include <stdbool.h>
#include <stdint.h>

#define GPS_RX_BUF_SIZE 256

typedef struct App {
    // UI
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* menu;           // menu principal
    Submenu* preset_menu;    // liste des presets OSM
    NotificationApp* notification;

    // Expansion service (désactivé le temps qu'on utilise l'UART GPS)
    Expansion* expansion;

    // UART handle
    struct FuriHalSerialHandle* serial;

    // Données GPS
    bool has_fix;
    float lat;
    float lon;
    float hdop;
    float altitude;       // mètres au-dessus du geoïde (GGA)
    uint8_t sats;
    uint32_t last_fix_tick; // furi_get_tick() du dernier fix reçu (0 = jamais)

    // Vue Quick Log
    View* quick_view;

    // Buffer de ligne pour NMEA (rempli en RX ISR)
    char nmea_line[GPS_RX_BUF_SIZE];
    size_t nmea_pos;

    // Note rapide optionnelle
    char quick_note[64];

    // Preset courant (index dans PRESETS, défini dans quick_log.c)
    uint8_t current_preset;

    // Compteur de points sauvegardés depuis le démarrage de l'app
    uint16_t session_count;
} App;

typedef enum {
    AppViewMenu = 0,
    AppViewPresets = 1,
    AppViewQuickLog = 2,
} AppView;

#ifdef __cplusplus
extern "C" {
#endif
void app_show_status(App* app);
#ifdef __cplusplus
}
#endif
