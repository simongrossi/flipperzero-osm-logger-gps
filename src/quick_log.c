#include <furi.h>
#include <gui/canvas.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "quick_log.h"
#include "presets.h"
#include "storage_helpers.h"

// Seuil de qualité GPS au-delà duquel on refuse OK court (force = OK long)
#define HDOP_MAX 2.5f

typedef struct {
    bool has_fix;
    float lat;
    float lon;
    float hdop;
    float altitude;
    uint8_t sats;
    uint8_t preset_idx;
    uint16_t session_count;
    uint32_t fix_age_s; // secondes écoulées depuis le dernier fix (0 si jamais eu)
    bool fix_ever;      // true si on a déjà eu un fix au moins une fois
} QuickLogModel;

static void quick_log_draw_callback(Canvas* canvas, void* ctx) {
    QuickLogModel* m = (QuickLogModel*)ctx;
    uint8_t idx = m->preset_idx < PRESETS_COUNT ? m->preset_idx : 0;
    const Preset* p = &PRESETS[idx];

    canvas_clear(canvas);

    // Header : label du preset
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 64, 2, AlignCenter, AlignTop, p->label);

    // Tag OSM
    char tag[48];
    snprintf(tag, sizeof(tag), "%s=%s", p->key, p->value);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 64, 16, AlignCenter, AlignTop, tag);

    // Coords + sats
    char line1[48];
    char line2[48];
    if(m->has_fix) {
        snprintf(line1, sizeof(line1), "%.5f, %.5f", (double)m->lat, (double)m->lon);
        snprintf(
            line2,
            sizeof(line2),
            "HDOP=%.1f sats=%u  #%u",
            (double)m->hdop,
            m->sats,
            m->session_count);
    } else {
        snprintf(line1, sizeof(line1), "No GPS fix");
        snprintf(line2, sizeof(line2), "sats=%u  #%u", m->sats, m->session_count);
    }
    elements_multiline_text_aligned(canvas, 64, 28, AlignCenter, AlignTop, line1);
    elements_multiline_text_aligned(canvas, 64, 38, AlignCenter, AlignTop, line2);

    // Ligne altitude + age du fix
    char line3[48];
    if(m->fix_ever) {
        snprintf(
            line3,
            sizeof(line3),
            "alt=%.0fm  fix=%lus",
            (double)m->altitude,
            (unsigned long)m->fix_age_s);
    } else {
        snprintf(line3, sizeof(line3), "alt=--  fix=--");
    }
    elements_multiline_text_aligned(canvas, 64, 48, AlignCenter, AlignTop, line3);

    // Footer
    elements_multiline_text_aligned(
        canvas, 64, 62, AlignCenter, AlignBottom, "OK save  Hold=force  Back");
}

static bool quick_log_input_callback(InputEvent* event, void* ctx) {
    App* app = (App*)ctx;

    bool ok_short = (event->type == InputTypeShort && event->key == InputKeyOk);
    bool ok_long = (event->type == InputTypeLong && event->key == InputKeyOk);

    if(ok_short || ok_long) {
        bool force = ok_long;
        bool quality_ok = app->has_fix && (app->hdop <= HDOP_MAX);

        if(!quality_ok && !force) {
            // Qualité insuffisante -> refus (seul OK long force la sauvegarde)
            notification_message(app->notification, &sequence_error);
            return true;
        }

        uint8_t idx = app->current_preset < PRESETS_COUNT ? app->current_preset : 0;
        const Preset* p = &PRESETS[idx];
        char tag[48];
        snprintf(tag, sizeof(tag), "%s=%s", p->key, p->value);
        storage_write_all_formats(
            app->lat, app->lon, app->altitude, app->hdop, app->sats, tag, NULL);
        app->session_count++;
        quick_log_refresh(app);
        notification_message(app->notification, &sequence_success);
        return true;
    }

    return false; // Back tombe sur previous_callback -> retour submenu presets
}

static uint32_t quick_log_previous_callback(void* ctx) {
    (void)ctx;
    return AppViewPresets;
}

static void quick_log_enter(void* ctx) {
    App* app = (App*)ctx;
    quick_log_refresh(app);
}
static void quick_log_exit(void* ctx) {
    (void)ctx;
}

void quick_log_refresh(App* app) {
    if(!app->quick_view) return;

    uint32_t age_s = 0;
    bool fix_ever = (app->last_fix_tick != 0);
    if(fix_ever) {
        uint32_t freq = furi_kernel_get_tick_frequency();
        if(freq == 0) freq = 1;
        age_s = (furi_get_tick() - app->last_fix_tick) / freq;
    }

    with_view_model(
        app->quick_view,
        QuickLogModel * m,
        {
            m->has_fix = app->has_fix;
            m->lat = app->lat;
            m->lon = app->lon;
            m->hdop = app->hdop;
            m->altitude = app->altitude;
            m->sats = app->sats;
            m->preset_idx = app->current_preset;
            m->session_count = app->session_count;
            m->fix_age_s = age_s;
            m->fix_ever = fix_ever;
        },
        true);
}

void app_start_quick_log(App* app) {
    if(!app->quick_view) {
        View* view = view_alloc();
        view_allocate_model(view, ViewModelTypeLockFree, sizeof(QuickLogModel));
        view_set_context(view, app);
        view_set_draw_callback(view, quick_log_draw_callback);
        view_set_input_callback(view, quick_log_input_callback);
        view_set_enter_callback(view, quick_log_enter);
        view_set_exit_callback(view, quick_log_exit);
        view_set_previous_callback(view, quick_log_previous_callback);

        app->quick_view = view;
        view_dispatcher_add_view(app->dispatcher, AppViewQuickLog, view);
    }
    view_dispatcher_switch_to_view(app->dispatcher, AppViewQuickLog);
}

void app_stop_quick_log(App* app) {
    if(app->quick_view) {
        view_dispatcher_remove_view(app->dispatcher, AppViewQuickLog);
        view_free(app->quick_view);
        app->quick_view = NULL;
    }
}
