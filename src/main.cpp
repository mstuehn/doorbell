#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>

#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libpru.h>

#include "sound.h"
#include "gpio_bin.h"

pru_t pru_init( uint8_t prunum, uint8_t irq, uint8_t event )
{
    auto pru = pru_alloc("ti,am335x");

    if( pru == nullptr )
    {
        fprintf( stderr, "Allocation failed\n");
        return nullptr;
    }

    if( pru_upload_buffer( pru, prunum, (const char*)pru_gpio_fw, sizeof(pru_gpio_fw) ))
    {
        fprintf( stderr, "Uploading buffer failed\n");
        return nullptr;
    }

    if( pru_register_irq( pru, irq, irq, event ) < 0 )
    {
        fprintf( stderr, "registering interrupt failed\n");
        return nullptr;
    }

    if( pru_enable( pru, 0, 0 ) )
    {
        fprintf(stderr, "Enabling pru failed\n");
        return nullptr;
    }

    return pru;
}

static void __attribute__((noreturn))
usage(void)
{
    printf("usage: %s [-c] config-file\n", getprogname());
	exit(1);
}

static bool run = true;

int main( int argc, char* argv[] )
{
    signal( SIGINT, [](int sig){
                        std::cout
                            << "CTR-C pressed"
                            << std::endl;
                        run = false;
                    });

    Json::Value root;
    Json::Reader rd;
    std::string config_filename = "/usr/local/etc/doorbell/config.json";

    char ch;
    while ((ch = getopt(argc, argv, "c:")) != -1) {
        switch (ch) {
        case 'c':
			config_filename = std::string(optarg);
            break;
        case 'h':
            usage();
        default:
            break;
        }
    }
	argc -= optind;
	argv += optind;

    std::ifstream config( config_filename, std::ifstream::binary);
    bool parsingOk = rd.parse(config, root, false);

    if( !parsingOk )
    {
        std::cerr << "Error during reading " << config_filename << std::endl;
        exit( 2 );
    }

    root["sound-configuration"];
    uint8_t prunum = 0;
    uint8_t irq = 3;
    uint8_t event = 17;

    pru_t pru = pru_init( prunum, irq, event );
    if( pru == nullptr ) {
        printf("Error during PRU init, exiting\n");
        return -2;
    }

    if( spawn_sound_handler( pru, irq, root["sound-configuration"] ) < 0 ) {
        printf("Error during sound init, exiting\n");
        return -3;
    }

    // TODO: mosquitto part
    while( run ) sleep(1);

    stop_sound_handler();
    free( pru );

    return 0;
}
