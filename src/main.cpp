#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>

#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>

#include <sys/types.h>
#include <err.h>

#include <libgpio.h>

#include "sound.h"
#include "mqtt.h"

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

    signed char ch;
    while ((ch = getopt(argc, argv, "c:h")) != -1) {
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
    int pin;
    int gpio;

    if( spawn_sound_handler( gpio, pin, root["sound-configuration"] ) < 0 ) {
        printf("Error during sound init, exiting\n");
        return -3;
    }

    if( setup_mqtt( gpio, pin, root["mqtt-configuration"] ) < 0 ) {
        printf("Error during mqtt init, exiting\n");
        return -4;
    }

    for(;;)
    {
        if( !run ){
            printf("Stop Requested\n");
            break;
        }

        int result = loop_mqtt();
        if( result < 0 ) {
            printf("Error Occured\n");
            break;
        }
    }

    printf("stop sound handler\n");
    stop_sound_handler();

    printf("stop mqtt\n");
    stop_mqtt();

    return 0;
}
