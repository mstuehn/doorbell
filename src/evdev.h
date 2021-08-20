#pragma once

#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>

#include <string>
#include <utility>

std::pair<bool, std::string> scan_devices(uint16_t, uint16_t);
bool get_events(int fd, uint16_t type, uint16_t* code, uint16_t* value);
