#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration pour éviter l'inclusion croisée
typedef struct App App;

// API Quick Log (implémentée dans quick_log.c)
void app_start_quick_log(App* app);
void app_stop_quick_log(App* app);

#ifdef __cplusplus
}
#endif
