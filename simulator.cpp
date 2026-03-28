#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <random>
#include "arduino_mock/Arduino.h"
#include "priority_oled_fix.ino"

extern int _analogValues[2];
extern int _digitalPins[14];
extern long _millis;

// --- Physical Constants ---
const float PANEL_OPEN_VOLTAGE = 320.0;
const float PANEL_INTERNAL_RESISTANCE = 10.0;
const float KETTLE_RESISTANCE = 25.0;
const float ADC_REF_V = 5.0;
const float R1_VAL = 100000.0;
const float R2_VAL = 1000.0;

std::default_random_engine generator;
std::normal_distribution<double> noise(0.0, 5.0);

float simulate_hardware(float sun_intensity, bool kettle_on) {
    if (sun_intensity < 0) return 0; // Fault case
    float voc = PANEL_OPEN_VOLTAGE * sun_intensity;
    float v_terminal = voc;
    if (kettle_on) v_terminal = voc * (KETTLE_RESISTANCE / (PANEL_INTERNAL_RESISTANCE + KETTLE_RESISTANCE));
    float v_adc = v_terminal * (R2_VAL / (R1_VAL + R2_VAL));
    int raw_adc = (int)(v_adc * 1023.0 / ADC_REF_V);
    raw_adc += (int)noise(generator);
    return (int)constrain(raw_adc, 0, 1023);
}

void reset_system() {
    _millis = 0;
    currentState = STATE_GRID;
    kettleOnDC = false;
    lastSwitchTime = 0;
    lastAboveThresholdTime = 0;
    sampleIndex = 0;
    for (int i=0; i<FILTER_SIZE; i++) voltageSamples[i] = 0;
    _digitalPins[13] = 0;
}

void test_scenario(const char* name, int priority, std::vector<float> sun_profile, int steps_per_intensity) {
    std::cout << "\n--- SCENARIO: " << name << " ---" << std::endl;
    reset_system();
    setup();

    for (float sun : sun_profile) {
        for (int i = 0; i < steps_per_intensity; ++i) {
            _analogValues[A0] = simulate_hardware(sun, _digitalPins[13]);
            _analogValues[A1] = priority;
            loop();

            if (i % 5 == 0) {
                std::cout << "T: " << std::setw(6) << _millis << " | Sun: " << (sun*100) << "% | V: " << std::setw(3) << (int)currentVoltage << "V | "
                          << "Mode: " << (currentState == STATE_SOLAR ? "SOLAR" : (currentState == STATE_STABILIZING ? "STAB" : "GRID"))
                          << " (" << (_digitalPins[13] ? "ON" : "OFF") << ")" << std::endl;
            }
            _millis += 200;
        }
    }
}

int main() {
    // 1. Nominal case: High priority, quick response to sun
    test_scenario("High Priority / Good Sun", 1023, {0.5, 0.9, 0.5}, 20);

    // 2. Fault Case: Sensor disconnection
    test_scenario("Sensor Fault", 512, {0.9, -1.0, 0.9}, 20);

    // 3. Marginal Case: Oscillating sun
    test_scenario("Marginal/Oscillating Sun", 512, {0.6, 0.7, 0.6, 0.7}, 20);

    return 0;
}
