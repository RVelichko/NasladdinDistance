#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

#include "utils.hpp"
#include "Nasladdin.hpp"

using namespace nsld;
using namespace utils;


typedef StaticJsonDocument<5000> JsonBufferType;


PNasladdin& Nasladdin::getPtr(const DistanceSensor::Pins& pins) {
    static PNasladdin nsld;
    if (not nsld) {
        nsld.reset(new Nasladdin(pins));
    }
    return nsld;
}


Nasladdin::Nasladdin(const DistanceSensor::Pins& pins) 
    : _is_configure(false)
    , _sleep_time(NASLADDIN_TASK_DEFAULT_SLEEP) 
    , _state(State::READING_EVENTS) 
    , _last_snd_time(0)
    , _pos(0) 
    , _loaded(0) {
    #ifdef DEBUG
    Serial.println("Nasladdin");
    #endif
    pinMode(CONFIGURE_PIN, INPUT);
    _distance.reset(new DistanceSensor(pins));
    _snd_timeout  = Nvs::get()->getSendTimeout() * 1000;
}


Nasladdin::~Nasladdin() {
    #ifdef DEBUG
    Serial.println("~Nasladdin");
    #endif
}


void Nasladdin::newEvent(const nsld::DistanceSensor::Event& e) {
    if (EVENTS_BUFFER_TO_SEND <= _devs.size()) {
        _devs.pop();
    }
    _devs.push(e);
}


void Nasladdin::update() {
    switch(_state) {
        case State::READING_EVENTS: {
            uint32_t mls = (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
            if (_devs.size() and _last_snd_time + _snd_timeout < mls) {
                #ifdef DEBUG
                Serial.println("State::SENDING_TO_SERVER ");
                #endif
                _state = State::SENDING_TO_SERVER;
                auto wssid = Nvs::get()->getWifiSsid();
                auto wpswd = Nvs::get()->getWifiPswd();
                utils::Url url(Nvs::get()->getServiceUrl());
                _ws_client.reset(new WsClientFacade(wssid, wpswd, url.host, url.port, url.path));
            } else {
                int val = digitalRead(CONFIGURE_PIN);
                if (val == LOW) {
                    _is_configure = false;
                } else if (not _is_configure) {
                    _is_configure = true;
                } else {
                    #ifdef DEBUG
                    Serial.println("State::CONFIGURING");
                    #endif
                    _is_configure = false;
                    _state = State::CONFIGURING;
                    _conf_server.reset(new ConfigureWebServer());
                }
            }
        } break;
        case State::CONFIGURING:
            if (not (_conf_server and _conf_server->update())) {
                _conf_server.reset();
                #ifdef DEBUG
                Serial.println("State::READING_EVENTS");
                #endif
                _state = State::READING_EVENTS;
            }
            break;
        case State::SENDING_TO_SERVER:
            if (_ws_client) {
                JsonBufferType js;
                js["uid"] = Nvs::get()->getUid();
                js["url"] = Nvs::get()->getServiceUrl();
                js["ssid"] = Nvs::get()->getWifiSsid();
                js["pswd"] = Nvs::get()->getWifiPswd();
                js["timeout"] = Nvs::get()->getSendTimeout();
                js["token"] = Nvs::get()->getToken();
                JsonArray jevs = js.createNestedArray("events"); 
                while (not _devs.empty()) {
                    DistanceSensor::Event e = _devs.front();
                    //DistanceSensor::printEvent(e);
                    JsonObject je = jevs.createNestedObject();
                    je["tb"] = e.beg_time + utils::utc_time;
                    je["te"] = e.end_time + utils::utc_time;
                    JsonArray jdists = je.createNestedArray("d");
                    for (auto& d : e.dists) {
                        if (d) {
                            jdists.add(d);
                        }
                    }
                    _devs.pop();
                }
                String jstr;
                serializeJson(js, jstr);
                #ifdef DEBUG
                Serial.println("Send: " + jstr);
                #endif
                _ws_client->send(jstr);
                #ifdef DEBUG
                Serial.println("Sending complette.");
                #endif
                _ws_client.reset();
            }
            _last_snd_time = (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
            _state = State::READING_EVENTS;
            break;
    }
}


PConfigureWebServer& Nasladdin::getConfigureWebServer() {
    return _conf_server;
}


PDistanceSensor& Nasladdin::getDistanceSensor() {
    return _distance;
}


PWsClientFacade& Nasladdin::getWsClientFacade() {
    return _ws_client;
}
