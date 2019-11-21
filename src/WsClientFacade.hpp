#pragma once 

#include <Arduino.h>
#include <WebSocketClient.h> 




namespace nsld {

typedef std::unique_ptr<WebSocketClient> PWsClient;

class  WsClientFacade;
typedef std::unique_ptr<WsClientFacade> PWsClientFacade;


class  WsClientFacade {
public:
    struct Event {
        int result;
    };

private:
    PWsClient _client;
    String _addr;       
    uint16_t _port;   
    String _path;   
    bool _is_recv;
    bool _is_err;


public:
    WsClientFacade(const String& wssid, const String& wpswd, const String& addr, uint16_t port, const String &path);
    virtual ~WsClientFacade();

    void recvState(const String &srecv);

    void send(const String& json);
    void getConfig();
};
}