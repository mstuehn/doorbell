
#include <mosquittopp.h>
#include <string>

#include <json/json.h>

class mqtt_con: public mosqpp::mosquittopp
{
public:
    mqtt_con(const char *id=NULL, bool clean_session=true);
    virtual ~mqtt_con();

    virtual void on_connect(int /*rc*/);
    virtual void on_disconnect(int /*rc*/);
    virtual void on_publish(int /*mid*/);
    virtual void on_message(const struct mosquitto_message * /*message*/);
    virtual void on_subscribe(int /*mid*/, int /*qos_count*/, const int * /*granted_qos*/);
    virtual void on_unsubscribe(int /*mid*/);
    virtual void on_log(int /*level*/, const char * /*str*/);
    virtual void on_error();

    Json::Value print_stats();

private:
    uint32_t m_ReconnectCnt;

};
