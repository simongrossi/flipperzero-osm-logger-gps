#pragma once
#include <furi.h>
#include <gui/modules/submenu.h>
#include <gui/view_dispatcher.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include <stdbool.h>
#include <stdint.h>

#define GPS_RX_BUF_SIZE 256

typedef struct App {
    // UI
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* menu;
    NotificationApp* notification;

    // UART handle
    struct FuriHalSerialHandle* serial;

    // Données GPS
    bool has_fix;
    float lat;
    float lon;
    float hdop;
    uint8_t sats;

    // Vue Quick Log
    View* quick_view;

    // Buffer de ligne pour NMEA (rempli en RX ISR)
    char nmea_line[GPS_RX_BUF_SIZE];
    size_t nmea_pos;

    // Note rapide optionnelle
    char quick_note[64];
} App;

typedef enum {
    AppViewMenu = 0,
    AppViewQuickLog = 1,
} AppView;

#ifdef __cplusplus
extern "C" {
#endif
void app_show_status(App* app);
#ifdef __cplusplus
}
#endif
