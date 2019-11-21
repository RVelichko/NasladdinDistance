//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "Nasladdin.hpp"

typedef nsld::Nasladdin Nasladdin;
typedef nsld::PNasladdin PNasladdin;
typedef nsld::ConfigureWebServer ConfigureWebServer;
typedef nsld::PConfigureWebServer PConfigureWebServer;
typedef nsld::DistanceSensor DistanceSensor;
typedef nsld::PDistanceSensor PDistanceSensor;
typedef nsld::WsClientFacade WsClientFacade;
typedef nsld::PWsClientFacade PWsClientFacade;

static const unsigned portBASE_TYPE DISTANCE_QUEUE_SIZE = 20;
static const unsigned portBASE_TYPE DISTANCE_QUEUE_ITEM_SIZE = sizeof(time_t) * 2 + sizeof(uint32_t) * nsld::DISTANCE_ARRAY_SIZE;


xQueueHandle __distance_queue;


void NasladdinTask(void* param) {
    auto& nsld = Nasladdin::getPtr();
    if (nsld) {
        //xQueueHandle* q = reinterpret_cast<xQueueHandle*>(param); 
        while(true) {
            DistanceSensor::Event e;
            while (xQueueReceive(__distance_queue, &e, 0) == pdPASS) {
                nsld->newEvent(e);    
            };
            nsld->update();
            vTaskDelay(nsld->_sleep_time);
            taskYIELD();
        }
    }
    vTaskDelete(nullptr);
}


void DistanceTask(void* param) {
    auto& dist_sens = Nasladdin::getPtr()->getDistanceSensor();
    if (dist_sens) {
        //xQueueHandle* q = reinterpret_cast<xQueueHandle*>(param); 
        while(true) {
            bool is_event = dist_sens->update();
            if (is_event) {
                const auto& e = dist_sens->getEvent();
                portBASE_TYPE res = xQueueSendToBack(__distance_queue, &e, 0);
                //if (res == pdPASS) {
                //    #ifdef DEBUG
                //    Serial.println("Event added to queue.");
                //    #endif
                //} 
                //if (res == errQUEUE_FULL) {
                //    #ifdef DEBUG
                //    Serial.println("ERR: Can`t add event to queue.");
                //    #endif
                //}
            }
            vTaskDelay(dist_sens->_sleep_time);
            //taskYIELD();
        }
    }
    vTaskDelete(nullptr);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {
    Serial.begin(115200);
    
    #ifdef DEBUG
    Serial.println("Setup Nasladdin...");
    #endif

    auto wssid = utils::Nvs::get()->getWifiSsid();
    auto wpswd = utils::Nvs::get()->getWifiPswd();
    utils::Url url(utils::Nvs::get()->getServiceUrl());
    WsClientFacade wscf(wssid, wpswd, url.host, url.port, url.path);
    wscf.getConfig();

    Nasladdin::getPtr(DistanceSensor::Pins({nsld::DISTANCE_PIN_TRIGGER, nsld::DISTANCE_PIN_ECHO}));

    #ifdef DEBUG
    Serial.println("Create queue.");
    #endif
    __distance_queue = xQueueCreate(DISTANCE_QUEUE_SIZE, DISTANCE_QUEUE_ITEM_SIZE);

    #ifdef DEBUG
    Serial.println("Start Nasladdin task...");
    #endif
    xTaskCreate(NasladdinTask, "Nasladdin", 
                nsld::NASLADDIN_TASK_STACK_SIZE_IN_WORDS, 
                nullptr, 
                nsld::NASLADDIN_TASK_PRIORITY,
                nsld::NASLADDIN_TASK_HANDLE);

    #ifdef DEBUG
    Serial.println("Start Distance task...");
    #endif
    xTaskCreate(DistanceTask, "Distance", 
                nsld::DISTANCE_TASK_STACK_SIZE_IN_WORDS, 
                nullptr, 
                nsld::DISTANCE_TASK_PRIORITY,
                nsld::DISTANCE_TASK_HANDLE);
}


void loop() {
    while(true);
}
