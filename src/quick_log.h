#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct App App;

// Démarre l'UI "Mode Rapide" (carrousel preset/variant, OK=log immédiat)
void quicklog_start(App* app);
