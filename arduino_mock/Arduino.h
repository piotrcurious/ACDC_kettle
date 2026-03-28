#ifndef ARDUINO_H
#define ARDUINO_H

#include <iostream>
#include <stdint.h>
#include <map>

#define A0 0
#define A1 1
#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0
#define WHITE 1
#define BLACK 0
#define RED 2

void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
int analogRead(int pin);
void delay(unsigned long ms);
unsigned long millis();
int map(long x, long in_min, long in_max, long out_min, long out_max);
int constrain(int x, int a, int b);

class SerialMock {
public:
    void begin(int baud) {}
    void print(const char* s) { std::cout << s; }
    void print(int i) { std::cout << i; }
    void println(const char* s) { std::cout << s << std::endl; }
    void println(int i) { std::cout << i << std::endl; }
};

extern SerialMock Serial;

#endif
