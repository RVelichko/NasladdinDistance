
#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>

#include "utils.hpp"
#include "ConfigureWebServer.hpp"

using namespace nsld;
using namespace utils;


static const time_t CONFIGURE_TIMEOUT = 1800; ///< Количество секунд работы режима конфигурирования.

extern const uint8_t index_html_start[]        asm("_binary_src_ConfigurePage_index_html_gz_start");
extern const uint8_t index_html_end[]          asm("_binary_src_ConfigurePage_index_html_gz_end");
extern const uint8_t favicon_ico_start[]       asm("_binary_src_ConfigurePage_favicon_ico_start");
extern const uint8_t favicon_ico_end[]         asm("_binary_src_ConfigurePage_favicon_ico_end");
extern const uint8_t bootstrap_min_css_start[] asm("_binary_src_ConfigurePage_bootstrap_min_css_gz_start");
extern const uint8_t bootstrap_min_css_end[]   asm("_binary_src_ConfigurePage_bootstrap_min_css_gz_end");

static const char favicon_str[] = "/favicon.ico";
static const char bootstrap_min_css[] = "/bootstrap.min.css";


DeviceConfig ConfigureWebServer::_dev_conf;

typedef StaticJsonDocument<500> JsonBufferType;

bool ConfigureWebServer::_is_complete = false;
bool ConfigureWebServer::_is_timeout = false;


