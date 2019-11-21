#pragma once 

#include <Arduino.h>
#include <time.h>

#include <memory>

#include "NewPing.h"

#include "utils.hpp"

namespace nsld {
    
static const int DISTANCE_PIN_ECHO    = 36;
static const int DISTANCE_PIN_TRIGGER = 39;

static const uint32_t DISTANCE_TASK_STACK_SIZE_IN_WORDS = 10000;
static void* DISTANCE_TASK_PARAM = nullptr;
static const int DISTANCE_TASK_PRIORITY = -2;
static TaskHandle_t* DISTANCE_TASK_HANDLE = nullptr;

static const int DISTANCE_TASK_DEFAULT_SLEEP = 50;
static const int ZERO_COUNT_EVENT_END = 40;

//static const int SMOOTH_ARRAY = ; //500 / DISTANCE_TASK_DEFAULT_SLEEP;
static const int MAX_DISTANCE = 3000;
static const int DISTANCE_ARRAY_SIZE = 30;


typedef std::unique_ptr<NewPing> PNewPing;

class  DistanceSensor {
public:
    struct Pins {
        int trigger;
        int echo;
    };

    struct Event {
        time_t beg_time;
        time_t end_time;
        uint32_t dists[DISTANCE_ARRAY_SIZE];

        void operator= (const Event& e); 
    };

private:
    Pins _pins;
    PNewPing _sonar;
    bool _is_event;
    size_t _end_count;
    size_t _pos;
    uint32_t _mm;
    Event _event;

public:
    int _sleep_time;

    static void printEvent(const DistanceSensor::Event& e);

    DistanceSensor(const DistanceSensor::Pins& pins);
    virtual ~DistanceSensor();

    bool update();
    const DistanceSensor::Event& getEvent();
};
}