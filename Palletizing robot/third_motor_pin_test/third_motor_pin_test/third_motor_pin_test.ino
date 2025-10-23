#include <AccelStepper.h>

// Define stepper pins
#define STEP_PIN 12      // Step pin
#define DIR_PIN 13       // Direction pin

// Microstepping control pins
#define MS1_PIN 23
#define MS2_PIN 21

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 2;
float speedStepsPerSec;
float Max_Speed_StepsPerSec;
float desiredRPM = 300; // Set the desired speed in rpm (revolutions per minute)
float MaxRPM = 400; // Set max speed in rpm (revolutions per minute) wyprowadzono metodą eksperementalną

String actualCommand = "";
String receivedCommand = "";
String temp = "";
// AccelStepper instance in driver mode
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  // Set microstepping pins as outputs
  Serial.begin(9600);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);

  // Set microstepping mode (adjust as needed: HIGH or LOW)
  digitalWrite(MS1_PIN, HIGH);  // Set to LOW or HIGH for desired microstep setting
  digitalWrite(MS2_PIN, LOW);  // Set to LOW or HIGH for desired microstep setting

  // Set the desired RPM and the max RPM
  // float desiredRPM = 300; // Set the desired speed in rpm (revolutions per minute)
  // float MaxRPM = 400; // Set max speed in rpm (revolutions per minute) wyprowadzono metodą eksperementalną

  // Calculate and set the desired and max speed in steps per second
  // float speedStepsPerSec = -(microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
  // float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  // stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  // stepper.setSpeed(speedStepsPerSec);
}

void loop() {
  // Odczytaj całą linię z portu szeregowego
  //delay(1000);
  if (Serial.available()) {
    temp = Serial.readStringUntil('\n');
    temp.trim();
    if (temp == "motor 1 +" || temp == "motor 1 -" || temp == "0") {
      receivedCommand = temp;
      if(actualCommand != receivedCommand) {
        if (receivedCommand == "motor 1 +") {
          speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
          Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
          stepper.setMaxSpeed(Max_Speed_StepsPerSec);
          stepper.setSpeed(speedStepsPerSec);
          //actualCommand = receivedCommand;
          //delay(10);
        }

        else if (receivedCommand == "motor 1 -" && actualCommand != receivedCommand) {
          speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
          Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
          stepper.setMaxSpeed(Max_Speed_StepsPerSec);
          stepper.setSpeed(speedStepsPerSec);
          //actualCommand = receivedCommand;
          //delay(10);
        }

        else if (receivedCommand == "0" && actualCommand != receivedCommand) {
          stepper.setSpeed(0);
          //actualCommand = receivedCommand;
          //delay(10);
        }
        //receivedCommand = "";
        actualCommand = receivedCommand;
      }
    }
}


  // if (receivedCommand == "motor 1 +" && actualCommand != receivedCommand) {
  //   speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
  //   Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
  //   stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  //   stepper.setSpeed(speedStepsPerSec);
  //   actualCommand = receivedCommand;
  //   delay(10);
  // }

  // // else if (receivedCommand == "motor 1 -" && actualCommand != receivedCommand) {
  // //   speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
  // //   Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
  // //   stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  // //   stepper.setSpeed(speedStepsPerSec);
  // //   actualCommand = receivedCommand;
  // //   delay(10);
  // // }

  // // else if (receivedCommand == "0" && actualCommand != receivedCommand) {
  // //   stepper.setSpeed(0);
  // //   actualCommand = receivedCommand;
  // //   delay(10);
  // // }
  // // receivedCommand = "";

  stepper.runSpeed();
  //delay(10);
}