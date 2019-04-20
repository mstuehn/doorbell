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

    while( 1 )
    {
        mqtt.loop();
    }

    return 0;
}
