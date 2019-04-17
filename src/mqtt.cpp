#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <mosquitto.h>
#include <errno.h>
#include <libgpio.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "sound.h"
#include "mqtt.h"

static struct mosquitto* mqtt;
static int s_pin = -1;
static int s_ctrl = -1;
static bool run_thread = true;

static pthread_t gpio_poll;

static std::string pub_topic;
static std::string sub_topic;
static std::string base_topic;
static std::string host;

static std::string now() {
    auto t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream wss;
    wss << std::put_time(&tm, "%H:%M:%S %d-%m-%Y");
    return wss.str();
}

static bool pru_callback( uint64_t dummy )
{
    if( mqtt == nullptr || pub_topic == "" )
    {
        std::cerr   << "MQTT not ready: "
                    << mqtt << " "
                    << "Topic: ->"
                    << pub_topic <<
                    "<-" << std::endl;
        return false;
    }
    Json::StreamWriterBuilder wr;
    Json::Value info;

    info["date"] = now();
    info["doorbell"] = true;

    std::string msg = Json::writeString(wr, info);
    return mosquitto_publish( mqtt,
                              nullptr,
                              pub_topic.c_str(),
                              msg.length(),
                              msg.c_str(), 0, false ) == 0;
}

static int debounce( int value )
{
    return value;
}

static void* read_pru(void* dummy)
{
    /* this function blocks */
    struct timespec timeout;
    timeout.tv_nsec = 10000000;
    timeout.tv_sec = 0;

    gpio_handle_t handle = gpio_open( s_ctrl );

    if( handle == GPIO_INVALID_HANDLE ) return NULL;

    while( run_thread )
    {
        nanosleep(&timeout, NULL);
        int value = gpio_pin_get( handle, s_pin );
        if( debounce(value) ) dispatch_bell();
    }

    gpio_close( handle );
    return NULL;
}

void mqtt_msg_cb( struct mosquitto* mqtt, void* mqtt_new_data, const struct mosquitto_message* omsg )
{
    std::string topic = std::string(omsg->topic);
    if( omsg->payloadlen == 0 ) {
        std::cout << "got message on topic " << topic << std::endl;
        std::cout << "with no content" << std::endl;
        return;
    }
    std::string msg = std::string( (const char*)omsg->payload, omsg->payloadlen);

    bool match;
    std::string cmd_topic = base_topic+"/cmd/ring";
    int result = mosquitto_topic_matches_sub( cmd_topic.c_str(),
                                              topic.c_str(),
                                              &match);
    if( match ) dispatch_bell();
}

int setup_mqtt( int gpio, int pin, Json::Value config )
{
    s_ctrl = gpio;
    s_pin = pin;

    mosquitto_lib_init();
    base_topic = config["base_topic"].asString();
    pub_topic = base_topic + "/active";
    sub_topic = base_topic + "/cmd/#";

    mqtt = mosquitto_new( "nanobsd", true, nullptr );
    if( mqtt == nullptr ) {
        std::cerr << "failed to create mqtt handle" << std::endl;
        return -3;
    }

    host = config["server"].asString();
    auto port = config["port"].asInt();
    auto keepalive = config["keepalive"].asInt();
    if( mosquitto_connect( mqtt, host.c_str(), port, keepalive ) < 0 )
    {
        std::cerr << "failed to connect to mqtt" << std::endl;
        return -2;
    }

    if( mosquitto_subscribe( mqtt, NULL, sub_topic.c_str(), 0 ) < 0 )
    {
        std::cerr << "failed to subscribe mqtt to " << sub_topic << std::endl;
        return -3;
    }

    mosquitto_message_callback_set( mqtt, mqtt_msg_cb );

    if( pthread_create( &gpio_poll, NULL, read_pru, NULL ) )
    {
        std::cerr << "failed to create pru-reader pthread" << std::endl;
        perror("Failure");
        return -4;
    }

    return 0;
}

int loop_mqtt()
{
    int result = mosquitto_loop( mqtt, 1000, 1 );
    switch (result)
    {
        case 0:
            // No Error at all
            break;
        case MOSQ_ERR_NO_CONN:
        case MOSQ_ERR_CONN_LOST:
            sleep(2);
            result = mosquitto_reconnect( mqtt );
            std::cerr << "Connection lost, try to reconnect: " << result << std::endl;
            break;
        default:
            std::cerr << "Error during mqtt operation: " << result << std::endl;
            break;
    }
    return result;
}

int stop_mqtt()
{
    std::cerr << "stop mqtt-pru-irq" << std::endl;
    pthread_kill( gpio_poll, SIGINT );
    pthread_join( gpio_poll, nullptr );

    std::cerr << "cleanup mqtt lib" << std::endl;
    return mosquitto_lib_cleanup();
}
