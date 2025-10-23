#include <AccelStepper.h>

// Define stepper pins
#define STEP_PIN 0      // Step pin
#define DIR_PIN 2       // Direction pin

// Microstepping control pins
#define MS1_PIN 16
#define MS2_PIN 4

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 2;
int oneDegree;

// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

int moveToDegree(int degree) {

  //return degree / oneDegree;
  stepper.moveTo((int)floor(degree / oneDegree));
}

void setup() {
  // Set microstepping pins as outputs
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);

  // Set microstepping mode (adjust as needed: HIGH or LOW)
  digitalWrite(MS1_PIN, HIGH);  // Set to LOW or HIGH for desired microstep setting
  digitalWrite(MS2_PIN, LOW);  // Set to LOW or HIGH for desired microstep setting

  // Set the desired RPM and the max RPM
  float desiredRPM = 200; // Set the desired speed in rpm (revolutions per minute)
  float MaxRPM = 400; // Set max speed in rpm (revolutions per minute) wyprowadzono metodą eksperementalną

  // Calculate and set the desired and max speed in steps per second
  float speedStepsPerSec = (microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
  float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper.setSpeed(speedStepsPerSec);

  oneDegree = (int)floor(stepsPerRevolution / 360);
  //stepper.moveTo(stepsPerRevolution); // ma obrócic się na 360
  //delay(1000);
  moveToDegree(30);
  //stepper.moveTo(1000);

}

void loop() {
  // Run the motor at constant speed
  
  //stepper.runSpeed();

}