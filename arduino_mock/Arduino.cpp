#include "Arduino.h"
#include <iostream>
#include <algorithm>

long _millis = 0;
int _analogValues[2] = {0, 0};
int _digitalPins[14] = {0};

void pinMode(int pin, int mode) {}
void digitalWrite(int pin, int value) {
    if (pin >= 0 && pin < 14) _digitalPins[pin] = value;
}
int analogRead(int pin) {
    if (pin >= 0 && pin < 2) return _analogValues[pin];
    return 0;
}
void delay(unsigned long ms) { _millis += ms; }
unsigned long millis() { return _millis; }
int map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
int constrain(int x, int a, int b) {
    return std::max(a, std::min(x, b));
}

SerialMock Serial;
