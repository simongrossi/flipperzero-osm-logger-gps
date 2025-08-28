#include <furi.h>
#include <gui/canvas.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

// --- CONSTANTES ---
#define MAX_HDOP_TO_LOG   2.5f
#define LOG_DEBOUNCE_MS   800
#define MAX_NOTE_LEN      13
#define QUICK_LOG_VIEW_ID 7
// -------------------

typedef struct App App;  // forward-declare

// Interfaces fournies par app.c
bool app_get_fix(App* app, double* lat, double* lon, float* hdop, uint8_t* sats);
uint8_t app_get_preset_count(App* app);
const char* app_get_preset_key(App* app, uint8_t idx);
const char* app_get_preset_variant(App* app, uint8_t idx, uint8_t* has_variant);
ViewDispatcher* app_get_view_dispatcher(App* app);
bool app_save_point(App* app, const char* key, const char* variant, const char* note,
                    double lat, double lon, float hdop, uint8_t sats, const char* quality);

typedef struct {
    App* app;
    View* view;
    NotificationApp* notifications;
    uint32_t last_log_tick;
    uint8_t idx;          // index preset
    uint32_t view_id;     // ID de la vue (pour refresh)
} QuickLog;

// petit helper pour cleanup
static void quicklog_stop(QuickLog* ql) {
    if(!ql) return;
    ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
    if(vd) {
        // revenir au menu AVANT de retirer/libérer la vue
        view_dispatcher_switch_to_view(vd, 0);
        view_dispatcher_remove_view(vd, ql->view_id);
    }
    if(ql->view) view_free(ql->view);
    if(ql->notifications) furi_record_close(RECORD_NOTIFICATION);
    free(ql);
}

static void quicklog_draw(Canvas* canvas, void* ctx) {
    QuickLog* ql = (QuickLog*)ctx;
    canvas_clear(canvas);
    double lat=0, lon=0; float hdop=99; uint8_t sats=0;
    bool valid = false;
    if(ql->app) valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv=0;
    const char* var = app_get_preset_variant(ql->app, ql->idx, &hv);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 14, "Mode Rapide");

    canvas_set_font(canvas, FontSecondary);
    char line1[64];
    snprintf(line1, sizeof(line1), "Fix:%s HDOP:%.1f Sats:%u",
             valid ? "OK" : "--", (double)hdop, (unsigned)sats);
    canvas_draw_str(canvas, 4, 28, line1);

    char line2[96];
    if(hv && var && var[0]) {
        snprintf(line2, sizeof(line2), "Preset: %s  [%s]", key?key:"--", var);
    } else {
        snprintf(line2, sizeof(line2), "Preset: %s", key?key:"--");
    }
    canvas_draw_str(canvas, 4, 42, line2);

    canvas_draw_str(canvas, 4, 58, "OK=Log  OK long=Force  ^v Preset  <> Variant");
}

static void quicklog_do_log(QuickLog* ql, bool force_or_note) {
    uint32_t now = furi_get_tick();
    if(now - ql->last_log_tick < LOG_DEBOUNCE_MS) return;

    double lat=0, lon=0; float hdop=99; uint8_t sats=0;
    bool valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv=0;
    const char* variant = app_get_preset_variant(ql->app, ql->idx, &hv);
    const char* quality = (valid && hdop <= MAX_HDOP_TO_LOG) ? "ok" : "low";

    if(!valid && !force_or_note) { ql->last_log_tick = now; return; }
    if(valid && (hdop > MAX_HDOP_TO_LOG) && !force_or_note) { ql->last_log_tick = now; return; }

    char note[MAX_NOTE_LEN] = {0};
    bool ok = app_save_point(ql->app, key, variant, note, lat, lon, hdop, sats, quality);

    // vibration feedback (succès/erreur)
    if(ql->notifications) {
        if(ok)  notification_message(ql->notifications, &sequence_success);
        else    notification_message(ql->notifications, &sequence_error);
    }

    ql->last_log_tick = now;
}

static bool quicklog_input(InputEvent* evt, void* ctx) {
    QuickLog* ql = (QuickLog*)ctx;
    if(evt->type != InputTypeShort && evt->type != InputTypeLong) return false;

    bool need_redraw = false;

    switch(evt->key) {
        case InputKeyUp:
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) { ql->idx = (uint8_t)((ql->idx + cnt - 1) % cnt); need_redraw = true; }
            }
            break;
        case InputKeyDown:
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) { ql->idx = (uint8_t)((ql->idx + 1) % cnt); need_redraw = true; }
            }
            break;
        case InputKeyLeft:
        case InputKeyRight:
            // variants pas encore implémentés
            break;
        case InputKeyOk:
            if(evt->type == InputTypeShort) quicklog_do_log(ql, false);
            if(evt->type == InputTypeLong)  quicklog_do_log(ql, true);
            break;
        case InputKeyBack:
            quicklog_stop(ql);
            return true; // on a géré l’event
        default:
            break;
    }

    if(need_redraw) {
        // Forcer un redraw en revenant sur la même vue
        ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
        if(vd) view_dispatcher_switch_to_view(vd, ql->view_id);
    }
    return true;
}

void quicklog_start(App* app) {
    // Évite de ré-enregistrer la vue si elle existe déjà
    static bool s_registered = false;
    if(s_registered) {
        ViewDispatcher* vd = app_get_view_dispatcher(app);
        if(vd) view_dispatcher_switch_to_view(vd, QUICK_LOG_VIEW_ID);
        return;
    }

    QuickLog* ql = (QuickLog*)malloc(sizeof(QuickLog));
    memset(ql, 0, sizeof(QuickLog));
    ql->app = app;
    ql->view_id = QUICK_LOG_VIEW_ID;
    ql->notifications = furi_record_open(RECORD_NOTIFICATION); // cache le handle

    ql->view = view_alloc();
    view_set_context(ql->view, ql);
    view_set_draw_callback(ql->view, quicklog_draw);
    view_set_input_callback(ql->view, quicklog_input);

    ViewDispatcher* vd = app_get_view_dispatcher(app);
    view_dispatcher_add_view(vd, ql->view_id, ql->view);
    view_dispatcher_switch_to_view(vd, ql->view_id);

    s_registered = true;

    // NB: on remettra s_registered=false quand on quittera la vue (Back) via quicklog_stop()
}
