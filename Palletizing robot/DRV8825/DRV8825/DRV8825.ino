#include <AccelStepper.h>

// Define stepper pins
#define STEP_PIN 14     // Step pin
#define DIR_PIN 12      // Direction pin

// Microstepping control pins
// #define M0_PIN 13
// #define M1_PIN 12
// #define M2_PIN 14

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 1;

// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  // Set microstepping pins as outputs
  // pinMode(M0_PIN, OUTPUT);
  // pinMode(M1_PIN, OUTPUT);
  //pinMode(0, OUTPUT);

  // // Set microstepping mode (adjust as needed: HIGH or LOW)
  // digitalWrite(M0_PIN, LOW);  // Set to LOW or HIGH for desired microstep setting
  // digitalWrite(M1_PIN, LOW);  // Set to LOW or HIGH for desired microstep setting
  // digitalWrite(M2_PIN, LOW);  // Set to LOW or HIGH for desired microstep setting
  //digitalWrite(0, LOW);  // Set to LOW or HIGH for desired microstep setting

  // Set the desired RPM and the max RPM
  float desiredRPM = 200; // Set the desired speed in rpm (revolutions per minute)
  float MaxRPM = 300; // Set max speed in rpm (revolutions per minute)

  // Calculate and set the desired and max speed in steps per second
  float speedStepsPerSec = (microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
  float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper.setSpeed(speedStepsPerSec);
  //stepper.moveTo(100);
}

void loop() {
  // Run the motor at constant speed
  
  stepper.runSpeed();
  //stepper.setSpeed(0);

  // stepper.moveTo(100);
  // while (stepper.distanceToGo() != 0) {
  //   stepper.run();
  // }
  // delay(500);
}