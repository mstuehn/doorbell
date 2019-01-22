#pragma once
#include <json/json.h>

int setup_mqtt( Json::Value config_root );
int loop_mqtt();
int stop_mqtt();
