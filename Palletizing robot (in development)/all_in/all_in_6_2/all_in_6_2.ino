/* ESP32 controller (AS5600 x5 + AccelStepper x5 + SD) */

#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define NUM_MOTORS 5

// pins
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

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(1);
}

AS5600 encoder;
AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);
AccelStepper stepper_3(AccelStepper::DRIVER, STEP_PIN_3, DIR_PIN_3);
AccelStepper stepper_4(AccelStepper::DRIVER, STEP_PIN_4, DIR_PIN_4);
AccelStepper stepper_5(AccelStepper::DRIVER, STEP_PIN_5, DIR_PIN_5);
AccelStepper* steppers[NUM_MOTORS] = { &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 };

const float stepsPerRevolution = 200;
int microstepSetting = 1;
float desiredRPM = 300;
float MaxRPM = 300;
float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = (microstepSetting * stepsPerRevolution * MaxRPM) / 60.0;

// --- ГЛОБАЛЬНІ ЗМІННІ З ІНІЦІАЛІЗАЦІЄЮ ---
const int reducer_max_value_default = 40950;

int actual_encoder_points[NUM_MOTORS];
int previous_encoder_points[NUM_MOTORS];
long actual_reducer_points[NUM_MOTORS]; // Початкові значення
long previous_reducer_points[NUM_MOTORS];
int target_points[NUM_MOTORS];
int previus_error[NUM_MOTORS];
bool motor_moving[NUM_MOTORS] = {false,false,false,false,false};
bool previous_motor_moving[NUM_MOTORS] = {false,false,false,false,false};
bool is_this_first_error_check[NUM_MOTORS] = {true,true,true,true,true};
int motor_direction[NUM_MOTORS] = {0,0,0,0,0};

const int range = 100;
const int hysteresis_range = 800;
int delta;

long min_limit[NUM_MOTORS];
long max_limit[NUM_MOTORS];

unsigned long lastEncoderRead = 0;
unsigned long lastControlUpdate = 0;

String serialBuffer = "";
bool newData = false;

bool is_card_here = true;
bool calibration_mode = false;
bool request_log_write = false;

bool initial_encoder_read_done = false;
//bool is_this_first_error_check = true;

// ---------------------- File helpers ----------------------
void writeFullLog() {
  if(!is_card_here) {
    Serial.println("No SD card - cannot write log");
    return;
  }

  File file = SD.open("/log.txt", FILE_WRITE);
  if(!file){
    Serial.println("Failed to open /log.txt for writing");
    return;
  }

  for (int i=0;i<NUM_MOTORS;i++){
    file.print("min limit for motor ");
    file.print(i+1);
    file.print(": ");
    file.println(min_limit[i]);

    file.print("max limit for motor ");
    file.print(i+1);
    file.print(": ");
    file.println(max_limit[i]);

    file.print("reducer position ");
    file.print(i+1);
    file.print(": ");
    file.println(actual_reducer_points[i]);
  }
  file.close();
  Serial.println("Log file overwritten (/log.txt).");

  // ВІДПРАВЛЯЄМО ДАНІ ПІСЛЯ ЗАПИСУ ЛОГУ
  for (int i=0;i<NUM_MOTORS;i++){
    Serial.print("MOTOR_DATA:");
    Serial.print(i+1);
    Serial.print(":");
    Serial.print(min_limit[i]);
    Serial.print(":");
    Serial.print(max_limit[i]);
    Serial.print(":");
    Serial.print(target_points[i]);
    Serial.print(":");
    Serial.println(actual_reducer_points[i]);
  }
}

bool loadFromLog() {
  if(!is_card_here) return false;
  if(!SD.exists("/log.txt")) return false;

  File file = SD.open("/log.txt");
  if(!file) return false;

  Serial.println("=== READING LOG FILE ===");
  bool data_loaded = false;

  while(file.available()){
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length()==0) continue;

    Serial.println("Line: " + line);

    if(line.startsWith("min limit for motor ")) {
      int idx = line.charAt(20) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        min_limit[idx] = val.toInt();
        Serial.print("LOG_MIN:");
        Serial.print(idx+1);
        Serial.print(":");
        Serial.println(min_limit[idx]);
        data_loaded = true;
      }
    } 
    else if(line.startsWith("max limit for motor ")) {
      int idx = line.charAt(20) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        max_limit[idx] = val.toInt();
        Serial.print("LOG_MAX:");
        Serial.print(idx+1);
        Serial.print(":");
        Serial.println(max_limit[idx]);
        data_loaded = true;
      }
    } 
    else if(line.startsWith("reducer position ")) {
      int idx = line.charAt(17) - '1';
      int colon = line.indexOf(':');
      if(idx>=0 && idx<NUM_MOTORS && colon>0){
        String val = line.substring(colon+1); 
        val.trim();
        actual_reducer_points[idx] = val.toInt();
        previous_reducer_points[idx] = actual_reducer_points[idx];
        target_points[idx] = actual_reducer_points[idx];
        Serial.print("LOG_POS:");
        Serial.print(idx+1);
        Serial.print(":");
        Serial.println(actual_reducer_points[idx]);
        data_loaded = true;
      }
    }
  }
  file.close();

  // ВІДПРАВЛЯЄМО ДАНІ ПІСЛЯ ЗАВАНТАЖЕННЯ ЛОГУ
  if(data_loaded) {
    Serial.println("=== SUCCESS: Log data loaded ===");
    for (int i=0;i<NUM_MOTORS;i++){
      Serial.print("MOTOR_DATA:");
      Serial.print(i+1);
      Serial.print(":");
      Serial.print(min_limit[i]);
      Serial.print(":");
      Serial.print(max_limit[i]);
      Serial.print(":");
      Serial.print(target_points[i]);
      Serial.print(":");
      Serial.println(actual_reducer_points[i]);
    }
  } else {
    Serial.println("=== WARNING: No valid data found in log ===");
  }
  
  return data_loaded;
} 

