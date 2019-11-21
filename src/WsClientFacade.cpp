#include <ArduinoJson.h>
#include <WiFi.h>

#include "utils.hpp"
#include "WsClientFacade.hpp"


static const uint16_t MAX_ATTEMPS_CONNECTIONS = 50;


using namespace utils;
using namespace nsld;


typedef StaticJsonDocument<1000> JsonBufferType;


WsClientFacade::WsClientFacade(const String& wssid, const String& wpswd, const String& addr, uint16_t port, const String &path) 
    : _addr(addr) 
    , _port(port) 
    , _path(path)
    , _is_recv(false)
    , _is_err(false) {
    #ifdef DEBUG
    Serial.println("+ WsClientFacade");
    #endif
    WiFi.begin(wssid.c_str(), wpswd.c_str());
    #ifdef DEBUG
    Serial.print("Try connect...");
    #endif
    for (uint16_t i = 0; ((i < MAX_ATTEMPS_CONNECTIONS) and (WiFi.status() not_eq WL_CONNECTED)); ++i) {
        delay(50);
    } 
    if (WiFi.status() not_eq WL_CONNECTED) {
        utils::ErrorLights::get()->error();
    } else {
        _client.reset(new WebSocketClient(false));
    }
    delay(50);
}


WsClientFacade::~WsClientFacade() {
    #ifdef DEBUG
    Serial.println("- WsClientFacade");
    #endif
}


void WsClientFacade::recvState(const String &srecv) {
    _is_recv = false;
    JsonBufferType jbuf;
    _is_err = deserializeJson(jbuf, srecv.c_str());
    #ifdef DEBUG
    Serial.println("MSG: \"" + srecv + "\"");              
    #endif
    if (not _is_err) {
        String status = jbuf["status"].as<char*>();
        if (status == "ok") {
            utils::Blink<utils::BLUE_PIN>::get()->on();
            #ifdef DEBUG
            Serial.println("Recv OK");              
            #endif
            _is_recv = true;
            delay(50);
            utils::Blink<utils::BLUE_PIN>::get()->off();
        } else if (status == "cfg") {
            JsonObject jcfg = jbuf["cfg"];
            String uid = jcfg["uid"].as<char*>();
            if (uid) {
                Nvs::get()->setUid(uid);
            }
            String url = jcfg["url"].as<char*>();
            if (url) {
                Nvs::get()->setServiceUrl(url);
            }
            String ssid = jcfg["ssid"].as<char*>();
            if (ssid) {
                Nvs::get()->setWifiSsid(ssid);
            }
            String pswd = jcfg["pswd"].as<char*>();
            if (pswd) {
                Nvs::get()->setWifiPswd(pswd);
            }
            String token = jcfg["token"].as<char*>();
            if (token) {
                Nvs::get()->setToken(token);
            }
            time_t timeout = jcfg["timeout"].as<time_t>();
            if (timeout) {
                Nvs::get()->setSendTimeout(timeout);
            }
            utils::utc_time = jcfg["time"].as<time_t>();
            _is_recv = true;
        } else {
            _is_err = true;
        }
    } 
    if (_is_err) {
        #ifdef DEBUG
        Serial.println("Recv ERROR");              
        #endif
        utils::ErrorLights::get()->error();
    }
}


 
void WsClientFacade::getConfig() {
    if (_client) {
        if (_client->connect(_addr, _path, _port)) {
            #ifdef DEBUG
            Serial.println("Connected to \"" + _addr + ":" +  String(_port, DEC) + _path + "\"");
            #endif
            utils::Blink<utils::BLUE_PIN>::get()->on();
            JsonBufferType js;
            JsonObject jcmd = js.createNestedObject("cmd");
            jcmd["name"] = "get_config";
            String jstr;
            serializeJson(js, jstr);
            _client->send(jstr);
            delay(10);
            while (not _is_err and not _is_recv) {
                #ifdef DEBUG
                Serial.print(".");
                #endif
                String msg;
                if (_client->getMessage(msg)) {
                    #ifdef DEBUG
                    Serial.println(".");
                    #endif
                    utils::Blink<utils::BLUE_PIN>::get()->off();
                    recvState(msg);
                }
            }
            utils::Blink<utils::BLUE_PIN>::get()->off();
        } else {
            #ifdef DEBUG
            Serial.println("Can`t connect to \"" + _addr + ":" +  String(_port, DEC) + _path + "\"");
            #endif
            utils::ErrorLights::get()->error();
            _is_err = true;
        }
    }
}


void WsClientFacade::send(const String& json) {
    if (_client) {
        if (_client->connect(_addr, _path, _port)) {
            #ifdef DEBUG
            Serial.println("Connected to \"" + _addr + ":" +  String(_port, DEC) + _path + "\"");
            #endif
            utils::Blink<utils::BLUE_PIN>::get()->on();
            _client->send(json);
            delay(10);
            #ifdef DEBUG
            Serial.println("Wait recv");
            #endif
            while (not _is_err and not _is_recv) {
                #ifdef DEBUG
                Serial.print(".");
                #endif
                String msg;
                if (_client->getMessage(msg)) {
                    #ifdef DEBUG
                    Serial.println(".");
                    #endif
                    utils::Blink<utils::BLUE_PIN>::get()->off();
                    recvState(msg);
                }
            }
            utils::Blink<utils::BLUE_PIN>::get()->off();
        } else  {
            #ifdef DEBUG
            Serial.println("Can`t connect to \"" + _addr + ":" +  String(_port, DEC) + _path + "\"");
            #endif
            utils::ErrorLights::get()->error();
            _is_err = true;
        }
    }
}
