/**
 * \brief Класс реализующий режим конфигурирования. 
 */ 

#pragma once

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "utils.hpp"


namespace nsld {

static const int CONFIGURE_PIN = 15;


class ConfigureWebServer;
typedef std::unique_ptr<ConfigureWebServer> PConfigureWebServer;

typedef std::unique_ptr<AsyncWebServer> PAsyncWebServer;
typedef utils::WifiConfig WifiConfig;
typedef utils::DeviceConfig DeviceConfig;


class ConfigureWebServer {
public:
    struct Event {
        int result;
    };

private:
    static bool _is_complete;
    static bool _is_timeout;

    int _end_count;
    String _srv_ssid;
    String _srv_pswd;
    PAsyncWebServer _server;
    time_t _start_time;

    static DeviceConfig _dev_conf;

public:
    static PConfigureWebServer& getPtr(const String& srv_ssid = "nasladdin_distance", const String& srv_pswd = "password");

    static void handleNotFound(AsyncWebServerRequest *request);
    static void handleRoot(AsyncWebServerRequest *request);
    static void handleSave(AsyncWebServerRequest *request);
    static void handleCancel(AsyncWebServerRequest *request);
    static void handleSettings(AsyncWebServerRequest *request);

    static bool parseSettings(const String &js);

    ConfigureWebServer(const String& srv_ssid = "nasladdin_distance", const String& srv_pswd = "password");
    ~ConfigureWebServer();

    bool update();
};
}