// src/app.c
#include "app.h"

#include <furi.h>
#include <gui/gui.h>
#include <storage/storage.h>
#include <furi_hal.h>

// UART (SDK récent, API handle-based)
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>

#include "nmea.h"
#include "quick_log.h"
#include "presets.h"
#include "storage_helpers.h"

// --- Forward
static void app_menu_callback(void* ctx, uint32_t index);
static void preset_menu_callback(void* ctx, uint32_t index);

typedef enum {
    MenuItemQuickLog = 0,
    MenuItemStatus = 1,
} MenuItem;

// Back depuis le menu principal -> on sort de l'app
static bool app_navigation_callback(void* ctx) {
    (void)ctx;
    return false; // false => view_dispatcher_run s'arrête
}

// Back depuis le sous-menu des presets -> retour au menu principal
static uint32_t preset_menu_previous(void* ctx) {
    (void)ctx;
    return AppViewMenu;
}

// Tick périodique -> met à jour le modèle de la vue Quick Log
static void app_tick_callback(void* ctx) {
    App* app = (App*)ctx;
    quick_log_refresh(app);
}

// --- Helpers
void app_show_status(App* app) {
    if(app->has_fix) {
        FURI_LOG_I("OSM", "Fix OK lat=%.6f lon=%.6f HDOP=%.1f sats=%u",
                   (double)app->lat, (double)app->lon, (double)app->hdop, app->sats);
        notification_message(app->notification, &sequence_success);
    } else {
        FURI_LOG_W("OSM", "No fix (sats=%u)", app->sats);
        notification_message(app->notification, &sequence_error);
    }
}

// --- RX async callback (IRQ): NE PAS toucher au GUI depuis une IRQ
static void serial_rx_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    (void)handle;
    App* app = (App*)context;

    if(event & FuriHalSerialRxEventData) {
        while(furi_hal_serial_async_rx_available(app->serial)) {
            uint8_t c = furi_hal_serial_async_rx(app->serial);

            if(c == '\n' || c == '\r') {
                if(app->nmea_pos > 0) {
                    app->nmea_line[app->nmea_pos] = '\0';

                    bool has_fix = false;
                    float lat = 0, lon = 0, hdop = 99.9f, altitude = 0;
                    uint8_t sats = 0;

                    nmea_parse_line(
                        app->nmea_line, &has_fix, &lat, &lon, &hdop, &sats, &altitude);

                    app->has_fix = has_fix;
                    app->lat = lat;
                    app->lon = lon;
                    app->hdop = hdop;
                    app->altitude = altitude;
                    app->sats = sats;
                    if(has_fix) app->last_fix_tick = furi_get_tick();

                    app->nmea_pos = 0;
                }
            } else {
                if(app->nmea_pos < (GPS_RX_BUF_SIZE - 1)) {
                    app->nmea_line[app->nmea_pos++] = (char)c;
                } else {
                    app->nmea_pos = 0; // overflow safety
                }
            }
        }
    }
    (void)event;
}

// --- Menu principal
static void app_menu_callback(void* ctx, uint32_t index) {
    App* app = ctx;
    if(index == MenuItemQuickLog) {
        view_dispatcher_switch_to_view(app->dispatcher, AppViewPresets);
    } else if(index == MenuItemStatus) {
        app_show_status(app);
    }
}

// --- Sous-menu presets : choix -> ouvre Quick Log avec ce preset
static void preset_menu_callback(void* ctx, uint32_t index) {
    App* app = ctx;
    if(index < PRESETS_COUNT) {
        app->current_preset = (uint8_t)index;
    }
    app_start_quick_log(app);
}

// --- Entry point
int32_t app(void* p) {
    (void)p;

    App* app = malloc(sizeof(App));
    furi_assert(app);
    memset(app, 0, sizeof(App));

    // Records
    app->gui = furi_record_open(RECORD_GUI);
    app->notification = furi_record_open(RECORD_NOTIFICATION);

    // Dispatcher
    app->dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->dispatcher, app_navigation_callback);
    view_dispatcher_set_tick_event_callback(app->dispatcher, app_tick_callback, 500);
    view_dispatcher_attach_to_gui(app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Menu principal
    app->menu = submenu_alloc();
    submenu_add_item(
        app->menu, "Mode rapide (Quick Log)", MenuItemQuickLog, app_menu_callback, app);
    submenu_add_item(
        app->menu, "Statut GPS (vibre/LED + log)", MenuItemStatus, app_menu_callback, app);
    View* menu_view = submenu_get_view(app->menu);
    view_dispatcher_add_view(app->dispatcher, AppViewMenu, menu_view);

    // Sous-menu des presets OSM
    app->preset_menu = submenu_alloc();
    submenu_set_header(app->preset_menu, "Type de POI");
    for(uint8_t i = 0; i < PRESETS_COUNT; i++) {
        submenu_add_item(app->preset_menu, PRESETS[i].label, i, preset_menu_callback, app);
    }
    View* preset_view = submenu_get_view(app->preset_menu);
    view_set_previous_callback(preset_view, preset_menu_previous);
    view_dispatcher_add_view(app->dispatcher, AppViewPresets, preset_view);

    view_dispatcher_switch_to_view(app->dispatcher, AppViewMenu);

    // --- Désactiver l'Expansion service (sinon il intercepte l'UART GPS) ---
    app->expansion = furi_record_open(RECORD_EXPANSION);
    expansion_disable(app->expansion);

    // --- UART init (SDK récent) ---
    app->serial = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_hal_serial_init(app->serial, 9600);
    furi_hal_serial_async_rx_start(app->serial, serial_rx_callback, app, false);

    // UI loop
    view_dispatcher_run(app->dispatcher);

    // --- Teardown ---
    if(app->serial) {
        furi_hal_serial_async_rx_stop(app->serial);
        furi_hal_serial_deinit(app->serial);
        furi_hal_serial_control_release(app->serial);
        app->serial = NULL;
    }

    if(app->expansion) {
        expansion_enable(app->expansion);
        furi_record_close(RECORD_EXPANSION);
        app->expansion = NULL;
    }

    if(app->quick_view) {
        view_dispatcher_remove_view(app->dispatcher, AppViewQuickLog);
        view_free(app->quick_view);
        app->quick_view = NULL;
    }
    view_dispatcher_remove_view(app->dispatcher, AppViewPresets);
    submenu_free(app->preset_menu);

    view_dispatcher_remove_view(app->dispatcher, AppViewMenu);
    submenu_free(app->menu);

    view_dispatcher_free(app->dispatcher);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);
    return 0;
}
