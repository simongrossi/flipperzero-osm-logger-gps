#include "quick_log.h"
#include <furi.h>
#include <gui/canvas.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define QUICKLOG_HDOP_DEFAULT 2.5f
#define QUICKLOG_ANTI_DBL_MS  800
#define QUICKLOG_NOTE_MAX     12

#ifndef HAVE_APP_SAVE_POINT
#define QUICKLOG_EMBED_JSONL 1
#endif

#ifdef QUICKLOG_EMBED_JSONL
#include <storage/storage.h>
static bool quicklog_write_jsonl(double lat, double lon, float hdop, uint8_t sats,
                                 const char* key, const char* variant, const char* note,
                                 const char* quality) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) return false;
    const char* dir = "/ext/apps_data/osm_logger";
    const char* path = "/ext/apps_data/osm_logger/points.jsonl";

    FileInfo fi;
    if(storage_common_stat(storage, dir, &fi) != FSE_OK) {
        storage_common_mkdir(storage, dir);
    }

    File* fp = storage_file_alloc(storage);
    if(storage_file_open(storage, fp, path, FSAM_WRITE, FSOM_OPEN_APPEND) != FSE_OK) {
        storage_file_free(fp);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    // timestamp ISO8601 UTC (RTC)
    FuriHalRtcDateTime dt; furi_hal_rtc_get_datetime(&dt);
    char ts[25];
    snprintf(ts, sizeof(ts), "%04u-%02u-%02uT%02u:%02u:%02uZ",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);

    char line[512];
    snprintf(line, sizeof(line),
        "{\"ts\":\"%s\",\"lat\":%.6f,\"lon\":%.6f,\"hdop\":%.1f,\"sats\":%u,"
        "\"poi_key\":\"%s\",\"variant\":\"%s\",\"note\":\"%s\",\"quality\":\"%s\","
        "\"source\":\"flipperzero-osm-logger-gps\"}",
        ts, lat, lon, hdop, sats, key ? key : "", variant ? variant : "",
        note ? note : "", quality ? quality : "ok");

    size_t n = storage_file_write(storage, fp, line, strlen(line));
    storage_file_write(storage, fp, "\n", 1);
    storage_file_close(storage, fp);
    storage_file_free(fp);
    furi_record_close(RECORD_STORAGE);
    return n == strlen(line);
}
#endif

// ==== Interfaces attendues côté App (déclarées ailleurs) ====
bool app_get_fix(App* app, double* lat, double* lon, float* hdop, uint8_t* sats);
uint8_t app_get_preset_count(App* app);
const char* app_get_preset_key(App* app, uint8_t idx);
const char* app_get_preset_variant(App* app, uint8_t idx, uint8_t* has_variant);
ViewDispatcher* app_get_view_dispatcher(App* app);
#ifdef HAVE_APP_SAVE_POINT
bool app_save_point(App* app, const char* key, const char* variant, const char* note,
                    double lat, double lon, float hdop, uint8_t sats, const char* quality);
#endif

typedef struct {
    App* app;
    View* view;
    uint32_t last_log_tick;
    float min_hdop;
    uint8_t idx;          // index preset
    uint8_t has_variant;  // 0/1
    uint8_t variant_idx;  // index variant courant
} QuickLog;

static void quicklog_draw(Canvas* canvas, void* ctx) {
    QuickLog* ql = ctx;
    canvas_clear(canvas);
    // Récup fix
    double lat=0, lon=0; float hdop=99; uint8_t sats=0;
    bool valid = false;
    if(ql->app) valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv=0;
    const char* var = app_get_preset_variant(ql->app, ql->idx, &hv);

    // Titre
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 14, "Mode Rapide");

    // Ligne état GPS
    canvas_set_font(canvas, FontSecondary);
    char line1[64];
    snprintf(line1, sizeof(line1), "Fix:%s HDOP:%.1f Sats:%u",
             valid ? "OK" : "--", hdop, sats);
    canvas_draw_str(canvas, 4, 28, line1);

    // Preset + variant
    char line2[96];
    if(hv && var && var[0]) {
        snprintf(line2, sizeof(line2), "Preset: %s  [%s]", key?key:"--", var);
    } else {
        snprintf(line2, sizeof(line2), "Preset: %s", key?key:"--");
    }
    canvas_draw_str(canvas, 4, 42, line2);

    // Hint
    canvas_draw_str(canvas, 4, 58, "OK=Log  OK long=Force/Note  ^v Preset  <> Variant");
}

