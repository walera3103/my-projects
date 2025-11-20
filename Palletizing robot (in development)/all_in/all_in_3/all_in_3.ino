#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>

// Піни для 5 моторів
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

AS5600 encoder;
AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);
AccelStepper stepper_3(AccelStepper::DRIVER, STEP_PIN_3, DIR_PIN_3);
AccelStepper stepper_4(AccelStepper::DRIVER, STEP_PIN_4, DIR_PIN_4);
AccelStepper stepper_5(AccelStepper::DRIVER, STEP_PIN_5, DIR_PIN_5);

const float stepsPerRevolution = 200;
int microstepSetting = 1;
float desiredRPM = 300;
float MaxRPM = 300;

float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = (microstepSetting * stepsPerRevolution * MaxRPM) / 60.0;

// Початкові значення цілі для 5 моторів
int target_1_point = 20000;
int target_2_point = 20000;
int target_3_point = 20000;
int target_4_point = 20000;
int target_5_point = 20000;
const int range = 500;
const int hysteresis_range = 800;

long actual_reducer_1_point = 30000;
long actual_reducer_2_point = 0;
long actual_reducer_3_point = 10000;
long actual_reducer_4_point = 15000;
long actual_reducer_5_point = 25000;
int previous_encoder_1_point = 0;
int previous_encoder_2_point = 0;
int previous_encoder_3_point = 0;
int previous_encoder_4_point = 0;
int previous_encoder_5_point = 0;
int actual_encoder_1_point = 0;
int actual_encoder_2_point = 0;
int actual_encoder_3_point = 0;
int actual_encoder_4_point = 0;
int actual_encoder_5_point = 0;

AccelStepper* steppers[] = { &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 };
int actual_encoder_points[] = { actual_encoder_1_point, actual_encoder_2_point, actual_encoder_3_point, actual_encoder_4_point, actual_encoder_5_point };
int previous_encoder_points[] = { previous_encoder_1_point, previous_encoder_2_point, previous_encoder_3_point, previous_encoder_4_point, previous_encoder_5_point };
long actual_reducer_points[] = { actual_reducer_1_point, actual_reducer_2_point, actual_reducer_3_point, actual_reducer_4_point, actual_reducer_5_point };
int target_points[] = { target_1_point, target_2_point, target_3_point, target_4_point, target_5_point };
bool motor_moving[5] = { false, false, false, false, false };
int motor_direction[5] = { 0, 0, 0, 0, 0 };

unsigned long lastEncoderRead = 0;
unsigned long lastControlUpdate = 0;
int delta;

const long reducer_max_value = 40960;

String serialBuffer = "";
bool newData = false;

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(1);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  encoder.begin();

  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

  for (int i = 0; i < 5; i++) {
    steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
    steppers[i]->setSpeed(0);
  }
  
  Serial.println("System initialized - 5 Motors Control");
}

void parseSerialData() {
  if (newData) {
    if (serialBuffer.startsWith("motor")) {
      int motorNumber = serialBuffer.charAt(5) - '1';
      if (motorNumber >= 0 && motorNumber < 5) {
        int spaceIndex = serialBuffer.indexOf(' ');
        if (spaceIndex != -1) {
          long newTarget = serialBuffer.substring(spaceIndex + 1).toInt();
          newTarget = constrain(newTarget, 0, reducer_max_value);
          target_points[motorNumber] = newTarget;
          
          Serial.print("Motor ");
          Serial.print(motorNumber + 1);
          Serial.print(" target set to: ");
          Serial.println(newTarget);
        }
      }
    }
    serialBuffer = "";
    newData = false;
  }
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      newData = true;
      parseSerialData();
    } else {
      serialBuffer += c;
    }
  }

  unsigned long now = millis();

  // --- Читання енкодерів ---
  if (now - lastEncoderRead >= 10) {
    for (int i = 0; i < 5; i++) {
      PCA9548A(i);
      actual_encoder_points[i] = encoder.rawAngle();

      delta = actual_encoder_points[i] - previous_encoder_points[i];
      if (delta > 2048) delta -= 4096;
      else if (delta < -2048) delta += 4096;

      if (abs(delta) < 2) delta = 0;

      actual_reducer_points[i] += delta;

      // Обмеження позиції
      if (actual_reducer_points[i] > reducer_max_value) {
        actual_reducer_points[i] = reducer_max_value;
      } else if (actual_reducer_points[i] < 0) {
        actual_reducer_points[i] = 0;
      }

      previous_encoder_points[i] = actual_encoder_points[i];
    }
    lastEncoderRead = now;
  }

  // --- Просте стабільне керування ---
  if (now - lastControlUpdate >= 20) {
    for (int i = 0; i < 5; i++) {
      int error = target_points[i] - actual_reducer_points[i];
      int abs_error = abs(error);
      
      // Перевірка меж
      bool at_min_limit = (actual_reducer_points[i] <= 0 && error < 0);
      bool at_max_limit = (actual_reducer_points[i] >= reducer_max_value && error > 0);
      
      if (at_min_limit || at_max_limit) {
        steppers[i]->setSpeed(0);
        motor_moving[i] = false;
        motor_direction[i] = 0;
        if (now % 5000 < 20 && motor_moving[i]) {
          Serial.print("Motor ");
          Serial.print(i + 1);
          Serial.println(" at limit - STOPPED");
        }
        continue;
      }
      
      // Логіка керування
      if (motor_moving[i]) {
        // Мотор рухається - перевіряємо чи потрібно зупинитись
        if (abs_error <= range) {
          steppers[i]->setSpeed(0);
          motor_moving[i] = false;
          motor_direction[i] = 0;
          if (now % 1000 < 20) {
            Serial.print("Motor ");
            Serial.print(i + 1);
            Serial.println(" reached target - STOPPED");
          }
        }
      } else {
        // Мотор стоїть - перевіряємо чи потрібно запуститись
        if (abs_error > hysteresis_range) {
          // Визначаємо напрямок
          int new_direction = (error > 0) ? 1 : -1;
          
          // Запускаємо мотор
          float currentSpeed = (new_direction > 0) ? speedStepsPerSec : -speedStepsPerSec;
          steppers[i]->setSpeed(currentSpeed);
          motor_moving[i] = true;
          motor_direction[i] = new_direction;
          
          if (now % 1000 < 20) {
            Serial.print("Motor ");
            Serial.print(i + 1);
            Serial.print(" started moving ");
            Serial.println((new_direction > 0) ? "FORWARD" : "BACKWARD");
          }
        }
      }
    }
    lastControlUpdate = now;
  }

  // --- Виконання кроків ---
  for (int i = 0; i < 5; i++) {
    steppers[i]->runSpeed();
  }

  // --- Відладка ---
  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 1000) {
    for (int i = 0; i < 5; i++) {
      int current_error = target_points[i] - actual_reducer_points[i];
      float currentSpeed = steppers[i]->speed();
      
      Serial.print("Motor ");
      Serial.print(i + 1);
      Serial.print(" | Target: ");
      Serial.print(target_points[i]);
      Serial.print(" | Actual: ");
      Serial.print(actual_reducer_points[i]);
      Serial.print(" | Error: ");
      Serial.print(abs(current_error));
      Serial.print(" | Speed: ");
      Serial.print(currentSpeed);
      Serial.print(" | State: ");
      
      if (motor_moving[i]) {
        Serial.println(motor_direction[i] > 0 ? "MOVING FORWARD" : "MOVING BACKWARD");
      } else {
        Serial.println("STOPPED");
      }
    }
    Serial.println("========================================");
    lastDebug = now;
  }
}