#include <furi.h>
#include <gui/canvas.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <gui/elements.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <stdio.h>

#include "app.h"
#include "track.h"
#include "storage_helpers.h"

// Intervalle entre deux trkpts
#define TRACK_TICK_MS 5000

typedef struct {
    bool has_fix;
    float lat;
    float lon;
    float hdop;
    float altitude;
    uint8_t sats;
    uint32_t points;
    uint32_t duration_s;
    bool active;
} TrackModel;

void track_refresh(App* app) {
    if(!app->track_view) return;

    uint32_t duration = 0;
    if(app->track_start_tick != 0) {
        uint32_t freq = furi_kernel_get_tick_frequency();
        if(freq == 0) freq = 1;
        duration = (furi_get_tick() - app->track_start_tick) / freq;
    }

    with_view_model(
        app->track_view,
        TrackModel * m,
        {
            m->has_fix = app->has_fix;
            m->lat = app->lat;
            m->lon = app->lon;
            m->hdop = app->hdop;
            m->altitude = app->altitude;
            m->sats = app->sats;
            m->points = app->track_points;
            m->duration_s = duration;
            m->active = (app->track_timer != NULL);
        },
        true);
}

// Callback du timer : écrit un trkpt si on a un fix
static void track_timer_callback(void* ctx) {
    App* app = (App*)ctx;
    if(!app->has_fix) return; // pas de fix -> on saute ce tick

    storage_append_trkpt(app->lat, app->lon, app->altitude, app->track_new_segment);
    app->track_new_segment = false;
    app->track_points++;
    // note : le rafraîchissement visuel passe par le tick callback du ViewDispatcher
    // (qui appelle track_refresh via app.c), pas besoin de le faire ici.
}

static void track_draw_callback(Canvas* canvas, void* ctx) {
    TrackModel* m = (TrackModel*)ctx;

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(
        canvas, 64, 2, AlignCenter, AlignTop, m->active ? "REC" : "Idle");

    canvas_set_font(canvas, FontSecondary);

    char line1[48];
    char line2[48];
    if(m->has_fix) {
        snprintf(line1, sizeof(line1), "%.5f, %.5f", (double)m->lat, (double)m->lon);
        snprintf(
            line2,
            sizeof(line2),
            "HDOP=%.1f sats=%u alt=%.0fm",
            (double)m->hdop,
            m->sats,
            (double)m->altitude);
    } else {
        snprintf(line1, sizeof(line1), "Waiting for fix...");
        snprintf(line2, sizeof(line2), "sats=%u", m->sats);
    }
    // fall-through: Idle et REC déjà en anglais
    elements_multiline_text_aligned(canvas, 64, 18, AlignCenter, AlignTop, line1);
    elements_multiline_text_aligned(canvas, 64, 28, AlignCenter, AlignTop, line2);

    char line3[48];
    uint32_t h = m->duration_s / 3600;
    uint32_t min = (m->duration_s / 60) % 60;
    uint32_t s = m->duration_s % 60;
    snprintf(
        line3, sizeof(line3), "pts=%lu  %02lu:%02lu:%02lu",
        (unsigned long)m->points,
        (unsigned long)h, (unsigned long)min, (unsigned long)s);
    elements_multiline_text_aligned(canvas, 64, 42, AlignCenter, AlignTop, line3);

    elements_multiline_text_aligned(
        canvas, 64, 62, AlignCenter, AlignBottom, "Auto-log 5s  Back to stop");
}

static bool track_input_callback(InputEvent* event, void* ctx) {
    (void)event;
    (void)ctx;
    return false; // Back -> previous_callback -> menu
}

static uint32_t track_previous_callback(void* ctx) {
    (void)ctx;
    return AppViewMenu;
}

static void track_enter(void* ctx) {
    App* app = (App*)ctx;

    // Démarre la session : flag nouveau segment, reset compteurs
    app->track_new_segment = true;
    app->track_points = 0;
    app->track_start_tick = furi_get_tick();

    if(!app->track_timer) {
        app->track_timer =
            furi_timer_alloc(track_timer_callback, FuriTimerTypePeriodic, app);
    }
    if(app->track_timer) {
        furi_timer_start(app->track_timer, furi_ms_to_ticks(TRACK_TICK_MS));
    }

    track_refresh(app);
}

static void track_exit(void* ctx) {
    App* app = (App*)ctx;
    if(app->track_timer) {
        furi_timer_stop(app->track_timer);
    }
}

void app_start_track(App* app) {
    if(!app->track_view) {
        View* view = view_alloc();
        view_allocate_model(view, ViewModelTypeLockFree, sizeof(TrackModel));
        view_set_context(view, app);
        view_set_draw_callback(view, track_draw_callback);
        view_set_input_callback(view, track_input_callback);
        view_set_enter_callback(view, track_enter);
        view_set_exit_callback(view, track_exit);
        view_set_previous_callback(view, track_previous_callback);

        app->track_view = view;
        view_dispatcher_add_view(app->dispatcher, AppViewTrack, view);
    }
    view_dispatcher_switch_to_view(app->dispatcher, AppViewTrack);
}

void app_stop_track(App* app) {
    if(app->track_timer) {
        furi_timer_stop(app->track_timer);
        furi_timer_free(app->track_timer);
        app->track_timer = NULL;
    }
    if(app->track_view) {
        view_dispatcher_remove_view(app->dispatcher, AppViewTrack);
        view_free(app->track_view);
        app->track_view = NULL;
    }
}
