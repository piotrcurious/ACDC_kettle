/**
 * @file priority_oled_fix.ino
 * @brief Solar Kettle Power Switcher
 *
 * This device automatically switches a kettle from the AC grid to a DC solar source
 * when sufficient solar power is available. It features an OLED display for
 * monitoring, a priority knob to adjust sensitivity, and several safety features.
 */

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// --- Configuration Constants ---
#define MAX_VOLTAGE 350.0       // Maximum voltage for display scale (Volts)
#define VOLTAGE_WARN_OVER 330.0   // Alert threshold for high solar voltage
#define VOLTAGE_FAULT_LOW 20.0    // Threshold below which sensor fault is assumed
#define THRESHOLD_MIN 150       // Minimum threshold for priority knob (Volts)
#define THRESHOLD_MAX 250       // Maximum threshold for priority knob (Volts)

#define HYSTERESIS_MS 2000      // Time to wait (stabilize) before switching from AC to DC
#define PRIORITY_WAIT_MIN_MS 5000     // Minimum DC-to-AC delay (High priority)
#define PRIORITY_WAIT_MAX_MS 300000   // Maximum DC-to-AC delay (Low priority)

// Pin Definitions
const int dcVoltagePin = A0;    // Voltage divider output
const int priorityKnobPin = A1; // Priority potentiometer
const int kettlePin = 13;      // Relay control (HIGH for DC/Solar, LOW for AC/Grid)

// Hardware Model (Voltage Divider)
const float R1 = 100000.0;      // 100k
const float R2 = 1000.0;        // 1k

// --- System State and Variables ---
enum SystemState {
  STATE_GRID,         // Operating on AC power
  STATE_STABILIZING,    // Solar power detected, waiting for stability before switching
  STATE_SOLAR         // Operating on DC solar power
};

SystemState currentState = STATE_GRID;

// Voltage Filtering (Moving Average)
#define FILTER_SIZE 10
float voltageSamples[FILTER_SIZE];
int sampleIndex = 0;
float filteredVoltage = 0;

float currentVoltage = 0;
int currentThreshold = 200;
unsigned long waitTimeLimit = 5000;
unsigned long lastSwitchTime = 0;
unsigned long lastAboveThresholdTime = 0;
bool kettleOnDC = false;
bool sensorFault = false;
bool overVoltage = false;

Adafruit_SSD1306 display(128, 32, &Wire, -1);

float readVoltage(); // Function prototype

void setup() {
  pinMode(dcVoltagePin, INPUT);
  pinMode(priorityKnobPin, INPUT);
  pinMode(kettlePin, OUTPUT);
  digitalWrite(kettlePin, LOW); // Start on AC (safety first)

  // Initialize filter with the first valid reading
  float initialV = readVoltage();
  for (int i = 0; i < FILTER_SIZE; i++) voltageSamples[i] = initialV;
  filteredVoltage = initialV;

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

/**
 * Reads ADC and converts to real voltage using the divider formula.
 * Also maintains a moving average filter.
 */
float readVoltage() {
  int raw = analogRead(dcVoltagePin);
  float vOut = raw * (5.0 / 1023.0);
  float vIn = vOut * (R1 + R2) / R2;

  voltageSamples[sampleIndex] = vIn;
  sampleIndex = (sampleIndex + 1) % FILTER_SIZE;

  float sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) sum += voltageSamples[i];
  filteredVoltage = sum / FILTER_SIZE;

  return vIn;
}

/**
 * Updates the OLED with system status, modes, and warnings.
 */
void updateDisplay() {
  display.clearDisplay();

  if (sensorFault) {
    display.setCursor(0, 0);
    display.print("! SENSOR FAULT !");
    display.setCursor(0, 12);
    display.print("Check Connections");
    display.display();
    return;
  }

  display.setCursor(0, 0);
  display.print("V:"); display.print(currentVoltage, 0);
  display.print(" T:"); display.print(kettleOnDC ? (int)(currentThreshold * 0.8) : currentThreshold);
  if (overVoltage) display.print(" !OV!");

  display.setCursor(0, 10);
  switch (currentState) {
    case STATE_GRID:
      display.print("MODE: GRID (AC)");
      break;
    case STATE_STABILIZING:
      display.print("MODE: STABILIZING...");
      display.print((int)((HYSTERESIS_MS - (millis() - lastSwitchTime)) / 100));
      break;
    case STATE_SOLAR:
      display.print("MODE: SOLAR (DC)");
      if (filteredVoltage < (currentThreshold * 0.8)) {
          int remaining = (int)((waitTimeLimit - (millis() - lastAboveThresholdTime)) / 1000);
          display.print(" ("); display.print(remaining); display.print("s)");
      }
      break;
  }

  display.drawRect(0, 22, 128, 10, WHITE);
  int fillWidth = map(constrain(currentVoltage, 0, MAX_VOLTAGE), 0, MAX_VOLTAGE, 0, 126);
  display.fillRect(1, 23, fillWidth, 8, WHITE);
  int thresholdX = map(currentThreshold, 0, MAX_VOLTAGE, 0, 126);
  display.drawLine(thresholdX + 1, 20, thresholdX + 1, 32, WHITE);

  display.display();
}

void loop() {
  // 1. Inputs and fault detection
  int priorityRaw = analogRead(priorityKnobPin);
  currentThreshold = map(priorityRaw, 0, 1023, THRESHOLD_MIN, THRESHOLD_MAX);
  waitTimeLimit = map(priorityRaw, 0, 1023, PRIORITY_WAIT_MAX_MS, PRIORITY_WAIT_MIN_MS);

  currentVoltage = readVoltage();
  unsigned long now = millis();

  sensorFault = (filteredVoltage < VOLTAGE_FAULT_LOW);
  overVoltage = (filteredVoltage > VOLTAGE_WARN_OVER);

  // 2. State Machine Logic
  if (sensorFault) {
      if (kettleOnDC) {
          kettleOnDC = false;
          digitalWrite(kettlePin, LOW);
          lastSwitchTime = now;
      }
      currentState = STATE_GRID;
  } else {
      // Use adaptive threshold when active to account for voltage drop under load.
      int effectiveThreshold = kettleOnDC ? (currentThreshold * 0.8) : currentThreshold;

      if (filteredVoltage >= effectiveThreshold) {
          lastAboveThresholdTime = now;

          if (!kettleOnDC) {
              if (currentState != STATE_STABILIZING) {
                  currentState = STATE_STABILIZING;
                  lastSwitchTime = now;
              } else if (now - lastSwitchTime >= HYSTERESIS_MS) {
                  kettleOnDC = true;
                  digitalWrite(kettlePin, HIGH);
                  lastSwitchTime = now;
                  currentState = STATE_SOLAR;
              }
          }
      } else {
          // Below threshold: handle switching back to AC
          if (kettleOnDC && (now - lastAboveThresholdTime >= waitTimeLimit)) {
              kettleOnDC = false;
              digitalWrite(kettlePin, LOW);
              lastSwitchTime = now;
              currentState = STATE_GRID;
          } else if (!kettleOnDC) {
              currentState = STATE_GRID;
          }
      }
  }

  // 3. UI Update (Non-blocking)
  static unsigned long lastDisplayUpdate = 0;
  if (now - lastDisplayUpdate >= 200) {
    currentVoltage = filteredVoltage; // Use smoothed voltage for UI
    updateDisplay();
    lastDisplayUpdate = now;
  }
}
