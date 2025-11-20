#include <AccelStepper.h>

// Піни для моторів
#define STEP_PIN_1 16
#define STEP_PIN_2 0
#define STEP_PIN_3 15
#define STEP_PIN_4 12
#define STEP_PIN_5 27

#define DIR_PIN_1 4
#define DIR_PIN_2 2
#define DIR_PIN_3 13
#define DIR_PIN_4 14
#define DIR_PIN_5 26

#define EN_PIN_1 17

// === MOTOR SETTINGS ===
const int stepsPerRev = 200;
const int microsteps = 1;
const float rpm = 300;
const int numMotors = 5;

float speedStepsPerSec = (stepsPerRev * microsteps * rpm) / 60.0;

// === STEPPER INSTANCE ===
AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);
AccelStepper stepper_3(AccelStepper::DRIVER, STEP_PIN_3, DIR_PIN_3);
AccelStepper stepper_4(AccelStepper::DRIVER, STEP_PIN_4, DIR_PIN_4);
AccelStepper stepper_5(AccelStepper::DRIVER, STEP_PIN_5, DIR_PIN_5);

AccelStepper* steppers[] = { 
  &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 
};

String cmd = "";

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("=== 5-MOTOR ACCELSTEPPER CONTROL ===");
  
  // Ініціалізація Enable піна тільки для мотора 1
  pinMode(EN_PIN_1, OUTPUT);
  digitalWrite(EN_PIN_1, LOW);
  Serial.println("Motor 1 ENABLED");
  
  // Ініціалізація всіх моторів
  for (int i = 0; i < numMotors; i++) {
    steppers[i]->setMaxSpeed(speedStepsPerSec);
    steppers[i]->setSpeed(0);
    
    // Serial.print("Motor ");
    // Serial.print(i + 1);
    // Serial.print(": STEP=");
    // Serial.print(steppers[i]->getPin());
    // Serial.print(", DIR=");
    // Serial.println(steppers[i]->getPin1());
  }
  
  Serial.print("Speed: ");
  Serial.print(speedStepsPerSec);
  Serial.println(" steps/sec");
  Serial.println("Ready - motors stop automatically when buttons released");
}

void loop() {
  // ==== RECEIVE COMMAND ====
  if (Serial.available()) {
    cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    // Автоматична зупинка при відпусканні кнопки (порожня команда)
    if (cmd == "") {
      for (int i = 0; i < numMotors; i++) {
        steppers[i]->setSpeed(0);
      }
    }
    else {
      Serial.print("CMD: '");
      Serial.print(cmd);
      Serial.println("'");

      // Обробка команд для кожного мотора
      if (cmd == "motor 1 +") {
        steppers[0]->setSpeed(speedStepsPerSec);
      }
      else if (cmd == "motor 1 -") {
        steppers[0]->setSpeed(-speedStepsPerSec);
      }
      else if (cmd == "motor 2 +") {
        steppers[1]->setSpeed(speedStepsPerSec);
      }
      else if (cmd == "motor 2 -") {
        steppers[1]->setSpeed(-speedStepsPerSec);
      }
      else if (cmd == "motor 3 +") {
        steppers[2]->setSpeed(speedStepsPerSec);
      }
      else if (cmd == "motor 3 -") {
        steppers[2]->setSpeed(-speedStepsPerSec);
      }
      else if (cmd == "motor 4 +") {
        steppers[3]->setSpeed(speedStepsPerSec);
      }
      else if (cmd == "motor 4 -") {
        steppers[3]->setSpeed(-speedStepsPerSec);
      }
      else if (cmd == "motor 5 +") {
        steppers[4]->setSpeed(speedStepsPerSec);
      }
      else if (cmd == "motor 5 -") {
        steppers[4]->setSpeed(-speedStepsPerSec);
      }
    }
  }

  // ==== RUN ALL MOTORS ====
  for (int i = 0; i < numMotors; i++) {
    steppers[i]->runSpeed();
  }
  
  //delay(1);
}