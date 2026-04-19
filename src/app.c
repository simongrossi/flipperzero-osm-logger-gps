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
#include "track.h"
#include "status.h"
#include "about.h"
#include "presets.h"
#include "storage_helpers.h"

// --- Forward
static void app_menu_callback(void* ctx, uint32_t index);
static void preset_menu_callback(void* ctx, uint32_t index);

typedef enum {
    MenuItemQuickLog = 0,
    MenuItemTrack = 1,
    MenuItemStatus = 2,
    MenuItemAbout = 3,
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

// Tick périodique (500 ms) -> rafraîchit les vues + consomme les notifications
// posées par l'ISR (pas de call GUI depuis IRQ).
static void app_tick_callback(void* ctx) {
    App* app = (App*)ctx;
    if(app->pending_fix_notify) {
        app->pending_fix_notify = false;
        notification_message(app->notification, &sequence_success);
        FURI_LOG_I("OSM", "GPS fix acquired");
    }
    quick_log_refresh(app);
    track_refresh(app);
    status_refresh(app);
}

// --- RX async callback (IRQ): NE PAS toucher au GUI depuis une IRQ
static void serial_rx_callback(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    (void)handle;
    App* app = (App*)context;

    if(event & FuriHalSerialRxEventData) {
        while(furi_hal_serial_async_rx_available(app->serial)) {
            uint8_t c = furi_hal_serial_async_rx(app->serial);
            app->nmea_bytes_rx++;

            if(c == '\n' || c == '\r') {
                if(app->nmea_pos > 0) {
                    app->nmea_line[app->nmea_pos] = '\0';
                    app->nmea_lines_rx++;

                    bool has_fix = false;
                    float lat = 0, lon = 0, hdop = 99.9f, altitude = 0;
                    uint8_t sats = 0;

                    nmea_parse_line(
                        app->nmea_line, &has_fix, &lat, &lon, &hdop, &sats, &altitude);

                    bool was_fix = app->has_fix;
                    app->has_fix = has_fix;
                    app->lat = lat;
                    app->lon = lon;
                    app->hdop = hdop;
                    app->altitude = altitude;
                    app->sats = sats;
                    if(has_fix) {
                        app->last_fix_tick = furi_get_tick();
                        if(!was_fix) app->pending_fix_notify = true;
                    }

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
    } else if(index == MenuItemTrack) {
        app_start_track(app);
    } else if(index == MenuItemStatus) {
        app_start_status(app);
    } else if(index == MenuItemAbout) {
        app_start_about(app);
    }
}

// --- Sous-menu presets : choix -> ouvre Quick Log avec ce preset
static void preset_menu_callback(void* ctx, uint32_t index) {
    App* app = ctx;
    if(index < presets_count()) {
        app->current_preset = (uint8_t)index;
        app->current_variant = 0; // reset de la variante à chaque nouveau choix
    }
    app_start_quick_log(app);
}

// --- Note TextInput : validation -> retour Quick Log
static void note_input_callback(void* ctx) {
    App* app = (App*)ctx;
    view_dispatcher_switch_to_view(app->dispatcher, AppViewQuickLog);
}

// Back depuis le TextInput -> retour Quick Log (et non exit app)
static uint32_t note_input_previous(void* ctx) {
    (void)ctx;
    return AppViewQuickLog;
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

    // Init des presets (depuis SD si dispo, sinon defaults)
    presets_init();

    // Total cumulatif de points (compte les lignes de points.jsonl existant)
    app->total_count = storage_count_saved_points();

    // Menu principal
    app->menu = submenu_alloc();
    submenu_add_item(
        app->menu, "Quick Log (waypoints)", MenuItemQuickLog, app_menu_callback, app);
    submenu_add_item(
        app->menu, "Track mode (auto GPX)", MenuItemTrack, app_menu_callback, app);
    submenu_add_item(
        app->menu, "GPS status", MenuItemStatus, app_menu_callback, app);
    submenu_add_item(
        app->menu, "About", MenuItemAbout, app_menu_callback, app);
    View* menu_view = submenu_get_view(app->menu);
    view_dispatcher_add_view(app->dispatcher, AppViewMenu, menu_view);

    // Sous-menu des presets OSM
    app->preset_menu = submenu_alloc();
    submenu_set_header(app->preset_menu, "Pick a POI type");
    for(uint8_t i = 0; i < presets_count(); i++) {
        submenu_add_item(
            app->preset_menu, presets_get(i)->label, i, preset_menu_callback, app);
    }
    View* preset_view = submenu_get_view(app->preset_menu);
    view_set_previous_callback(preset_view, preset_menu_previous);
    view_dispatcher_add_view(app->dispatcher, AppViewPresets, preset_view);

    // Éditeur de note (TextInput partagé, alloué une seule fois)
    app->note_input = text_input_alloc();
    text_input_set_header_text(app->note_input, "Short note (OK to save)");
    text_input_set_result_callback(
        app->note_input,
        note_input_callback,
        app,
        app->quick_note,
        sizeof(app->quick_note),
        false /* on garde le texte précédent */);
    View* note_view = text_input_get_view(app->note_input);
    view_set_previous_callback(note_view, note_input_previous);
    view_dispatcher_add_view(app->dispatcher, AppViewNote, note_view);

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

    app_stop_track(app);
    app_stop_status(app);
    app_stop_about(app);

    if(app->note_input) {
        view_dispatcher_remove_view(app->dispatcher, AppViewNote);
        text_input_free(app->note_input);
        app->note_input = NULL;
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

    presets_deinit();

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);
    return 0;
}
