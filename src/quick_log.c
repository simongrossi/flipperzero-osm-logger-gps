#include <furi.h>
#include <furi_hal_power.h>
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
    uint8_t variant_idx;
    uint8_t variant_count;
    uint16_t session_count;
    uint32_t total_count;
    uint32_t fix_age_s;
    bool fix_ever;
    bool preview_pending;
    char note[64];
} QuickLogModel;

static void quick_log_draw_callback(Canvas* canvas, void* ctx) {
    QuickLogModel* m = (QuickLogModel*)ctx;
    const Preset* p = presets_get(m->preset_idx);
    if(!p) p = presets_get(0);
    if(!p) return;

    canvas_clear(canvas);

    // Mode preview (confirmation) : écran distinct et pédagogique.
    if(m->preview_pending) {
        canvas_set_font(canvas, FontPrimary);
        elements_multiline_text_aligned(
            canvas, 64, 2, AlignCenter, AlignTop, "Save this point?");

        canvas_set_font(canvas, FontSecondary);
        char tag[64];
        preset_build_tag(p, m->variant_idx, tag, sizeof(tag));
        elements_multiline_text_aligned(canvas, 64, 18, AlignCenter, AlignTop, tag);

        char line[80];
        if(m->has_fix) {
            snprintf(line, sizeof(line), "%.6f, %.6f", (double)m->lat, (double)m->lon);
        } else {
            snprintf(line, sizeof(line), "NO FIX (will save 0,0)");
        }
        elements_multiline_text_aligned(canvas, 64, 30, AlignCenter, AlignTop, line);

        snprintf(line, sizeof(line), "HDOP=%.1f sats=%u", (double)m->hdop, m->sats);
        elements_multiline_text_aligned(canvas, 64, 40, AlignCenter, AlignTop, line);

        if(m->note[0]) {
            snprintf(line, sizeof(line), "note: %s", m->note);
            elements_multiline_text_aligned(canvas, 64, 50, AlignCenter, AlignTop, line);
        }

        elements_multiline_text_aligned(
            canvas, 64, 62, AlignCenter, AlignBottom, "OK=save  Back=cancel");
        return;
    }

    // Badge batterie en haut à droite
    char bat[8];
    snprintf(bat, sizeof(bat), "%u%%", (unsigned)furi_hal_power_get_pct());
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 127, 2, AlignRight, AlignTop, bat);

    // Header : label du preset (avec marqueur de variante si plusieurs)
    canvas_set_font(canvas, FontPrimary);
    if(m->variant_count > 1) {
        char header[48];
        snprintf(
            header,
            sizeof(header),
            "< %s %u/%u >",
            p->label,
            (unsigned)(m->variant_idx + 1),
            (unsigned)m->variant_count);
        elements_multiline_text_aligned(canvas, 64, 2, AlignCenter, AlignTop, header);
    } else {
        elements_multiline_text_aligned(canvas, 64, 2, AlignCenter, AlignTop, p->label);
    }

    // Tag OSM effectif (variantes simples OU tags additionnels)
    char tag[64];
    preset_build_tag(p, m->variant_idx, tag, sizeof(tag));
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

    // Ligne altitude + age du fix + total
    char line3[56];
    if(m->fix_ever) {
        snprintf(
            line3,
            sizeof(line3),
            "alt=%.0fm fix=%lus  t=%lu",
            (double)m->altitude,
            (unsigned long)m->fix_age_s,
            (unsigned long)m->total_count);
    } else {
        snprintf(line3, sizeof(line3), "alt=-- fix=--  t=%lu", (unsigned long)m->total_count);
    }
    elements_multiline_text_aligned(canvas, 64, 48, AlignCenter, AlignTop, line3);

    // Footer : si note non-vide l'afficher, sinon rappel touches
    if(m->note[0]) {
        char footer[80];
        snprintf(footer, sizeof(footer), "note: %s", m->note);
        elements_multiline_text_aligned(canvas, 64, 62, AlignCenter, AlignBottom, footer);
    } else {
        elements_multiline_text_aligned(
            canvas, 64, 62, AlignCenter, AlignBottom, "OK save  Up:note  <>:var");
    }
}

