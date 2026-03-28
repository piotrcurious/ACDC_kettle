#include <iostream>
#include <vector>
#include <iomanip>
#include "arduino_mock/Arduino.h"
#include "priority_oled_fix.ino"

extern int _analogValues[2];
extern int _digitalPins[14];
extern long _millis;

void reset_simulation() {
    _millis = 0;
    _digitalPins[13] = 0;
    kettleOnDC = false;
    lastSwitchTime = 0;
    lastAboveThresholdTime = 0;
}

void step_simulation(int solarVoltage, int priority) {
    _analogValues[A0] = solarVoltage;
    _analogValues[A1] = priority;
    loop();
}

void test_priority_knob() {
    std::cout << "\nTest: Priority Knob Impact" << std::endl;

    // Low priority (Raw 0) -> Long wait time (5 minutes)
    reset_simulation();
    setup();
    step_simulation(600, 0);
    _millis += 2000;
    step_simulation(600, 0); // Establishing solar power
    std::cout << "Priority LOW: Kettle status: " << (_digitalPins[13] ? "ON" : "OFF") << std::endl;
    std::cout << "Sun goes behind cloud at " << _millis << "ms..." << std::endl;
    int switchTimeLow = -1;
    for (int i = 0; i < 400; ++i) { // 400 seconds
        step_simulation(200, 0);
        if (!_digitalPins[13] && switchTimeLow == -1) {
            switchTimeLow = _millis;
            std::cout << "Kettle switched to AC (LOW priority) at " << _millis << "ms" << std::endl;
        }
        _millis += 1000;
        if (switchTimeLow != -1) break;
    }

    // High priority (Raw 1023) -> Short wait time (5 seconds)
    reset_simulation();
    setup();
    step_simulation(600, 1023);
    _millis += 2000;
    step_simulation(600, 1023); // Establishing solar power
    std::cout << "Priority HIGH: Kettle status: " << (_digitalPins[13] ? "ON" : "OFF") << std::endl;
    std::cout << "Sun goes behind cloud at " << _millis << "ms..." << std::endl;
    int switchTimeHigh = -1;
    for (int i = 0; i < 20; ++i) { // 20 seconds
        step_simulation(200, 1023);
        if (!_digitalPins[13] && switchTimeHigh == -1) {
            switchTimeHigh = _millis;
            std::cout << "Kettle switched to AC (HIGH priority) at " << _millis << "ms" << std::endl;
        }
        _millis += 1000;
        if (switchTimeHigh != -1) break;
    }
}

int main() {
    test_priority_knob();
    return 0;
}
