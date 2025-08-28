// app.c — main app with proper dispatcher run + SDK 1.3.x navigation callback
#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct App App;

void storage_write_all_formats(float lat, float lon, float hdop, uint8_t sats,
                               const char* k, const char* v, const char* note);
void quicklog_start(App* app);

enum { ViewIdMainMenu = 0, };

struct App {
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* submenu;
    bool has_fix;
    double lat, lon;
    float hdop;
    uint8_t sats;
};

/* ===== Navigation (Back) =====
 * For SDK 1.3.x, view_dispatcher_set_navigation_event_callback takes a callback
 * with signature: bool (*)(void* ctx). It is called on Back events.
 */
static bool app_nav_callback(void* ctx) {
    App* app = (App*)ctx;
    // Quitter proprement l'application (sort de view_dispatcher_run)
    view_dispatcher_stop(app->dispatcher);
    return true; // évènement consommé
}

/* ====== Submenu callbacks ====== */
static void enter_classic_cb(void* ctx, uint32_t index) {
    (void)index;
    App* app = (App*)ctx;
    // TODO: implémenter le mode "classique"
    (void)app;
}

static void enter_quicklog_cb(void* ctx, uint32_t index) {
    (void)index;
    App* app = (App*)ctx;
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

/* ====== Interfaces exposées pour quick_log ====== */
bool app_get_fix(App* app, double* lat, double* lon, float* hdop, uint8_t* sats) {
    if(lat) *lat = app->lat;
    if(lon) *lon = app->lon;
    if(hdop) *hdop = app->hdop;
    if(sats) *sats = app->sats;
    return app->has_fix;
}

static const char* k_presets[] = {"bench","waste_basket","drinking_water","toilets"};
uint8_t app_get_preset_count(App* app){ (void)app; return (uint8_t)(sizeof(k_presets)/sizeof(k_presets[0])); }
const char* app_get_preset_key(App* app, uint8_t idx){ (void)app; return k_presets[idx % app_get_preset_count(app)]; }
const char* app_get_preset_variant(App* app, uint8_t idx, uint8_t* has_variant){ (void)app;(void)idx; if(has_variant) *has_variant=0; return ""; }

ViewDispatcher* app_get_view_dispatcher(App* app){ return app->dispatcher; }

bool app_save_point(App* app, const char* key, const char* variant, const char* note,
                    double lat, double lon, float hdop, uint8_t sats, const char* quality) {
    (void)variant; (void)quality; (void)app;
    storage_write_all_formats((float)lat, (float)lon, hdop, sats, "amenity", key, note?note:"");
    return true;
}

/* ====== Entrée principale ====== */
int32_t app(void* p) {
    (void)p;
    App* app = (App*)malloc(sizeof(App));
    memset(app, 0, sizeof(App));

    app->gui = furi_record_open(RECORD_GUI);
    app->dispatcher = view_dispatcher_alloc();

    // Attache GUI + contexte et callback navigation (SDK 1.3.x)
    view_dispatcher_attach_to_gui(app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->dispatcher, app_nav_callback);

    app_show_main_menu(app);

    // Boucle évènementielle (bloquante) — stop via app_nav_callback()
    view_dispatcher_run(app->dispatcher);

    // Nettoyage
    view_dispatcher_remove_view(app->dispatcher, ViewIdMainMenu);
    submenu_free(app->submenu);
    view_dispatcher_free(app->dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