static bool quick_log_do_save(App* app, bool force) {
    const Preset* p = presets_get(app->current_preset);
    if(!p) p = presets_get(0);
    if(!p) return false;

    bool quality_ok = app->has_fix && (app->hdop <= HDOP_MAX);
    if(!quality_ok && !force) {
        notification_message(app->notification, &sequence_error);
        return false;
    }

    char tag[64];
    preset_build_tag(p, app->current_variant, tag, sizeof(tag));
    const char* note = app->quick_note[0] ? app->quick_note : NULL;
    storage_write_all_formats(
        app->lat, app->lon, app->altitude, app->hdop, app->sats, tag, note);
    app->session_count++;
    app->total_count++;
    notification_message(app->notification, &sequence_success);
    return true;
}

static bool quick_log_input_callback(InputEvent* event, void* ctx) {
    App* app = (App*)ctx;

    bool ok_short = (event->type == InputTypeShort && event->key == InputKeyOk);
    bool ok_long = (event->type == InputTypeLong && event->key == InputKeyOk);
    bool is_press = (event->type == InputTypeShort || event->type == InputTypeRepeat);

    // Mode preview (confirmation) : gestion spéciale des touches
    if(app->preview_pending) {
        if(ok_short) {
            quick_log_do_save(app, false); // force=false : respecte gate HDOP
            app->preview_pending = false;
            quick_log_refresh(app);
            return true;
        }
        if(ok_long) {
            quick_log_do_save(app, true); // force=true : ignore gate
            app->preview_pending = false;
            quick_log_refresh(app);
            return true;
        }
        if(event->type == InputTypeShort && event->key == InputKeyBack) {
            // Annule le preview, reste sur Quick Log
            app->preview_pending = false;
            quick_log_refresh(app);
            return true;
        }
        return true; // en preview, on bloque les autres touches
    }

    // ←/→ : cycle sur les variantes du preset courant
    if(is_press && (event->key == InputKeyLeft || event->key == InputKeyRight)) {
        const Preset* p = presets_get(app->current_preset);
        if(p && p->variant_count > 1) {
            if(event->key == InputKeyRight) {
                app->current_variant = (uint8_t)((app->current_variant + 1) % p->variant_count);
            } else {
                app->current_variant = app->current_variant == 0
                                           ? (uint8_t)(p->variant_count - 1)
                                           : (uint8_t)(app->current_variant - 1);
            }
            quick_log_refresh(app);
        }
        return true;
    }

    // UP : ouvre l'éditeur de note
    if(event->type == InputTypeShort && event->key == InputKeyUp) {
        view_dispatcher_switch_to_view(app->dispatcher, AppViewNote);
        return true;
    }

    // DOWN : efface la note courante
    if(event->type == InputTypeShort && event->key == InputKeyDown) {
        app->quick_note[0] = '\0';
        quick_log_refresh(app);
        return true;
    }

    if(ok_short || ok_long) {
        // Si preview activé et OK court : aller en mode preview au lieu de sauver direct.
        // OK long court-circuite toujours le preview (force save).
        if(ok_short && app->settings.preview_before_save) {
            app->preview_pending = true;
            quick_log_refresh(app);
            return true;
        }
        quick_log_do_save(app, ok_long);
        quick_log_refresh(app);
        return true;
    }

    return false;
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
            m->variant_idx = app->current_variant;
            const Preset* p = presets_get(app->current_preset);
            m->variant_count = p ? p->variant_count : 1;
            m->session_count = app->session_count;
            m->total_count = app->total_count;
            m->fix_age_s = age_s;
            m->fix_ever = fix_ever;
            m->preview_pending = app->preview_pending;
            strncpy(m->note, app->quick_note, sizeof(m->note) - 1);
            m->note[sizeof(m->note) - 1] = '\0';
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
