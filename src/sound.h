#pragma once

#include <json/json.h>

int spawn_sound_handler( int, int, Json::Value config );
int dispatch_bell();
int stop_sound_handler();
