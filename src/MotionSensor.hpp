#pragma once 

#include <Arduino.h>


namespace nsld {

static const uint32_t MOTION_TASK_STACK_SIZE_IN_WORDS = 10000;
static void* MOTION_TASK_PARAM = nullptr;
static const int MOTION_TASK_PRIORITY = -1;
static TaskHandle_t* MOTION_TASK_HANDLE = nullptr;
 
class Nasladdin;

class  MotionSensor {
    Nasladdin* _nsld;
    int _out;

public:
    MotionSensor(Nasladdin* parent, int out_pin);
    virtual ~MotionSensor();

    void update();
};
}