#ifndef ADAFRUIT_SSD1306_H
#define ADAFRUIT_SSD1306_H

#include <iostream>

#define SSD1306_SWITCHCAPVCC 0x02
#define WIRE 0
int Wire;

class Adafruit_SSD1306 {
public:
    Adafruit_SSD1306(int w, int h, int* wire, int reset) {}
    bool begin(int type, int addr) { return true; }
    void clearDisplay() {}
    void display() {}
    void drawRect(int x, int y, int w, int h, int color) {}
    void fillRect(int x, int y, int w, int h, int color) {}
    void drawLine(int x1, int y1, int x2, int y2, int color) {}
    void setCursor(int x, int y) {}
    void print(int i) {}
    void print(float f, int p = 2) {}
    void print(const char* s) {}
    void setTextColor(int color) {}
    void setTextSize(int size) {}
};

#endif
