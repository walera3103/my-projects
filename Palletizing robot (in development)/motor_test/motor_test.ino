#include <AccelStepper.h>

#define STEP_PIN 0
#define DIR_PIN 2
#define MS1_PIN 16
#define MS2_PIN 4

const float stepsPerRevolution = 200;
int microstepSetting = 2;
float desiredRPM = 300;
float MaxRPM = 400;

float speedStepsPerSec = 0;            // поточна швидкість
float targetSpeedStepsPerSec = 0;      // цільова швидкість
float accelerationStep = 50;           // крок прискорення (steps/s)
unsigned long lastAccelUpdate = 0;
const unsigned long accelInterval = 50; // оновлення швидкості кожні 50 мс

String actualCommand = "";
String receivedCommand = "";

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(9600);

  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, LOW);
  
  stepper.setMaxSpeed(microstepSetting * stepsPerRevolution * MaxRPM / 60.0);
  stepper.setSpeed(0);
}

void loop() {
  // --- UART-команди ---
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "motor 1 +" || cmd == "motor 1 -" || cmd == "0") {
      receivedCommand = cmd;
      if (receivedCommand != actualCommand) {
        if (cmd == "motor 1 +") {
          targetSpeedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
        } 
        else if (cmd == "motor 1 -") {
          targetSpeedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
        } 
        else if (cmd == "0") {
          targetSpeedStepsPerSec = 0;
          speedStepsPerSec = 0; // миттєва зупинка
          stepper.setSpeed(0);
        }
        actualCommand = receivedCommand;
      }
    }
  }

  // --- Плавне прискорення тільки при старті ---
  if (targetSpeedStepsPerSec != 0) {  // тільки якщо мотор має рухатися
    unsigned long now = millis();
    if (now - lastAccelUpdate >= accelInterval) {
      lastAccelUpdate = now;

      if (speedStepsPerSec < targetSpeedStepsPerSec) {
        speedStepsPerSec += accelerationStep;
        if (speedStepsPerSec > targetSpeedStepsPerSec) speedStepsPerSec = targetSpeedStepsPerSec;
        stepper.setSpeed(speedStepsPerSec);
      }
      else if (speedStepsPerSec > targetSpeedStepsPerSec) {
        speedStepsPerSec -= accelerationStep;
        if (speedStepsPerSec < targetSpeedStepsPerSec) speedStepsPerSec = targetSpeedStepsPerSec;
        stepper.setSpeed(speedStepsPerSec);
      }
    }
  }

  stepper.runSpeed();
}
