#include <Arduino.h>

const int dcVoltagePin = A0;
const int priorityKnobPin = A1;
const int kettlePin = 13;

int dcVoltage;
int priority;
int threshold;
int time;
int previousVoltage;

void setup() {
  pinMode(dcVoltagePin, INPUT);
  pinMode(priorityKnobPin, INPUT);
  pinMode(kettlePin, OUTPUT);
}

void loop() {
  priority = analogRead(priorityKnobPin);
  threshold = map(priority, 0, 1023, 150, 250);
  time = map(priority, 0, 1023, 300, 5);

  dcVoltage = analogRead(dcVoltagePin);

  // Check if the voltage is below the threshold and if the previous voltage was above the threshold.
  if (dcVoltage < threshold && previousVoltage >= threshold) {
    // Delay for 1 second before switching to AC.
    delay(1000);
    digitalWrite(kettlePin, LOW);
  }

  // Check if the voltage is above the threshold and if the previous voltage was below the threshold.
  else if (dcVoltage >= threshold && previousVoltage < threshold) {
    // Delay for 1 second before switching to DC.
    delay(1000);
    digitalWrite(kettlePin, HIGH);
  } else {
    // If the voltage is still below the threshold, but the previous voltage was above the threshold,
    // then keep the kettle on DC for a certain time.
    if (dcVoltage < threshold && previousVoltage >= threshold) {
      delay(time);
    }
  }

  previousVoltage = dcVoltage;
}