void initializeEncodersFromSavedPositions() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    PCA9548A(i);
    int current_encoder_value = encoder.rawAngle();
    previous_encoder_points[i] = current_encoder_value;
    Serial.print("Encoder ");
    Serial.print(i+1);
    Serial.print(" initialized to: ");
    Serial.println(current_encoder_value);
  }
  initial_encoder_read_done = true;
}

// ---------------------- Serial / Command handling ----------------------
void emitStatusLines() {
  for (int i=0;i<NUM_MOTORS;i++){
    Serial.print("MOTOR_STATUS:");
    Serial.print(i+1);
    Serial.print(":");
    Serial.print(target_points[i]);
    Serial.print(":");
    Serial.println(actual_reducer_points[i]);
  }
}

void parseSerialData() {
  if (!newData) return;
  String cmd = serialBuffer;
  cmd.trim();

  if (cmd.length() == 0) { serialBuffer=""; newData=false; return; }

  if (cmd.startsWith("motor")) {
    int motorNumber = cmd.charAt(5) - '1';
    if (motorNumber >=0 && motorNumber < NUM_MOTORS) {
      int spaceIndex = cmd.indexOf(' ');
      if (spaceIndex != -1) {
        long newTarget = cmd.substring(spaceIndex+1).toInt();
        if (!calibration_mode) {
          if (newTarget < min_limit[motorNumber]) newTarget = min_limit[motorNumber];
          if (newTarget > max_limit[motorNumber]) newTarget = max_limit[motorNumber];
        }
        target_points[motorNumber] = (int)newTarget;
      }
    }
  }
  else if (cmd.equalsIgnoreCase("calibration start")) {
    calibration_mode = true;
    Serial.println("CALIBRATION_STARTED");
  }
  else if (cmd.equalsIgnoreCase("calibration stop")) {
    calibration_mode = false;
    Serial.println("CALIBRATION_STOPPED");
    writeFullLog();
  }
  else if (cmd.startsWith("calibration set_min")) {
    int sp = cmd.lastIndexOf(' ');
    if (sp>0) {
      int motorIndex = cmd.substring(sp+1).toInt() - 1;
      if (motorIndex >=0 && motorIndex < NUM_MOTORS) {
        min_limit[motorIndex] = actual_reducer_points[motorIndex];
        Serial.print("MIN_SET:");
        Serial.print(motorIndex+1);
        Serial.print(":");
        Serial.println(min_limit[motorIndex]);
      }
    }
  }
  else if (cmd.startsWith("calibration set_max")) {
    int sp = cmd.lastIndexOf(' ');
    if (sp>0) {
      int motorIndex = cmd.substring(sp+1).toInt() - 1;
      if (motorIndex >=0 && motorIndex < NUM_MOTORS) {
        max_limit[motorIndex] = actual_reducer_points[motorIndex];
        Serial.print("MAX_SET:");
        Serial.print(motorIndex+1);
        Serial.print(":");
        Serial.println(max_limit[motorIndex]);
      }
    }
  }
  else if (cmd.equalsIgnoreCase("GET_POS")) {
    emitStatusLines();
  }
  else if (cmd.equalsIgnoreCase("STOP")) {
    for (int i=0;i<NUM_MOTORS;i++){
      steppers[i]->setSpeed(0);
      motor_moving[i] = false;
    }
    Serial.println("EMERGENCY_STOP");
  }

  serialBuffer = "";
  newData = false;
}

void critical_error_script() {
  if(!is_card_here) {
    while(true) {
      for (int i = 0; i < NUM_MOTORS; i++) {
        steppers[i]->runSpeed();
      }
    }
  }
}

