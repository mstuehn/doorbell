#include <pthread.h>
#include <fcntl.h>
#include <libpru.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <mosquitto.h>
#include <errno.h>

#include <iostream>
#include <string>

#include "sound.h"
#include "mqtt.h"

static struct mosquitto* mqtt;
static pru_t s_pru;
static int s_irq = -1;

static pthread_t pru_irq;

static std::string pub_topic;
static std::string sub_topic;
static std::string host;

static bool pru_callback( uint64_t )
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
    info["date"] = std::string("now");
    std::string msg = Json::writeString(wr, info);
    return mosquitto_publish(   mqtt, nullptr,
                                pub_topic.c_str(),
                                pub_topic.length(),
                                msg.c_str(), 0, false ) == 0;
}

static void* read_pru(void* dummy)
{
    /* this function blocks */
    pru_wait_irq( s_pru, s_irq, pru_callback );
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
    dispatch_bell();
}

int setup_mqtt( pru_t pru, int8_t irq, Json::Value config )
{
    s_pru = pru;
    s_irq = irq;

    mosquitto_lib_init();
    pub_topic = config["base_topic"].asString() +"/occ";
    sub_topic = config["base_topic"].asString() +"/cmd/#";

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

    if( pthread_create( &pru_irq, NULL, read_pru, NULL ) )
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
    pthread_kill( pru_irq, SIGINT );
    pthread_join( pru_irq, nullptr );

    std::cerr << "cleanup mqtt lib" << std::endl;
    return mosquitto_lib_cleanup();
}
