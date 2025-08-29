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
#include "storage_helpers.h"

// Custom event -> demander un redraw
static bool quick_log_custom_callback(uint32_t event, void* ctx) {
    (void)ctx;
    (void)event;
    return true; // true => invalider/redessiner
}

// --- Dessin de l'écran Quick Log
static void quick_log_draw_callback(Canvas* canvas, void* ctx) {
    App* app = (App*)ctx;

    canvas_clear(canvas);
    elements_multiline_text_aligned(canvas, 64, 4, AlignCenter, AlignTop, "Quick Log");

    char line1[64];
    char line2[64];

    if(app->has_fix) {
        snprintf(line1, sizeof(line1), "lat=%.5f lon=%.5f",
                 (double)app->lat, (double)app->lon);
        snprintf(line2, sizeof(line2), "HDOP=%.1f sats=%u",
                 (double)app->hdop, app->sats);
    } else {
        snprintf(line1, sizeof(line1), "No GPS fix");
        snprintf(line2, sizeof(line2), "sats=%u", app->sats);
    }

    elements_multiline_text_aligned(canvas, 64, 24, AlignCenter, AlignTop, line1);
    elements_multiline_text_aligned(canvas, 64, 36, AlignCenter, AlignTop, line2);

    elements_multiline_text_aligned(canvas, 64, 60, AlignCenter, AlignBottom,
                                    "OK: save point  •  Back: exit");
}

// --- Gestion des entrées
static bool quick_log_input_callback(InputEvent* event, void* ctx) {
    App* app = (App*)ctx;

    if(event->type == InputTypeShort && event->key == InputKeyBack) {
        view_dispatcher_switch_to_view(app->dispatcher, AppViewMenu);
        return true;
    }

    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        // Écrit dans tous les formats via helpers
        storage_write_all_formats(app->lat, app->lon, NULL);
        notification_message(app->notification, &sequence_success);
        return true;
    }

    return false;
}

static void quick_log_enter(void* ctx) { (void)ctx; }
static void quick_log_exit(void* ctx) { (void)ctx; }

// --- API d’écran Quick Log
void app_start_quick_log(App* app) {
    if(!app->quick_view) {
        View* view = view_alloc();
        view_set_context(view, app);
        view_set_draw_callback(view, quick_log_draw_callback);
        view_set_input_callback(view, quick_log_input_callback);
        view_set_enter_callback(view, quick_log_enter);
        view_set_exit_callback(view, quick_log_exit);
        // Accept custom events -> génère un redraw
        view_set_custom_callback(view, quick_log_custom_callback);

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
