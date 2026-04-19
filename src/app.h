#pragma once
#include <furi.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/view_dispatcher.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <expansion/expansion.h>

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

#define GPS_RX_BUF_SIZE 256

typedef struct App {
    // UI
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* menu;           // menu principal
    Submenu* preset_menu;    // liste des presets OSM
    Submenu* last_points_menu; // vue "Last points" (browse + undo)
    TextInput* note_input;   // éditeur de note courte
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
    float heading_deg;    // cap en degrés (course over ground, RMC)
    uint8_t sats;
    uint32_t last_fix_tick; // furi_get_tick() du dernier fix reçu (0 = jamais)
    uint32_t last_notify_tick; // dernier "fix acquired" notifié (cooldown anti-spam)
    bool pending_fix_notify; // posé en ISR à la transition no-fix -> fix, consommé dans le tick

    // Vue Quick Log
    View* quick_view;

    // Buffer de ligne pour NMEA (rempli en RX ISR)
    char nmea_line[GPS_RX_BUF_SIZE];
    size_t nmea_pos;

    // Diagnostics UART (incrémentés en ISR)
    uint32_t nmea_bytes_rx;  // total octets reçus
    uint32_t nmea_lines_rx;  // total lignes NMEA parsées

    // Note rapide optionnelle
    char quick_note[64];

    // Preset courant (index dans presets_get()) + variante active au sein de ce preset
    uint8_t current_preset;
    uint8_t current_variant;

    // Compteur de points sauvegardés depuis le démarrage de l'app
    uint16_t session_count;

    // Total cumulatif de points dans points.jsonl (rechargé au démarrage)
    uint32_t total_count;

    // --- Mode trace ---
    View* track_view;
    FuriTimer* track_timer;
    uint32_t track_points;
    uint32_t track_start_tick;
    bool track_new_segment;
    // Filtre distance : dernière position écrite (pour haversine)
    float last_trk_lat;
    float last_trk_lon;
    bool last_trk_valid;

    // --- Statut GPS ---
    View* status_view;

    // --- About ---
    View* about_view;

    // --- Settings (persistés sur SD) ---
    Settings settings;
    VariableItemList* settings_list;
    View* settings_view;

    // --- Point detail ---
    View* point_detail_view;

    // --- Preview save (flag temporaire) ---
    bool preview_pending;
} App;

void app_reconfigure_uart(App* app);

typedef enum {
    AppViewMenu = 0,
    AppViewPresets = 1,
    AppViewQuickLog = 2,
    AppViewTrack = 3,
    AppViewStatus = 4,
    AppViewNote = 5,
    AppViewAbout = 6,
    AppViewSettings = 7,
    AppViewLastPoints = 8,
    AppViewPointDetail = 9,
} AppView;

