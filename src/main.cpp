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

#include "doorbell.h"
#include "mqtt.h"

#include <json/json.h>

#include <libevdev/libevdev.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <ctime>

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
    printf("usage: %s [-d evdev-device] [-c config-file]\n", getprogname());
    exit(1);
}

int main( int argc, char* argv[] )
{
    Json::Value root;
    Json::Reader rd;
    std::string config_filename = "/usr/local/etc/doorbell/config.json";
    std::string config_devicename = "/dev/input/event7";

    signed char ch;
    while ((ch = getopt(argc, argv, "d:c:h")) != -1) {
        switch (ch) {
        case 'c':
            config_filename = std::string(optarg);
            break;
        case 'd':
            config_devicename = std::string(optarg);
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

    DoorBell bell(root["sound-configuration"]);

    mqtt.add_callback( base_topic+"/cmd/ring", [&bell](uint8_t*msg, size_t len){ bell.ring(); } );

    std::thread evdevpoll([&bell, &config_devicename, &mqtt, &base_topic](){
            struct libevdev *dev = NULL;
            int fd;
            int rc = 1;

            fd = open(config_devicename.c_str(), O_RDONLY);
            rc = libevdev_new_from_fd(fd, &dev);
            if (rc < 0) {
                fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
                exit(1);
            }
            printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
            printf("Input device ID: bus %#x vendor %#x product %#x\n",
                    libevdev_get_id_bustype(dev),
                    libevdev_get_id_vendor(dev),
                    libevdev_get_id_product(dev));
            do {
                struct input_event ev;
                rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
                if( rc == 0 )
                {
                    printf("Event: %s %s %d\n",
                            libevdev_event_type_get_name(ev.type),
                            libevdev_event_code_get_name(ev.type, ev.code),
                            ev.value);
                    if( ev.type == EV_KEY && ev.value == 1 ){
                        bell.ring();

                        Json::StreamWriterBuilder wr;
                        Json::Value info;
                        info["date"] = now();
                        info["doorbell"] = true;

                        std::string msg = Json::writeString(wr, info);
                        mqtt.publish(base_topic+"/ringed", msg.c_str(), msg.length(), 0 );
                    }
                }
            } while( rc == 1 || rc == 0 || rc == -EAGAIN );
    });

    while(1) mqtt.loop();

    printf("Exit program\n");

    evdevpoll.join();
    return 0;
}
