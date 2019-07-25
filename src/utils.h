
#pragma once
#include <pthread.h>
#include <pthread_np.h>
#include <stdio.h>

static char* name="foo";

static inline void print_guard( void ) {
    //char name[128];
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    size_t guard;
    pthread_attr_getguardsize( &attr, &guard );
    //pthread_get_name_np( pthread_self() , name, 128 );

    printf( "%s's Guardsize: %ld\n", name, guard );
}
