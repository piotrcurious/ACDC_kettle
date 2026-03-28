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

// --- Physical Constants for Simulation ---
const float PANEL_OPEN_VOLTAGE = 320.0; // V (Voc)
const float PANEL_INTERNAL_RESISTANCE = 10.0; // Ohms (Simplified solar model)
const float KETTLE_RESISTANCE = 25.0; // Ohms (220V, 2000W approx)
const float ADC_REF_V = 5.0;
const float R1_VAL = 100000.0;
const float R2_VAL = 1000.0;

std::default_random_engine generator;
std::normal_distribution<double> noise(0.0, 5.0); // 5 units of ADC noise

float simulate_hardware(float sun_intensity, bool kettle_on) {
    // sun_intensity: 0.0 to 1.0
    float voc = PANEL_OPEN_VOLTAGE * sun_intensity;
    float v_terminal = voc;

    if (kettle_on) {
        // Simple voltage divider between panel internal R and Kettle R
        v_terminal = voc * (KETTLE_RESISTANCE / (PANEL_INTERNAL_RESISTANCE + KETTLE_RESISTANCE));
    }

    // Scale for ADC
    float v_adc = v_terminal * (R2_VAL / (R1_VAL + R2_VAL));
    int raw_adc = (int)(v_adc * 1023.0 / ADC_REF_V);

    // Add noise
    raw_adc += (int)noise(generator);
    return (int)constrain(raw_adc, 0, 1023);
}

void run_dynamic_simulation(float sun_intensity, int priority, int duration_ms) {
    std::cout << "\nStarting Dynamic Simulation (Sun: " << (sun_intensity * 100) << "%, Priority: " << priority << ")" << std::endl;
    _millis = 0;
    // Reset state for each run
    kettleOnDC = false;
    lastSwitchTime = 0;
    lastAboveThresholdTime = 0;
    sampleIndex = 0;
    for (int i=0; i<FILTER_SIZE; i++) voltageSamples[i] = 0;

    setup();

    for (int i = 0; i < duration_ms / 100; ++i) {
        _analogValues[A0] = simulate_hardware(sun_intensity, _digitalPins[13]);
        _analogValues[A1] = priority;
        loop();

        if (i % 10 == 0) {
            std::cout << "Time: " << std::setw(6) << _millis << "ms | "
                      << "V_DC: " << std::setw(5) << currentVoltage << "V | "
                      << "Kettle: " << (_digitalPins[13] ? "ON (SOLAR)" : "OFF (GRID)") << std::endl;
        }
        _millis += 100;
    }
}

int main() {
    // Case 1: High sun, should stay on DC even with load drop
    run_dynamic_simulation(1.0, 512, 10000);

    // Case 2: Marginal sun, might oscillate or switch to AC
    // Sun at 65% Voc = 208V. Threshold = 200V. Load Voc = 208 * 25/35 = 148V.
    // Adaptive threshold = 200 * 0.8 = 160V.
    // At 65% sun, load voltage 148V is below adaptive threshold 160V, so it should switch to AC after waitTimeLimit.
    run_dynamic_simulation(0.65, 512, 20000);

    // Case 3: Good sun but not perfect
    // Sun at 75% Voc = 240V. Threshold = 200V. Load Voc = 240 * 25/35 = 171V.
    // Adaptive threshold = 160V.
    // At 75% sun, load voltage 171V is above adaptive threshold 160V, so it should stay on DC.
    run_dynamic_simulation(0.75, 512, 10000);

    return 0;
}
