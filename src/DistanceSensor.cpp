#include <time.h>

#include "Nasladdin.hpp"
#include "DistanceSensor.hpp"

using namespace nsld;


void DistanceSensor::Event::operator = (const Event& e) {
    if (this not_eq &e) {
        beg_time = e.beg_time;
        end_time = e.end_time;
        size_t i;
        for (i = 0; i < sizeof(e.dists) and e.dists[i]; ++i) {
            dists[i] = e.dists[i];
        }
        if (i < sizeof(e.dists)) {
            dists[i] = 0;
        }
    }
}


void DistanceSensor::printEvent(const DistanceSensor::Event& e) {
    Serial.println("E::beg=" + String(e.beg_time, DEC)); 
    String dist_arr;
    for (auto d : e.dists) {
        if (d) {
            dist_arr += String(d, DEC) + ", ";
        }
    }
    Serial.println("E::arr: " + dist_arr); 
    Serial.println("E::end=" + String(e.end_time, DEC)); 
}


DistanceSensor::DistanceSensor(const DistanceSensor::Pins& pins) 
    : _pins(pins) 
    , _is_event(false) 
    , _end_count(0) 
    , _pos(0) 
    , _mm(0) 
    , _sleep_time(DISTANCE_TASK_DEFAULT_SLEEP) {
    _sonar.reset(new NewPing(_pins.trigger, _pins.echo, MAX_DISTANCE));
    delay(100);
    #ifdef DEBUG
    Serial.println("DistanceSensor(trigger: "  + String(_pins.trigger, DEC) + ", echo: " + String(_pins.echo, DEC) + ")");
    #endif
}


DistanceSensor::~DistanceSensor() {
    #ifdef DEBUG
    Serial.println("~DistanceSensor");
    #endif
}


bool DistanceSensor::update() {
    bool is_event = false;
    if (_sonar) {
        uint32_t mm = _sonar->ping_cm(MAX_SENSOR_DISTANCE) * 10;
        //Serial.println("###: " + String(mm, DEC));
        uint32_t mls = (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
        if (mm not_eq 0 and _mm == 0) {
            _pos = 0;
            _event.beg_time = mls;
            for(size_t i = 0; i < DISTANCE_ARRAY_SIZE; ++i) {
                _event.dists[i] = 0;
            }
            _event.end_time = 0;
        } else if (mm == 0 and _mm not_eq 0) {
            ++_end_count;
            if (_end_count == ZERO_COUNT_EVENT_END) {
                _mm = 0;
                _end_count = 0;
                _event.end_time = mls;
                is_event = true;
                //#ifdef DEBUG
                //DistanceSensor::printEvent(_event);
                //#endif
            }
        } 
        if (mm not_eq 0 and _mm not_eq mm) {
            _mm = mm;
            if (_pos + 1 < DISTANCE_ARRAY_SIZE) {
                ++_pos;
            }
            //Serial.println("$: " + String(_pos, DEC) + " : " + String(_mm, DEC));
            _event.dists[_pos] = _mm; 
        }
        return is_event;
    }
}


const DistanceSensor::Event& DistanceSensor::getEvent() {
    return _event;
}
