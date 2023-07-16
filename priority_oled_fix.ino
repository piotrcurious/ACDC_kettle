#include <Arduino.h>
#include <Adafruit_SSD1306.h>

#define MAX_VOLTAGE 350
#define THRESHOLD_MIN 150
#define THRESHOLD_MAX 250
#define HYSTERESIS_TIME 1000 // The time to wait before switching to DC or AC after the voltage crosses the threshold.
#define MAX_ON_TIME 300 // The maximum time to keep the kettle on DC after the voltage crosses the threshold.
#define MAX_OFF_TIME 1000 // The maximum time to keep the kettle on AC after the voltage drops below the threshold.

const int dcVoltagePin = A0;
const int priorityKnobPin = A1;
const int kettlePin = 13;

int dcVoltage;
int priority;
int threshold;
int time;
int previousVoltage;
int resistor1 = 10000;
int resistor2 = 1000;

Adafruit_SSD1306 display(128, 32, SSD1306_SWITCHCAPVCC, 0x3C);

void setup() {
  pinMode(dcVoltagePin, INPUT);
  pinMode(priorityKnobPin, INPUT);
  pinMode(kettlePin, OUTPUT);

  display.begin();
  display.clearDisplay();
}

int readVoltage() {
  int rawVoltage = analogRead(dcVoltagePin);
  // Scale the voltage using the voltage divider.
  float voltage = (rawVoltage * resistor2) / (resistor1 + resistor2);
  // Constrain the voltage to the range 0-MAX_VOLTAGE.
  voltage = constrain(voltage, 0, MAX_VOLTAGE);
  return voltage;
}

void drawGauge(int voltage) {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 32, WHITE);
  int gaugeValue = map(voltage, 0, MAX_VOLTAGE, 0, 128);
  display.fillRect(0, 0, gaugeValue, 32, BLACK);
  display.drawLine(0, 16, 128, 16, WHITE);
  // Draw the threshold marker.
  int thresholdValue = map(threshold, 0, MAX_VOLTAGE, 0, 128);
  display.drawLine(thresholdValue, 0, thresholdValue, 32, RED);
  display.setCursor(64, 16);
  display.print(voltage);
  display.display();
}

void loop() {
  priority = analogRead(priorityKnobPin);
  threshold = map(priority, 0, 1023, THRESHOLD_MIN, THRESHOLD_MAX);
  time = map(priority, 0, 1023, MAX_ON_TIME, MAX_OFF_TIME);

  dcVoltage = readVoltage();

  // Check if the voltage is below the threshold and if the previous voltage was above the threshold.
  if (dcVoltage < threshold && previousVoltage >= threshold) {
    // Switch to AC.
    delay(HYSTERESIS_TIME);
    digitalWrite(kettlePin, LOW);
  }

  // Check if the voltage is above the threshold and if the previous voltage was below the threshold.
  else if (dcVoltage >= threshold && previousVoltage < threshold) {
    // Switch to DC.
    delay(HYSTERESIS_TIME);
    digitalWrite(kettlePin, HIGH);
  } else {
    // If the voltage is still below the threshold, but the previous voltage was above the threshold,
    // then keep the kettle on DC for a certain time.
    if (dcVoltage < threshold && previousVoltage >= threshold) {
      delay(time);
    }
  }

  previousVoltage = dcVoltage;

  drawGauge(dcVoltage);
}
