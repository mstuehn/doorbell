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

#include <json/json.h>
#if !defined(__amd64__) && !defined(__i386__)
#include <libgpio.h>
#include <thread>
#endif

#include "doorbell.h"
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

    MQTT mqtt(root["mqtt-configuration"]);
    auto base_topic = root["base_topic"].asString();

    mqtt.add_callback( base_topic+"/foo", [](uint8_t*msg, size_t len){ printf("%s\n", msg); } );

    DoorBell bell(root["sound-configuration"]);

    mqtt.add_callback( base_topic+"/cmd/ring", [&bell](uint8_t*msg, size_t len){ bell.ring(); } );

#if !defined(__amd64__) && !defined(__i386__)

    std::thread gpiopoll([&bell, device, &keeprunning, pin](){
            struct timespec poll = { .tv_sec = 0; .tv_nsec = 5000000; /*5ms*/ };
            int old = 0;

            gpio_handle_t handle = gpio_open_device(device);
            if( handle == GPIO_INVALID_HANDLE) err(1, "gpio_open failed");

            while(run) {
                nanosleep(&poll, NULL);
                if( gpio_pin_get( handle, pin) == 1 && old == 0 ){
                    bell.ring();
                    old = 1;
                } else old = 0;
            }

            gpio_close( handle );

            });

#else
#warning "GPIO only works on !amd64/!i386
#endif

    while(run) mqtt.loop();

    printf("Exit program\n");

#if !defined(__amd64__) && !defined(__i386__)
    gpiopoll.join();
#endif
    return 0;
}
