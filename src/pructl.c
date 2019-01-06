#include <libpru.h>
#include <stdlib.h>
#include <stdio.h>

#include "gpio_bin.h"

static uint64_t last;
static pru_t s_pru;
static int s_prunum = -1;

static int s_irq = -1;
static int s_event = -1;

uint64_t get_last()
{
    return last;
}

static void pru_cb( uint64_t ts )
{
    last = ts;
}

int initialize_pru( int prunum, int irq, int event )
{
    s_pru = pru_alloc("ti,am335x");

    if( s_pru == NULL )
    {
        printf("Allocation failed\n");
        return -1;
    }

    if( pru_upload_buffer( s_pru, prunum, (const char*)pru_gpio_fw, sizeof(pru_gpio_fw) ))
    {
        printf("Uploading buffer failed\n");
        return -2;
    }

    if( pru_register_irq( s_pru, irq, irq, event, pru_cb) < 0 )
    {
        printf("registering interrupt failed\n");
        return -3;
    }

    if( pru_enable( s_pru, 0, 0 ) )
    {
        printf("Enabling pru failed\n");
        return -4;
    }

    s_prunum = prunum;
    s_irq = irq;
    s_event = event;

    return 0;
}

int pru_exit()
{
    pru_disable( s_pru, s_prunum );
    pru_deregister_irq( s_pru, s_irq );
    pru_free( s_pru );
    return 0;
}
