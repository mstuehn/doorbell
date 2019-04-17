#pragma once
#include <json/json.h>

int setup_mqtt( int gpio, int pin, Json::Value config );
int loop_mqtt();
int stop_mqtt();
