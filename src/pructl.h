
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t get_last();
int wait_for_events();
int trywait_for_events();
int initialize_pru( int prunum, int irq, int event );
int pru_exit();

#ifdef __cplusplus
};
#endif
