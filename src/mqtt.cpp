#include <pthread.h>
#include <fcntl.h>
#include <libpru.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <mosquitto.h>

#include <string>

#include "mqtt.h"

static struct mosquitto* mqtt;

int setup_mqtt( Json::Value config_root )
{
    mosquitto_lib_init();

}

int loop_mqtt()
{

}

int stop_mqtt()
{
    //pthread_kill( sound_handler, SIGINT );

    mosquitto_lib_cleanup();
}
