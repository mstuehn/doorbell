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

    DoorBell bell(root["sound-configuration"]);

    mqtt.add_callback( base_topic+"/cmd/ring", [&bell](uint8_t*msg, size_t len){ bell.ring(); } );

    std::thread evdevpoll([&bell, &mqtt, &base_topic](){
            struct libevdev *dev = NULL;
            int fd = -1;
            int rc = 1;
            DIR *eventdir;
            bool evdev_found = false;
            const char* dirname = "/dev/input";

            do {
                eventdir = opendir(dirname);
                if( eventdir ) {
                    while( 1 )
                    {
                        struct dirent* dir = readdir(eventdir);
                        if( dir == nullptr ){
                            std::cout << "Noting found, try again in 5s" << std::endl;
                            sleep(5);
                            break;
                        } else if( strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 ) {
                            char absname[32];
                            snprintf(absname, sizeof(absname), "%s/%s", dirname, dir->d_name );
                            std::cout << "Try to open " << absname << std::endl;
                            fd = open( absname, O_RDONLY);
                            rc = libevdev_new_from_fd(fd, &dev);
                            if (rc < 0) {
                                fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
                                exit(1);
                            }
                            int vendor = libevdev_get_id_vendor(dev);
                            int product = libevdev_get_id_product(dev);
                            printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
                            printf("Input device ID: bus %#x vendor %#x product %#x\n",
                                    libevdev_get_id_bustype(dev),
                                    vendor,
                                    product );
                            if( vendor == 0x1b4f && product == 0x9203 ){
                                evdev_found = true;
                                std::cout << "Found suitable input device" << std::endl;
                                break;
                            } else {
                                libevdev_free( dev );
                                close(fd);
                            }
                        }
                    }
                    closedir( eventdir );
                } else {
                            fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
                            exit(1);
                }
            } while( !evdev_found );

            do {
                struct input_event ev;
                rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev);
                if( rc == 0 )
                {
                    printf("Event: %s %s %d\n",
                            libevdev_event_type_get_name(ev.type),
                            libevdev_event_code_get_name(ev.type, ev.code),
                            ev.value);
                    if( ev.type == EV_KEY && ev.code == KEY_F24 && ev.value == 1 )
                    {
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