// ---------------------- setup / loop ----------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  encoder.begin();

  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

  Serial.println("=== SYSTEM STARTING ===");

  // Ініціалізація SD карти
  if(!SD.begin(5)){
    Serial.println("SD_CARD_FAILED");
    is_card_here = false;
  } else {
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
      Serial.println("NO_SD_CARD");
      is_card_here = false;
    } else {
      is_card_here = true;
      Serial.println("SD card OK");
    }
  }

  for (int i = 0; i < NUM_MOTORS; i++) {
    steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
    steppers[i]->setSpeed(0);
  }

  critical_error_script();

  // Спроба завантажити дані з логу
  bool log_loaded = false;
  if (is_card_here) {
    log_loaded = loadFromLog();
  }

  initializeEncodersFromSavedPositions();

    Serial.println("=== SYSTEM READY ===");
  
  // Виводимо поточні значення для перевірки
  Serial.println("=== CURRENT VALUES ===");
  for (int i=0;i<NUM_MOTORS;i++){
    Serial.print("Motor ");
    Serial.print(i+1);
    Serial.print(": min=");
    Serial.print(min_limit[i]);
    Serial.print(", max=");
    Serial.print(max_limit[i]);
    Serial.print(", target=");
    Serial.print(target_points[i]);
    Serial.print(", actual=");
    Serial.println(actual_reducer_points[i]);
  }

  // ВІДПРАВЛЯЄМО ДАНІ ДЛЯ PYTHON ДОДАТКУ ПРИ СТАРТІ
  for (int i=0;i<NUM_MOTORS;i++){
    Serial.print("MOTOR_DATA:");
    Serial.print(i+1);
    Serial.print(":");
    Serial.print(min_limit[i]);
    Serial.print(":");
    Serial.print(max_limit[i]);
    Serial.print(":");
    Serial.print(target_points[i]);
    Serial.print(":");
    Serial.println(actual_reducer_points[i]);
  }

  delay(100);
  emitStatusLines();

}

bool anyMotorMoving() {
  for (int i=0;i<NUM_MOTORS;i++) if (motor_moving[i]) return true;
  return false;
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

  if (now - lastEncoderRead >= 10) {
    for (int i = 0; i < NUM_MOTORS; i++) {
      PCA9548A(i);
      actual_encoder_points[i] = encoder.rawAngle();

      delta = actual_encoder_points[i] - previous_encoder_points[i];
      if (delta > 2048) delta -= 4096;
      else if (delta < -2048) delta += 4096;
      if (abs(delta) < 2) delta = 0;

      actual_reducer_points[i] += delta;

      if (!calibration_mode) {
        if (actual_reducer_points[i] > max_limit[i]) actual_reducer_points[i] = max_limit[i];
        if (actual_reducer_points[i] < min_limit[i]) actual_reducer_points[i] = min_limit[i];
      }
      previous_encoder_points[i] = actual_encoder_points[i];
    }
    lastEncoderRead = now;
  }

  if (now - lastControlUpdate >= 20) {
    for (int i = 0; i < NUM_MOTORS; i++) {
      int error = target_points[i] - actual_reducer_points[i];
      int abs_error = abs(error);

      if(!is_this_first_error_check[i] && abs_error - previus_error[i] >= 100 ) {
        Serial.print("Cabel for ");
        Serial.print(i);
        Serial.print(" motor is not connected correctly");
        critical_error_script();
      } 

      bool at_min_limit = (!calibration_mode) && (actual_reducer_points[i] <= min_limit[i] && error < 0);
      bool at_max_limit = (!calibration_mode) && (actual_reducer_points[i] >= max_limit[i] && error > 0);

      if (at_min_limit || at_max_limit) {
        steppers[i]->setSpeed(0);
        motor_moving[i] = false;
        motor_direction[i] = 0;
        continue;
      }

      if (motor_moving[i]) {
        if (abs_error <= range) {
          steppers[i]->setSpeed(0);
          motor_moving[i] = false;
          motor_direction[i] = 0;
          request_log_write = true;
          is_this_first_error_check[i] = true;
        }
      } else {
        if (abs_error > hysteresis_range) {
          int new_direction = (error > 0) ? 1 : -1;
          float currentSpeed = (new_direction > 0) ? speedStepsPerSec : -speedStepsPerSec;
          steppers[i]->setSpeed(currentSpeed);
          motor_moving[i] = true;
          motor_direction[i] = new_direction;
        } else {
          steppers[i]->setSpeed(0);
          motor_moving[i] = false;
          is_this_first_error_check[i] = true;
        }
        if(motor_moving[i] == true && is_this_first_error_check[i] == true) {
          previus_error[i] = abs_error;
          is_this_first_error_check[i] = false;
        }
      }
    }
    lastControlUpdate = now;
  }

  bool anyMovingNow = anyMotorMoving();
  for (int i=0;i<NUM_MOTORS;i++){
    if (previous_motor_moving[i] && !motor_moving[i]) {
      request_log_write = true;
    }
    previous_motor_moving[i] = motor_moving[i];
  }

  if (request_log_write && !anyMovingNow) {
    writeFullLog();
    request_log_write = false;
  }

  for (int i = 0; i < NUM_MOTORS; i++) {
    steppers[i]->runSpeed();
  }

  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 1000) {
    for (int i = 0; i < NUM_MOTORS; i++) {
      int current_error = target_points[i] - actual_reducer_points[i];
      float currentSpeed = steppers[i]->speed();
      
      Serial.print("Motor ");
      Serial.print(i + 1);
      Serial.print(" | Target: ");
      Serial.print(target_points[i]);
      Serial.print(" | Actual: ");
      Serial.print(actual_reducer_points[i]);
      Serial.print(" | min:");
      Serial.print(min_limit[i]);
      Serial.print(" max:");
      Serial.print(max_limit[i]);
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