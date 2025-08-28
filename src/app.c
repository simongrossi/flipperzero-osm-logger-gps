#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <input/input.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "nmea.h"
#include "storage_helpers.h"
#include "quick_log.h"

#define UART_BAUD 9600
#define UART_BUFF 128

enum {
    ViewIdMainMenu = 0,
};

struct App {
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* submenu;

    bool has_fix;
    double lat, lon;
    float hdop;
    uint8_t sats;
};

static void app_uart_init(void) {
    furi_hal_uart_init(FuriHalUartIdUSART1, UART_BAUD);
}

static void app_uart_poll(App* app) {
    (void)app;
    // Ici, on pourrait lire du NMEA et mettre à jour has_fix/lat/lon/hdop/sats.
    // (Le parseur minimal n'est pas branché pour cette démo.)
}

static void enter_classic_cb(void* ctx) {
    App* app = ctx;
    UNUSED(app);
    // Stub: dans la V1, on pouvait déclencher une capture directe
    // Ici on ne fait rien (à implémenter selon ton mode classique).
}

static void enter_quicklog_cb(void* ctx) {
    App* app = ctx;
    quicklog_start(app);
}

void app_show_main_menu(App* app) {
    if(app->submenu) submenu_free(app->submenu);
    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "Logger classique", 0, enter_classic_cb, app);
    submenu_add_item(app->submenu, "Mode rapide",     1, enter_quicklog_cb, app);
    submenu_set_header(app->submenu, "OSM Logger");
    view_dispatcher_add_view(app->dispatcher, ViewIdMainMenu, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->dispatcher, ViewIdMainMenu);
}

/* ====== API exposées pour quick_log ====== */
bool app_get_fix(App* app, double* lat, double* lon, float* hdop, uint8_t* sats) {
    if(lat) *lat = app->lat;
    if(lon) *lon = app->lon;
    if(hdop) *hdop = app->hdop;
    if(sats) *sats = app->sats;
    return app->has_fix;
}

static const char* k_presets[] = {"bench","waste_basket","drinking_water","toilets"};
uint8_t app_get_preset_count(App* app){ (void)app; return sizeof(k_presets)/sizeof(k_presets[0]); }
const char* app_get_preset_key(App* app, uint8_t idx){ (void)app; return k_presets[idx % app_get_preset_count(app)]; }
const char* app_get_preset_variant(App* app, uint8_t idx, uint8_t* has_variant){ (void)app;(void)idx; if(has_variant) *has_variant=0; return ""; }

ViewDispatcher* app_get_view_dispatcher(App* app){ return app->dispatcher; }

bool app_save_point(App* app, const char* key, const char* variant, const char* note,
                    double lat, double lon, float hdop, uint8_t sats, const char* quality) {
    (void)variant; (void)quality;
    storage_write_all_formats((float)lat, (float)lon, hdop, sats, "amenity", key, note?note:"");
    return true;
}

/* ====== Entrée principale ====== */
int32_t app(void* p) {
    UNUSED(p);
    App* app = malloc(sizeof(App));
    memset(app, 0, sizeof(App));

    app->gui = furi_record_open(RECORD_GUI);
    app->dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app_uart_init();
    app_show_main_menu(app);

    // Boucle simple (poll UART si besoin)
    while(1) {
        app_uart_poll(app);
        furi_delay_ms(50);
    }

    // Jamais atteint dans ce squelette
    return 0;
}
