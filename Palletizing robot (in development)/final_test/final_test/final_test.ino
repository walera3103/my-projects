#include <AccelStepper.h>


// Define stepper pins
#define STEP_PIN_MOTOR_1 16      // Step pin
#define DIR_PIN_MOTOR_1 4       // Direction pin

#define STEP_PIN_MOTOR_2 2      // Step pin
#define DIR_PIN_MOTOR_2 15       // Direction pin

#define STEP_PIN_MOTOR_3 7      // Step pin
#define DIR_PIN_MOTOR_3 6       // Direction pin

// Microstepping control pins
// #define MS1_PIN_MOTOR_1 16
// #define MS2_PIN_MOTOR_1 4

// #define MS1_PIN_MOTOR_2 18
// #define MS2_PIN_MOTOR_2 19

// #define MS1_PIN_MOTOR_3 23
// #define MS2_PIN_MOTOR_3 21

// Steps per revolution for the motor
const float stepsPerRevolution = 200;
// Microstepping multiplier (1, 2, 4, 8, 16, or 32)
int microstepSetting = 2;
float desiredRPM = 300; // Set the desired speed in rpm (revolutions per minute)
float MaxRPM = 400; // Set max speed in rpm (revolutions per minute) wyprowadzono metodą eksperementalną

float speedStepsPerSec = (microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
float accelerationSpeed = (microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
// AccelStepper instance in driver mode

String actualCommand = "";
String receivedCommand = "";
String temp = "";

AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_MOTOR_1, DIR_PIN_MOTOR_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_MOTOR_2, DIR_PIN_MOTOR_2);
AccelStepper stepper_3(AccelStepper::DRIVER, STEP_PIN_MOTOR_3, DIR_PIN_MOTOR_3);

void setup() {
  // Set microstepping pins as outputs
  Serial.begin(9600);
  pinMode(MS1_PIN_MOTOR_1, OUTPUT);
  pinMode(MS2_PIN_MOTOR_1, OUTPUT);

  pinMode(MS1_PIN_MOTOR_2, OUTPUT);
  pinMode(MS2_PIN_MOTOR_2, OUTPUT);

  pinMode(MS1_PIN_MOTOR_3, OUTPUT);
  pinMode(MS2_PIN_MOTOR_3, OUTPUT);

  // Set microstepping mode (adjust as needed: HIGH or LOW)
  digitalWrite(MS1_PIN_MOTOR_1, HIGH);  // Set to LOW or HIGH for desired microstep setting
  digitalWrite(MS2_PIN_MOTOR_1, LOW);  // Set to LOW or HIGH for desired microstep setting

  digitalWrite(MS1_PIN_MOTOR_2, HIGH);  // Set to LOW or HIGH for desired microstep setting
  digitalWrite(MS2_PIN_MOTOR_2, LOW);  // Set to LOW or HIGH for desired microstep setting

  digitalWrite(MS1_PIN_MOTOR_3, HIGH);  // Set to LOW or HIGH for desired microstep setting
  digitalWrite(MS2_PIN_MOTOR_3, LOW);  // Set to LOW or HIGH for desired microstep setting

  // Set the desired RPM and the max RPM
  // float desiredRPM = 50; // Set the desired speed in rpm (revolutions per minute)
  // float MaxRPM = 400; // Set max speed in rpm (revolutions per minute) wyprowadzono metodą eksperementalną

  // Calculate and set the desired and max speed in steps per second
  // float speedStepsPerSec = (microstepSetting * stepsPerRevolution*desiredRPM) / 60.0;
  // float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60; // Specify max speed in steps/sec (converted from RPM)
  stepper_1.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper_1.setSpeed(speedStepsPerSec);

  stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper_2.setSpeed(speedStepsPerSec);

  stepper_3.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper_3.setSpeed(speedStepsPerSec);
}

void loop() {
  // Run the motor at constant speed
  if (Serial.available()) {
      temp = Serial.readStringUntil('\n');
      temp.trim();
      if (temp == "motor 1 +" || temp == "motor 1 -" || temp == "motor 2 +" || temp == "motor 2 -" || temp == "motor 3 +" || temp == "motor 3 -"|| temp == "0") {
        receivedCommand = temp;
        if(actualCommand != receivedCommand) {
          if (receivedCommand == "motor 1 +") {
            speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            stepper_1.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_1.setAcceleration(accelerationSpeed);
            stepper_1.setSpeed(speedStepsPerSec);
            //actualCommand = receivedCommand;
            //delay(10);
          }

          else if (receivedCommand == "motor 1 -" && actualCommand != receivedCommand) {
            speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            stepper_1.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_1.setAcceleration(accelerationSpeed);
            stepper_1.setSpeed(speedStepsPerSec);
            //actualCommand = receivedCommand;
            //delay(10);
          }

           else if (receivedCommand == "motor 2 +" && actualCommand != receivedCommand) {
            // speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            // Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            // stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
            // stepper_2.setAcceleration(accelerationSpeed);
            // //stepper_2.run();
            // stepper_2.setSpeed(speedStepsPerSec);
            //stepper_2.moveTo(-5000);
            //stepper_2.run();
            //actualCommand = receivedCommand;
            //delay(10);


            // stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
            // stepper_2.setAcceleration(accelerationSpeed);
            // stepper_2.move(100000);  // duży dystans do przodu


            // stepper_2.setAcceleration(accelerationSpeed);
            // stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
            // stepper_2.moveTo(stepper_1.currentPosition() + 1000000);  // lub -1000000 dla przeciwnych obrotów
            // stepper_2.run();


            stepper_2.setAcceleration(accelerationSpeed);
            stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_2.moveTo(stepper_1.currentPosition() + 1000000); // nowy cel
          }

          else if (receivedCommand == "motor 2 -" && actualCommand != receivedCommand) {
            speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            stepper_2.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_2.setAcceleration(accelerationSpeed);
            stepper_2.setSpeed(speedStepsPerSec);
            //actualCommand = receivedCommand;
            //delay(10);
          }

           else if (receivedCommand == "motor 3 +" && actualCommand != receivedCommand) {
            speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            stepper_3.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_3.setAcceleration(accelerationSpeed);
            stepper_3.setSpeed(speedStepsPerSec);
            //actualCommand = receivedCommand;
            //delay(10);
          }

          else if (receivedCommand == "motor 3 -" && actualCommand != receivedCommand) {
            speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
            Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
            stepper_3.setMaxSpeed(Max_Speed_StepsPerSec);
            stepper_3.setAcceleration(accelerationSpeed);
            stepper_3.setSpeed(speedStepsPerSec);
            //actualCommand = receivedCommand;
            //delay(10);
          }

          else if (receivedCommand == "0" && actualCommand != receivedCommand) {
            stepper_1.setSpeed(0);
            stepper_2.setSpeed(0);
            //stepper_2.stop();  // zatrzyma z hamowaniem (jeśli run() jest używane)
            stepper_3.setSpeed(0);
            //actualCommand = receivedCommand;
            //delay(10);
          }
          //receivedCommand = "";
          actualCommand = receivedCommand;
        }
      }
  }

  stepper_1.runSpeed();
  //stepper_2.runSpeed();
  stepper_2.run();
  stepper_3.runSpeed();
}
