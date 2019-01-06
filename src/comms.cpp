#include "comms.h"
#include <unistd.h>

#include <iostream>


using namespace mosqpp;

mqtt_con::mqtt_con(const char *id, bool clean_session) : mosquittopp(id, clean_session), m_ReconnectCnt(0)
{

}

mqtt_con::~mqtt_con()
{

}

void mqtt_con::on_connect(int rc) {
    std::cout << "on_connect" << std::endl;
    return;
}

void mqtt_con::on_disconnect(int rc) {
    std::cout << "on_disconnect: rc " << rc << std::endl;
    m_ReconnectCnt++;
    sleep(1);
    reconnect();
    return;
}

void mqtt_con::on_publish(int mid) {
    std::cout << "on_publish" << std::endl;
    return;
}

void mqtt_con::on_message(const struct mosquitto_message * message) {
    std::cout << "Topic: " << message->topic
              << " Payload: " << std::string( (const char*)message->payload, message->payloadlen )
              << std::endl;
}

void mqtt_con::on_subscribe(int mid, int qos_count, const int * granted_qos) {
    std::cout << "on_subscribe" << std::endl;
    return;
}

void mqtt_con::on_unsubscribe(int mid) {
    std::cout << "on_unsubscribe" << std::endl;
    return;
}

void mqtt_con::on_log(int level, const char * str) {
    //std::cout << "on_log: level " << level << " str: " << str<< std::endl;
    return;
}

void mqtt_con::on_error() {
    std::cout << "on_error" << std::endl;
    return;
}

Json::Value mqtt_con::print_stats() {

    Json::Value root;
    root["reconnects"] = m_ReconnectCnt;

    return root;
}
