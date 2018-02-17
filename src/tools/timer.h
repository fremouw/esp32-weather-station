#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifndef TIMER_H_
#define TIMER_H_

class Timer {

public:
  static hw_timer_t * CreateTimer(void (*fn)(void), const uint8_t timerIndex = 0, uint64_t interruptAt = 10000000, bool autoReload = false, uint16_t divider = 80, bool countUp = true, bool edge = true) {
    //  Use 1st timer of 4 (counted from zero).
    //  Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    //  info).
    hw_timer_t * timer = timerBegin(timerIndex, divider, countUp);

    // Attach onTimer function to our timer.
    timerAttachInterrupt(timer, fn, edge);

    // Set alarm to call onTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    // 3600000000
    // 10000000
    timerAlarmWrite(timer, interruptAt, autoReload);

    // 
    timerStart(timer);

    // Start an alarm
    timerAlarmEnable(timer);

    return timer;
  }
};

#endif // TIMER_H_
