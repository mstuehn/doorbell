#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <chrono>

#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>

#include <dirent.h>

#include "doorbell.h"
#include "mqtt.h"

#include <json/json.h>

#include "evdev.h"
#include <thread>
#include <sstream>
#include <iomanip>
#include <ctime>

#include <iomanip>
#include <chrono>

static std::string now() {
    auto t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream wss;
    wss << std::put_time(&tm, "%H:%M:%S %d-%m-%Y");
    return wss.str();
}

static void __attribute__((noreturn))
usage(void)
{
#if defined(__FreeBSD__)
    printf("usage: %s [-d evdev-device] [-c config-file]\n", getprogname());
#else
    printf("usage: [-d evdev-device] [-c config-file]\n");
#endif
    exit(1);
}

const Json::Value get_config_node( Json::Value& root, std::string name ) {

    const Json::Value result = root[name];

    if( result.empty() ) {
        std::cerr << "Error retrieving config value for " << name << std::endl;
        exit(1);
    }

    return result;
}

timeval to_timeval( float seconds )
{
    timeval t;
    t.tv_sec = (int)seconds;
    t.tv_usec = (seconds - t.tv_sec) * 1000000;

    return t;
}

int main( int argc, char* argv[] )
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
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

    std::ifstream config( config_filename, std::ifstream::binary );
    bool parsingOk = parseFromStream( builder, config, &root, &errs );

    if( !parsingOk ) {
        std::cerr << "Error during reading " << config_filename << " : " << errs << std::endl;
        exit( 2 );
    }

    const Json::Value input_cfg = get_config_node( root, "input" );
    const Json::Value mqtt_cfg = get_config_node( root, "mqtt-configuration" );
    const Json::Value bell_cfg = get_config_node( root, "sound-configuration" );

    auto base_topic = root.get("base_topic", "house/bell/generic").asString();
    timeval throttle = to_timeval( root.get("throttle", 1.0 ).asFloat() );

    const std::string sub_topic = base_topic + "/cmd/ring";
    const std::string pub_topic = base_topic + "/rang";

    DoorBell bell( bell_cfg );

    MQTT mqtt( mqtt_cfg );
    mqtt.add_callback( sub_topic, [&bell](uint8_t*msg, size_t len){
            printf("Ring due to mqtt-message\n");
            bell.ring();
            } );

    const uint32_t vendor_number = std::stol( input_cfg.get( "vendor", "0x1b4f" ).asString(), nullptr, 0 );
    const uint32_t product_number = std::stol( input_cfg.get( "product", "0x9206" ).asString(), nullptr, 0 );
    const uint16_t evdev_code = input_cfg.get( "event", KEY_F24 ).asUInt();

    EvDevice evdev( vendor_number, product_number );
    evdev.add_callback( evdev_code, [&bell, &mqtt, pub_topic, throttle](uint16_t code, timeval when){
            static timeval last = {0, 0};

            // only rising edge
            if( code == 0 ) return;

            timeval cmpare;
            timeradd( &last, &throttle, &cmpare );

            last = when;
            if( !timercmp( &cmpare, &when, < ) ) return;

            bell.ring();

            Json::StreamWriterBuilder wr;
            Json::Value info;
            info["date"] = now();
            info["doorbell"] = true;
            std::string msg = Json::writeString( wr, info );

            mqtt.publish( pub_topic, msg.c_str(), msg.length(), 0 );
        });

    while(1) mqtt.loop();

    std::cout << "Exit program" << std::endl;

    return 0;
}
