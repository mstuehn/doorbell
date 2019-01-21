#pragma once

#include <json/json.h>

int spawn_sound_handler( pru_t pru, int8_t irq, Json::Value config );
int stop_sound_handler();
