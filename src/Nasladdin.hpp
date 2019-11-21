#pragma once 

#include <memory>
#include <queue>

#include <Arduino.h>

#include "ConfigureWebServer.hpp"
#include "DistanceSensor.hpp"
#include "WsClientFacade.hpp"


namespace nsld {

static const uint32_t NASLADDIN_TASK_STACK_SIZE_IN_WORDS = 10000;
static void* NASLADDIN_TASK_PARAM = nullptr;
static const uint32_t NASLADDIN_TASK_PRIORITY = 0;
static TaskHandle_t* NASLADDIN_TASK_HANDLE = nullptr;

static const int NASLADDIN_TASK_DEFAULT_SLEEP = 50;
static const int EVENTS_BUFFER_TO_SEND = 50;
static const int SEND_EVENTS_TIMEOUT = 60 * 60 * 1000;


typedef std::unique_ptr<ConfigureWebServer> PConfigureWebServer;
typedef std::unique_ptr<DistanceSensor> PDistanceSensor;
typedef std::unique_ptr<WsClientFacade> PWsClientFacade;
class Nasladdin;
typedef std::unique_ptr<Nasladdin> PNasladdin;
typedef std::queue<DistanceSensor::Event> DistEventArr;


class Nasladdin {
    enum class State {
        READING_EVENTS,
        CONFIGURING,
        SENDING_TO_SERVER
    };
    PConfigureWebServer _conf_server;
    PDistanceSensor _distance;
    PWsClientFacade _ws_client;
    bool _is_configure;
    State _state;
    uint32_t _snd_timeout;
    uint32_t _last_snd_time;
    size_t _pos;
    size_t _loaded;
    DistEventArr _devs;

public:
    int _sleep_time;

    static PNasladdin& getPtr(const DistanceSensor::Pins& pins = DistanceSensor::Pins());

    Nasladdin(const DistanceSensor::Pins& pins);
    virtual ~Nasladdin();

    void newEvent(const nsld::DistanceSensor::Event& event);
    void update();

    PConfigureWebServer& getConfigureWebServer();
    PDistanceSensor& getDistanceSensor();
    PWsClientFacade& getWsClientFacade();
};
}