static void quicklog_note_popup(char* buf, size_t n) {
    if(n) buf[0] = 0; // Pas de saisie pour version minimale
}

static void quicklog_do_log(QuickLog* ql, bool force_or_note) {
    uint32_t now = furi_get_tick();
    if(now - ql->last_log_tick < QUICKLOG_ANTI_DBL_MS) return;

    double lat=0, lon=0; float hdop=99; uint8_t sats=0;
    bool valid = app_get_fix(ql->app, &lat, &lon, &hdop, &sats);

    const char* key = app_get_preset_key(ql->app, ql->idx);
    uint8_t hv=0;
    const char* variant = app_get_preset_variant(ql->app, ql->idx, &hv);
    const char* quality = (valid && hdop <= QUICKLOG_HDOP_DEFAULT) ? "ok" : "low";

    if(!valid && !force_or_note) {
        ql->last_log_tick = now;
        return;
    }
    if(valid && (hdop > QUICKLOG_HDOP_DEFAULT) && !force_or_note) {
        ql->last_log_tick = now;
        return;
    }

    char note[QUICKLOG_NOTE_MAX+1] = {0};
    if(force_or_note) {
        quicklog_note_popup(note, sizeof(note)); // vide par défaut
    }

    bool ok = false;
#ifdef HAVE_APP_SAVE_POINT
    ok = app_save_point(ql->app, key, variant, note, lat, lon, hdop, sats, quality);
#else
    ok = quicklog_write_jsonl(lat, lon, hdop, sats, key, variant, note, quality);
#endif
    (void)ok;
    ql->last_log_tick = now;
}

static bool quicklog_input(InputEvent* evt, void* ctx) {
    QuickLog* ql = ctx;
    if(evt->type != InputTypeShort && evt->type != InputTypeLong) return false;

    switch(evt->key) {
        case InputKeyUp: {
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) ql->idx = (ql->idx + cnt - 1) % cnt;
            }
        } break;
        case InputKeyDown: {
            if(evt->type == InputTypeShort) {
                uint8_t cnt = app_get_preset_count(ql->app);
                if(cnt) ql->idx = (ql->idx + 1) % cnt;
            }
        } break;
        case InputKeyLeft:
        case InputKeyRight:
            // Variants non gérés ici (hook si ton app en fournit)
            break;
        case InputKeyOk:
            if(evt->type == InputTypeShort) quicklog_do_log(ql, false);
            if(evt->type == InputTypeLong)  quicklog_do_log(ql, true);
            break;
        case InputKeyBack: {
            ViewDispatcher* vd = app_get_view_dispatcher(ql->app);
            if(vd) view_dispatcher_switch_to_view(vd, 0 /* menu principal attendu */);
        } break;
        default: break;
    }
    return true;
}

void quicklog_start(App* app) {
    QuickLog* ql = malloc(sizeof(QuickLog));
    memset(ql, 0, sizeof(QuickLog));
    ql->app = app;

    View* view = view_alloc();
    ql->view = view;
    view_set_context(view, ql);
    view_set_draw_callback(view, quicklog_draw);
    view_set_input_callback(view, quicklog_input);

    ViewDispatcher* vd = app_get_view_dispatcher(app);
    // ViewId 7 arbitraire pour demo
    const uint32_t kQuickViewId = 7;
    view_dispatcher_add_view(vd, kQuickViewId, view);
    view_dispatcher_switch_to_view(vd, kQuickViewId);
}
