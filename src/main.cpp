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

    if( !parsingOk )
    {
        std::cerr << "Error during reading " << config_filename << " : " << errs << std::endl;
        exit( 2 );
    }

    MQTT mqtt( root["mqtt-configuration"] );
    auto base_topic = root["base_topic"].asString();

    DoorBell bell( root["sound-configuration"] );

    mqtt.add_callback( base_topic+"/cmd/ring", [&bell](uint8_t*msg, size_t len){
            printf("Ring due to mqtt-message\n");
            bell.ring();
            } );

            std::thread evdevpoll( [&bell, &mqtt, &base_topic, &root ](){

            uint32_t vendor_number = std::stol(root["input"]["vendor"].asString(), nullptr, 0);
            uint32_t product_number = std::stol(root["input"]["product"].asString(), nullptr, 0);

            std::string filename;
            do {
                auto result = scan_devices(vendor_number, product_number);
                if( result.first ) {
                    filename = result.second;
                    break;
                }
                else {
                    std::cerr << "Scanning devices not successfull: " << std::endl;
                    std::cerr << result.second << std::endl;
                }
            } while( true );

            std::cout << "Opening " << filename << std::endl;

            int fd;
            if( (fd = open(filename.c_str(), O_RDONLY) ) < 0) {
                perror("");
                if (errno == EACCES && getuid() != 0) {
                    fprintf(stderr, "You do not have access to %s. Try "
                            "running as root instead.\n", filename.c_str());
                    }
                    return;
            }

            while( 1 ) {
                uint16_t code, value;
                if( get_events( fd, EV_KEY, &code, &value ) && value == 1 )
                {
                    switch( code ) {
                        case KEY_F24:
                        {
                            bell.ring();
                            Json::StreamWriterBuilder wr;
                            Json::Value info;
                            info["date"] = now();
                            info["doorbell"] = true;
                            std::string msg = Json::writeString(wr, info);
                            mqtt.publish(base_topic+"/ringed", msg.c_str(), msg.length(), 0 );
                        } break;

                        case KEY_F23:
                        {
                            using std::chrono::system_clock;
                            std::time_t now = system_clock::to_time_t( system_clock::now() );
                            struct std::tm *pnow = std::localtime(&now);
                            std::cout << std::put_time( pnow, "[%F %T] " ) << "Lifesign" << std::endl;
                        }break;

                    }
            }
        }
    });

    while(1) mqtt.loop();

    printf("Exit program\n");

    evdevpoll.join();
    return 0;
}
