#pragma once
#include <json/json.h>
#include <libpru.h>

int setup_mqtt( pru_t pru, int8_t irq, Json::Value config );
int loop_mqtt();
int stop_mqtt();
