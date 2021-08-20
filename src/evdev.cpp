
#include "evdev.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <linux/input.h>

#include <string>
#include <filesystem>
#include <iostream>

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

//static int
//is_event_device(const struct dirent *dir) {
//    return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
//}

static bool does_device_match( int fd, int vendor, int product )
{
    int version;
    uint16_t id[4];

    if( ioctl(fd, EVIOCGVERSION, &version) ) {
        perror("can't get version");
    } else {
        printf("Input driver version is %d.%d.%d\n",
                version >> 16, (version >> 8) & 0xff, version & 0xff);
    }

    if( ioctl(fd, EVIOCGID, id) )
    {
        perror("can't retrieve vendor/product");
        return false;
    }

    printf("Input device ID: bus 0x%x vendor 0x%x product 0x%x version 0x%x\n\n",
            id[ID_BUS], id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

    return (id[ID_VENDOR] == vendor) && (id[ID_PRODUCT] == product);
}

std::pair<bool, std::string> scan_devices( uint16_t vendor, uint16_t product )
{
    printf("Scanning for compatible Vendor/Product:  0x%04X/0x%04X\n", vendor, product );

    printf("Available devices:\n");

    std::string filename;
    for( auto const& dir: std::filesystem::directory_iterator{DEV_INPUT_EVENT} ) {
        char name[256] = "???";
        const std::string& fname = dir.path();

        int fd = open(fname.c_str(), O_RDONLY);
        if (fd < 0) continue;

        int result = ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if( result < 0 ) { close(fd); continue; }
        printf("%s:  %s\n", fname.c_str(), name);

        if( !does_device_match( fd, vendor, product )) { close(fd); continue; }

        printf("Device found: %s \n", fname.c_str() );
        return {true, fname };
    }

    return { false, "No devices match configured vendor/product" } ;
}

bool get_events(int fd, uint16_t type, uint16_t* code, uint16_t* value)
{
    struct input_event ev;
    auto size = read(fd, &ev, sizeof(struct input_event));

    if( size < (ssize_t)sizeof(struct input_event) ) {
        printf("expected %lu bytes, got %li\n", sizeof(struct input_event), size);
        perror("\nerror reading");
        return false;
    }

    //printf("Event: time %ld.%06ld, ", ev.time.tv_sec, ev.time.tv_usec);
    //printf("type: %i, code: %i, value: %i\n", ev.type, ev.code, ev.value);

    if( type != ev.type ) return false;

    *code = ev.code;
    *value = ev.value;

    return true;
}

