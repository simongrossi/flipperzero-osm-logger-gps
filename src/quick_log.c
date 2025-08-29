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
#define MAIN_MENU_VIEW_ID 0
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
    uint8_t idx;
} QuickLog;

// --- Forward declarations ---
static void quicklog_request_redraw(QuickLog* ql);
static void quicklog_enter(void* context);
static void quicklog_exit(void* context);
static uint32_t quicklog_previous_callback(void* context);

// --- DRAW ---
static void quicklog_draw(Canvas* canvas, void* context) {
    QuickLog* ql = (QuickLog*)context;
    canvas_clear(canvas);

    double lat = 0.0, lon = 0.0;
    float hdop = 99.0f;
    uint8_t sats = 0;
    bool valid = false;

    if(ql && ql->app) {
        valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);
    }

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv = 0;
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
        snprintf(line2, sizeof(line2), "Preset: %s  [%s]", key ? key : "--", var);
    } else {
        snprintf(line2, sizeof(line2), "Preset: %s", key ? key : "--");
    }
    canvas_draw_str(canvas, 4, 42, line2);

    canvas_draw_str(canvas, 4, 58, "OK=Log  OK long=Force  ^v Preset  <> Variant");
}

// --- LOG ---
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

    if(ql->notifications) {
        if(ok)  notification_message(ql->notifications, &sequence_success);
        else    notification_message(ql->notifications, &sequence_error);
    }
    ql->last_log_tick = now;
}

// --- INPUT ---
static bool quicklog_input(InputEvent* evt, void* context) {
    QuickLog* ql = (QuickLog*)context;
    if(evt->type != InputTypeShort && evt->type != InputTypeLong) return false;

    // BACK: laisser le dispatcher appeler previous_callback -> cleanup sûr
    if(evt->key == InputKeyBack && evt->type == InputTypeShort) {
        return false;
    }

    bool need_redraw = false;

    switch(evt->key) {
        case InputKeyUp: {
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) { ql->idx = (uint8_t)((ql->idx + cnt - 1) % cnt); need_redraw = true; }
            }
        } break;
        case InputKeyDown: {
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) { ql->idx = (uint8_t)((ql->idx + 1) % cnt); need_redraw = true; }
            }
        } break;
        case InputKeyOk:
            if(evt->type == InputTypeShort) quicklog_do_log(ql, false);
            if(evt->type == InputTypeLong)  quicklog_do_log(ql, true);
            break;
        default: break;
    }

    if(need_redraw) quicklog_request_redraw(ql);
    return true;
}

// --- Lifecycle callbacks ---
static void quicklog_enter(void* context) {
    QuickLog* ql = (QuickLog*)context;
    ql->notifications = furi_record_open(RECORD_NOTIFICATION);
}

static void quicklog_exit(void* context) {
    QuickLog* ql = (QuickLog*)context;
    if(ql->notifications) {
        furi_record_close(RECORD_NOTIFICATION);
        ql->notifications = NULL;
    }
}

// --- Previous callback: retourne la vue menu et nettoie proprement ---
static uint32_t quicklog_previous_callback(void* context) {
    QuickLog* ql = (QuickLog*)context;
    ViewDispatcher* vd = app_get_view_dispatcher(ql->app);

    // Démonte la vue de manière ordonnée
    view_dispatcher_remove_view(vd, QUICK_LOG_VIEW_ID);

    if(ql->notifications) {
        furi_record_close(RECORD_NOTIFICATION);
        ql->notifications = NULL;
    }
    if(ql->view) {
        view_free(ql->view);
        ql->view = NULL;
    }
    free(ql);

    return MAIN_MENU_VIEW_ID;
}

// --- Helper redraw sûr ---
static void quicklog_request_redraw(QuickLog* ql) {
    ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
    view_dispatcher_switch_to_view(vd, QUICK_LOG_VIEW_ID);
}

// --- START ---
void quicklog_start(App* app) {
    QuickLog* ql = (QuickLog*)malloc(sizeof(QuickLog));
    memset(ql, 0, sizeof(QuickLog));
    ql->app = app;

    View* view = view_alloc();
    ql->view = view; // important pour redraw et cleanup
    view_set_context(view, ql);
    view_set_draw_callback(view, quicklog_draw);
    view_set_input_callback(view, quicklog_input);
    view_set_enter_callback(view, quicklog_enter);
    view_set_exit_callback(view, quicklog_exit);
    view_set_previous_callback(view, quicklog_previous_callback);

    ViewDispatcher* vd = app_get_view_dispatcher(app);
    view_dispatcher_add_view(vd, QUICK_LOG_VIEW_ID, view);
    view_dispatcher_switch_to_view(vd, QUICK_LOG_VIEW_ID);
}
