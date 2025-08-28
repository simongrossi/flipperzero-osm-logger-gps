
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
    uint32_t last_log_tick;
    uint8_t idx;          // index preset
    uint32_t view_id;     // ID de la vue (pour refresh)
} QuickLog;

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
    if(now - ql->last_log_tick < 800) return;

    double lat=0, lon=0; float hdop=99; uint8_t sats=0;
    bool valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv=0;
    const char* variant = app_get_preset_variant(ql->app, ql->idx, &hv);
    const char* quality = (valid && hdop <= 2.5f) ? "ok" : "low";

    if(!valid && !force_or_note) { ql->last_log_tick = now; return; }
    if(valid && (hdop > 2.5f) && !force_or_note) { ql->last_log_tick = now; return; }

    char note[13] = {0}; // Note: la taille est en dur ici, voir point suivant
    bool ok = app_save_point(ql->app, key, variant, note, lat, lon, hdop, sats, quality);
    
    // --- DÉBUT DE LA MODIFICATION ---
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    if(ok) {
        // Vibration courte pour succès
        notification_message(notifications, &sequence_success);
    } else {
        // Vibration plus longue pour erreur (si la sauvegarde échoue)
        notification_message(notifications, &sequence_error);
    }
    furi_record_close(RECORD_NOTIFICATION);
    // --- FIN DE LA MODIFICATION ---

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
        case InputKeyBack: {
            ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
            if(vd) view_dispatcher_switch_to_view(vd, 0);
        } break;
        default: break;
    }

    if(need_redraw) {
        // Forcer un redraw en revenant sur la même vue
        ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
        if(vd) view_dispatcher_switch_to_view(vd, ql->view_id);
    }
    return true;
}

void quicklog_start(App* app) {
    QuickLog* ql = (QuickLog*)malloc(sizeof(QuickLog));
    memset(ql, 0, sizeof(QuickLog));
    ql->app = app;

    View* view = view_alloc();
    ql->view = view;
    view_set_context(view, ql);
    view_set_draw_callback(view, quicklog_draw);
    view_set_input_callback(view, quicklog_input);

    ViewDispatcher* vd = app_get_view_dispatcher(app);
    ql->view_id = 7;
    view_dispatcher_add_view(vd, ql->view_id, view);
    view_dispatcher_switch_to_view(vd, ql->view_id);
}
