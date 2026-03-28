#include <Arduino.h>
#include <Adafruit_SSD1306.h>

// --- Configuration ---
#define MAX_VOLTAGE 350.0   // Maximum voltage we expect to measure (scaled)
#define THRESHOLD_MIN 150   // Minimum threshold for priority knob (Volts)
#define THRESHOLD_MAX 250   // Maximum threshold for priority knob (Volts)

// Hysteresis configuration: How long to wait before switching
#define HYSTERESIS_MS 1000

// Priority timing configuration
#define PRIORITY_WAIT_MIN_MS 5000     // 5 seconds at high priority
#define PRIORITY_WAIT_MAX_MS 300000   // 5 minutes at low priority

const int dcVoltagePin = A0;
const int priorityKnobPin = A1;
const int kettlePin = 13;

// Voltage divider resistors
const float R1 = 100000.0; // 100k
const float R2 = 1000.0;   // 1k

// --- State Variables ---
float currentVoltage = 0;
int currentThreshold = 200;
unsigned long waitTimeLimit = 5000;
unsigned long lastSwitchTime = 0;
unsigned long lastAboveThresholdTime = 0;
bool kettleOnDC = false;

Adafruit_SSD1306 display(128, 32, &Wire, -1);

void setup() {
  pinMode(dcVoltagePin, INPUT);
  pinMode(priorityKnobPin, INPUT);
  pinMode(kettlePin, OUTPUT);
  digitalWrite(kettlePin, LOW); // Start with AC (OFF)

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

float readVoltage() {
  int raw = analogRead(dcVoltagePin);
  // Formula for voltage divider: Vout = Vin * R2 / (R1 + R2)
  // Vin = Vout * (R1 + R2) / R2
  // Vout = raw * (5.0 / 1023.0)
  float vOut = raw * (5.0 / 1023.0);
  float vIn = vOut * (R1 + R2) / R2;
  return vIn;
}

void updateDisplay() {
  display.clearDisplay();

  // Draw gauge border
  display.drawRect(0, 22, 128, 10, WHITE);

  // Fill gauge based on voltage
  int fillWidth = map(constrain(currentVoltage, 0, MAX_VOLTAGE), 0, MAX_VOLTAGE, 0, 126);
  display.fillRect(1, 23, fillWidth, 8, WHITE);

  // Draw threshold marker
  int thresholdX = map(currentThreshold, 0, MAX_VOLTAGE, 0, 126);
  display.drawLine(thresholdX + 1, 20, thresholdX + 1, 32, WHITE);

  // Status text
  display.setCursor(0, 0);
  display.print("V: "); display.print(currentVoltage, 1);
  display.print(" T: "); display.print(currentThreshold);

  display.setCursor(0, 10);
  if (kettleOnDC) {
    display.print("POWER: SOLAR (DC)");
  } else {
    display.print("POWER: GRID (AC)");
  }

  display.display();
}

void loop() {
  // 1. Read Inputs
  int priorityRaw = analogRead(priorityKnobPin);
  currentThreshold = map(priorityRaw, 0, 1023, THRESHOLD_MIN, THRESHOLD_MAX);
  waitTimeLimit = map(priorityRaw, 0, 1023, PRIORITY_WAIT_MAX_MS, PRIORITY_WAIT_MIN_MS);

  currentVoltage = readVoltage();
  unsigned long now = millis();

  // 2. Logic
  if (currentVoltage >= currentThreshold) {
    lastAboveThresholdTime = now;

    // If we're on AC and voltage has been good for HYSTERESIS_MS, switch to DC
    if (!kettleOnDC && (now - lastSwitchTime >= HYSTERESIS_MS)) {
        kettleOnDC = true;
        digitalWrite(kettlePin, HIGH);
        lastSwitchTime = now;
    }
  } else {
    // Voltage is below threshold
    // If we're on DC and voltage has been low for more than waitTimeLimit, switch to AC
    if (kettleOnDC && (now - lastAboveThresholdTime >= waitTimeLimit)) {
        kettleOnDC = false;
        digitalWrite(kettlePin, LOW);
        lastSwitchTime = now;
    }
  }

  // 3. Update UI
  static unsigned long lastDisplayUpdate = 0;
  if (now - lastDisplayUpdate >= 200) { // Update display at 5Hz
    updateDisplay();
    lastDisplayUpdate = now;
  }
}
