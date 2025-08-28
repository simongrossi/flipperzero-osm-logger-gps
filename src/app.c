#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include "nmea.h"
#include "storage_helpers.h"

#define UART_BAUD 9600
#define UART_BUFF 128

typedef struct {
    Gui* gui;
    ViewPort* vp;
    FuriMessageQueue* input_queue;
    bool running;
    bool has_fix;
    float lat, lon, hdop;
    uint8_t sats;
    char line1[64];
} AppState;

static void draw_callback(Canvas* canvas, void* ctx) {
    AppState* app = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 14, "OSM Logger");
    canvas_set_font(canvas, FontSecondary);
    snprintf(app->line1, sizeof(app->line1), "Fix:%s lat:%.6f lon:%.6f", app->has_fix?"OK":"..", app->lat, app->lon);
    canvas_draw_str(canvas, 2, 30, app->line1);
    canvas_draw_str(canvas, 2, 60, "OK: Point | Back: Quitter");
}

static void input_callback(InputEvent* event, void* ctx) {
    AppState* app = ctx;
    furi_message_queue_put(app->input_queue, event, 0);
}

static void save_point(AppState* app) {
    const char* k = "amenity";
    const char* v = "bench";
    const char* note = "";
    if(app->has_fix) {
        storage_write_all_formats(app->lat, app->lon, app->hdop, app->sats, k, v, note);
    }
}

int32_t app(void* p) {
    UNUSED(p);
    AppState* app = malloc(sizeof(AppState));
    memset(app, 0, sizeof(AppState));
    app->running = true;

    app->gui = furi_record_open(RECORD_GUI);
    app->vp = view_port_alloc();
    view_port_draw_callback_set(app->vp, draw_callback, app);
    view_port_input_callback_set(app->vp, input_callback, app);
    gui_add_view_port(app->gui, app->vp, GuiLayerFullscreen);

    app->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    furi_hal_uart_init(FuriHalUartIdUSART1, UART_BAUD);

    while(app->running) {
        InputEvent evt;
        if(furi_message_queue_get(app->input_queue, &evt, 10) == FuriStatusOk) {
            if(evt.type == InputTypePress && evt.key == InputKeyBack) {
                app->running = false;
            } else if(evt.type == InputTypeShort && evt.key == InputKeyOk) {
                save_point(app);
            }
        }
        view_port_update(app->vp);
        furi_delay_ms(50);
    }

    furi_hal_uart_deinit(FuriHalUartIdUSART1);
    gui_remove_view_port(app->gui, app->vp);
    view_port_free(app->vp);
    furi_message_queue_free(app->input_queue);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}