PConfigureWebServer& ConfigureWebServer::getPtr(const String& srv_ssid, const String& srv_pswd) {
    static PConfigureWebServer conf_srv;
    if (not conf_srv) {
        conf_srv.reset(new ConfigureWebServer(srv_ssid, srv_pswd));
    } 
    return conf_srv;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ConfigureWebServer::handleNotFound(AsyncWebServerRequest *request) {
    request->send(404);
}


void ConfigureWebServer::handleSave(AsyncWebServerRequest *request) {
    bool complete = false;
    #ifdef DEBUG    
    Serial.print("Handle SAVE");
    #endif
    if (request->params()) {
        auto val = request->getParam(0)->value();
        #ifdef DEBUG    
        Serial.println(": " + val);
        #endif
        complete = ConfigureWebServer::parseSettings(val);
    } else {
        #ifdef DEBUG    
        Serial.println("");
        #endif
    }
    if (complete) {
        request->send(200, "application/json", "{\"status\",\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"status\",\"err\"}");
    }
    ConfigureWebServer::_is_complete = true;
}


void ConfigureWebServer::handleCancel(AsyncWebServerRequest *request) {
    #ifdef DEBUG    
    Serial.println("Handle EXIT");
    #endif
    request->send(200, "application/json", "{\"status\",\"ok\"}");
    ConfigureWebServer::_is_complete = true;
}


void ConfigureWebServer::handleSettings(AsyncWebServerRequest *request) {
    _dev_conf.uid = Nvs::get()->getUid();
    _dev_conf.wc.ssid = Nvs::get()->getWifiSsid();
    _dev_conf.wc.pswd = Nvs::get()->getWifiPswd();
    _dev_conf.service_url = Nvs::get()->getServiceUrl();
    _dev_conf.send_timeout = Nvs::get()->getSendTimeout();
    _dev_conf.token = Nvs::get()->getToken();
    String js = "{" \
        "\"uid\":\"" + _dev_conf.uid + "\"," \
        "\"ssid\":\"" + _dev_conf.wc.ssid + "\"," \
        "\"pswd\":\"" + _dev_conf.wc.pswd  + "\"," \
        "\"url\":\"" + _dev_conf.service_url + "\"," \
        "\"timeout\":" + String(_dev_conf.send_timeout, DEC) + ","\
        "\"token\":\"" + _dev_conf.token + "\"}";
    #ifdef DEBUG    
    Serial.println("Handle settings_info: \"" + js + "\"");
    #endif
    request->send(200, "application/json", js.c_str());
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool ConfigureWebServer::parseSettings(const String &jstr) {
    #ifdef DEBUG    
    Serial.println("Parse: \"" + jstr + "\"");
    #endif
    bool ret = false;
    if (jstr.length()) {
        JsonBufferType jbuf;
        auto err = deserializeJson(jbuf, jstr.c_str());
        if (not err) {
            String uid = jbuf["uid"].as<char*>();
            if (uid.length()) {
                uid = EscapeQuotes(uid);
                #ifdef DEBUG
                Serial.println("Read UID: \"" + uid + "\"");
                #endif
                Nvs::get()->setUid(uid);
            }
            String url = jbuf["url"].as<char*>();
            if (url.length()) {
                url = EscapeQuotes(url);
                #ifdef DEBUG
                Serial.println("Read URL: \"" + url + "\"");
                #endif
                Nvs::get()->setServiceUrl(url);
            }
            String ssid = jbuf["ssid"].as<char*>();
            if (ssid.length()) {
                ssid = EscapeQuotes(ssid);
                #ifdef DEBUG    
                Serial.println("Read wifi ssid: \"" + ssid + "\"");
                #endif
                Nvs::get()->setWifiSsid(ssid);
            }
            String pswd = jbuf["pswd"].as<char*>();
            if (pswd.length()) {
                pswd = EscapeQuotes(pswd);
                #ifdef DEBUG
                Serial.println("Read wifi pswd: \"" + pswd + "\"");
                #endif
                Nvs::get()->setWifiPswd(pswd);
            }
            uint64_t timeout = jbuf["timeout"].as<uint64_t>();
            if (timeout) {
                #ifdef DEBUG
                Serial.println("Read timeout: \"" + Nvs::idToStr(timeout) + "\"");
                #endif
                Nvs::get()->setSendTimeout(timeout);
            }
            String token = jbuf["token"].as<char*>();
            if (token.length()) {
                token = EscapeQuotes(token);
                #ifdef DEBUG
                Serial.println("Read token: \"" + token + "\"");
                #endif
                Nvs::get()->setToken(token);
            }
            ret = true;
        } else {
            #ifdef DEBUG
            Serial.println("ERR: Can`t parse settings. \"" + String(err.c_str()) + "\"");
            #endif
            ErrorLights::get()->error();
        }
    }
    return ret;
}


ConfigureWebServer::ConfigureWebServer(const String& srv_ssid, const String& srv_pswd) 
    : _end_count(10)
    , _srv_ssid(srv_ssid)
    , _srv_pswd(srv_pswd)
    , _server(new AsyncWebServer(80)) {
    Blink<RED_PIN>::get()->on();
    delay(10);
    WiFi.mode(WIFI_AP);
    IPAddress local_IP(192,168,4,22);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    WiFi.softAP(_srv_ssid.c_str(), _srv_pswd.c_str(), 1, 0, 1);
    delay(10);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    delay(10);
    
    _server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html; charset=UTF-8", index_html_start, index_html_end - index_html_start - 1);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    _server->on(favicon_str, HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "image/x-icon", favicon_ico_start, favicon_ico_end - favicon_ico_start - 1); 
    });
    _server->on(bootstrap_min_css, HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", bootstrap_min_css_start, bootstrap_min_css_end - bootstrap_min_css_start - 1);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server->on("/settings", HTTP_GET, handleSettings);
    _server->on("/save", HTTP_GET, handleSave);
    _server->on("/cancel", HTTP_GET, handleCancel);
    _server->onNotFound(handleNotFound);
    _server->begin();

    _start_time = time(nullptr);
    #ifdef DEBUG
    Serial.println(String("Config server[ " + srv_ssid + "@" + srv_pswd + " ] IP: \"") + WiFi.softAPIP().toString() + String("\""));
    #endif
     
    ConfigureWebServer::_is_timeout = false;
    ConfigureWebServer::_is_complete = false;
}


ConfigureWebServer::~ConfigureWebServer() {
    Blink<RED_PIN>::get()->off();
    #ifdef DEBUG
    Serial.println("End server");
    #endif
    if(_server) {
        _server->reset();
    }
    delay(100);
    WiFi.disconnect(); // обрываем WIFI соединения
    WiFi.softAPdisconnect(); // отключаем отчку доступа(если она была
    WiFi.mode(WIFI_OFF); // отключаем WIFI
    delay(100);
    #ifdef DEBUG
    if (ConfigureWebServer::_is_timeout and not ConfigureWebServer::_is_complete) {
        Serial.println(String("Configure is timeout."));
    } else {
        Serial.println(String("Configure is complete."));
    }
    #endif

}


bool ConfigureWebServer::update() {
    auto cur_time = time(nullptr); 
    ConfigureWebServer::_is_timeout = (CONFIGURE_TIMEOUT <= (cur_time - _start_time));
    if (ConfigureWebServer::_is_complete or ConfigureWebServer::_is_timeout) {
        --_end_count;
    }
    return _end_count;
}
