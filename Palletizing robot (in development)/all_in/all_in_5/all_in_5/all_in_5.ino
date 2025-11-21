#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

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
int target_1_point;
int target_2_point;
int target_3_point;
int target_4_point;
int target_5_point;
const int range = 500;
const int hysteresis_range = 800;

// Глобальні змінні
int previous_reducer_1_point;
int previous_reducer_2_point;
int previous_reducer_3_point;
int previous_reducer_4_point;
int previous_reducer_5_point;
int actual_reducer_1_point;
int actual_reducer_2_point;
int actual_reducer_3_point;
int actual_reducer_4_point;
int actual_reducer_5_point;
int previous_encoder_1_point;
int previous_encoder_2_point;
int previous_encoder_3_point;
int previous_encoder_4_point;
int previous_encoder_5_point;
int actual_encoder_1_point;
int actual_encoder_2_point;
int actual_encoder_3_point;
int actual_encoder_4_point;
int actual_encoder_5_point;

AccelStepper* steppers[] = { &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 };

// Масиви вказівників на окремі змінні
int* actual_encoder_points[] = { &actual_encoder_1_point, &actual_encoder_2_point, &actual_encoder_3_point, &actual_encoder_4_point, &actual_encoder_5_point };
int* previous_encoder_points[] = { &previous_encoder_1_point, &previous_encoder_2_point, &previous_encoder_3_point, &previous_encoder_4_point, &previous_encoder_5_point };
int* actual_reducer_points[] = { &actual_reducer_1_point, &actual_reducer_2_point, &actual_reducer_3_point, &actual_reducer_4_point, &actual_reducer_5_point };
int* previous_reducer_points[] = { &previous_reducer_1_point, &previous_reducer_2_point, &previous_reducer_3_point, &previous_reducer_4_point, &previous_reducer_5_point };
int* targets[] { &target_1_point, &target_2_point, &target_3_point, &target_4_point, &target_5_point };

int target_points[] = { target_1_point, target_2_point, target_3_point, target_4_point, target_5_point };
bool motor_moving[5] = { false, false, false, false, false };
int motor_direction[5] = { 0, 0, 0, 0, 0 };

char str[11];
unsigned long lastEncoderRead = 0;
unsigned long lastControlUpdate = 0;
unsigned long lastSaveCheck = 0;
int delta;

// Динамічні межі для калібрування
int reducer_min_value[5] = {0, 0, 0, 0, 0};
int reducer_max_value[5] = {40950, 40950, 40950, 40950, 40950};
bool calibration_mode = false;

String serialBuffer = "";
bool newData = false;
bool is_card_here = true;

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(1);
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  File file = fs.open(path, FILE_WRITE);
  if(!file) return;
  file.print(message);
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message, bool with_new_line){
  File file = fs.open(path, FILE_APPEND);
  if(!file) return;
  if (with_new_line) {
    file.println(message);
  } else {
    file.print(message);
  }
  file.close();
}

// Оновлена функція завантаження позицій та меж
bool loadPositionsAndLimitsFromLog() {
    Serial.println("Loading positions and limits from log.txt...");
    
    File file = SD.open("/log.txt");
    if (!file) {
        Serial.println("Failed to open log.txt for reading");
        return false;
    }
    
    String fileContent = "";
    while (file.available()) {
        fileContent += file.readString();
    }
    file.close();
    
    // Для кожного мотору: знайшли останнє входження і встановили reducer.
    for (int i = 0; i < 5; i++) {
        String searchString = "reducer position " + String(i + 1) + ": ";
        int startIndex = fileContent.lastIndexOf(searchString);
        
        if (startIndex != -1) {
            startIndex += searchString.length();
            int endIndex = fileContent.indexOf('\n', startIndex);
            if (endIndex == -1) endIndex = fileContent.length();
            
            String positionStr = fileContent.substring(startIndex, endIndex);
            positionStr.trim();
            
            int position = positionStr.toInt();
            
            if (position >= 0 && position <= reducer_max_value[i]) {
                *actual_reducer_points[i] = position;
                *previous_reducer_points[i] = position;
                
                // Синхронізуємо previous_encoder та actual_encoder з реальним значенням енкодера
                PCA9548A(i);
                int ang = encoder.rawAngle();
                *previous_encoder_points[i] = ang;
                *actual_encoder_points[i] = ang;
                
                Serial.print("Loaded position for motor ");
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(position);
                Serial.print(" (encoder=");
                Serial.print(ang);
                Serial.println(")");
            } else {
                // якщо парсинг не дав коректного — ініціалізуємо нулями
                *actual_reducer_points[i] = 0;
                *previous_reducer_points[i] = 0;
                PCA9548A(i);
                int ang = encoder.rawAngle();
                *previous_encoder_points[i] = ang;
                *actual_encoder_points[i] = ang;
                Serial.print("Invalid position in file for motor ");
                Serial.print(i + 1);
                Serial.println(", set to 0");
            }
        } else {
            // немає запису — ініціалізуємо нулями та синхронізуємо енкодер
            *actual_reducer_points[i] = 0;
            *previous_reducer_points[i] = 0;
            PCA9548A(i);
            int ang = encoder.rawAngle();
            *previous_encoder_points[i] = ang;
            *actual_encoder_points[i] = ang;
            Serial.print("No entry for motor ");
            Serial.print(i + 1);
            Serial.println(", set to 0");
        }
    }
    
    // Шукаємо межі калібрування
    for (int i = 0; i < 5; i++) {
        String minSearchString = "calibration min " + String(i + 1) + ": ";
        String maxSearchString = "calibration max " + String(i + 1) + ": ";
        
        int minStartIndex = fileContent.lastIndexOf(minSearchString);
        int maxStartIndex = fileContent.lastIndexOf(maxSearchString);
        
        if (minStartIndex != -1) {
            minStartIndex += minSearchString.length();
            int minEndIndex = fileContent.indexOf('\n', minStartIndex);
            String minStr = fileContent.substring(minStartIndex, minEndIndex);
            reducer_min_value[i] = minStr.toInt();
            Serial.print("Loaded min limit for motor ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(reducer_min_value[i]);
        }
        
        if (maxStartIndex != -1) {
            maxStartIndex += maxSearchString.length();
            int maxEndIndex = fileContent.indexOf('\n', maxStartIndex);
            String maxStr = fileContent.substring(maxStartIndex, maxEndIndex);
            reducer_max_value[i] = maxStr.toInt();
            Serial.print("Loaded max limit for motor ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.println(reducer_max_value[i]);
        }
    }
    
    return true;
}

// Оптимізована функція збереження позицій та меж
void savePositionsAndLimitsToSD() {
    if (!is_card_here) return;
    
    // Буферизуємо всі дані перед записом
    String saveData = "";
    for (int i = 0; i < 5; i++) {
        saveData += "reducer position ";
        saveData += String(i + 1);
        saveData += ": ";
        saveData += String(*actual_reducer_points[i]);
        saveData += "\n";
        
        saveData += "calibration min ";
        saveData += String(i + 1);
        saveData += ": ";
        saveData += String(reducer_min_value[i]);
        saveData += "\n";
        
        saveData += "calibration max ";
        saveData += String(i + 1);
        saveData += ": ";
        saveData += String(reducer_max_value[i]);
        saveData += "\n";
    }
    
    // Один запис у файл
    writeFile(SD, "/log.txt", saveData.c_str());
    Serial.println("Positions and calibration limits saved to SD");
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    encoder.begin();

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);

    // Ініціалізація SD карти
    if(!SD.begin(5)){
        Serial.println("Card Mount Failed");
        is_card_here = false;
    } else {
        uint8_t cardType = SD.cardType();
        if(cardType == CARD_NONE){
            Serial.println("No SD card attached");
            is_card_here = false;
        } else {
            Serial.println("SD card initialized successfully");
        }
    }

    // Завантаження позицій та меж
    if (is_card_here && SD.exists("/log.txt")) {
        if (!loadPositionsAndLimitsFromLog()) {
            Serial.println("Using default positions and limits");
        }
    } else {
        // Якщо немає файлу — ініціалізуємо енкодери поточними значеннями
        for (int i = 0; i < 5; i++) {
            PCA9548A(i);
            int ang = encoder.rawAngle();
            *previous_encoder_points[i] = ang;
            *actual_encoder_points[i] = ang;
            *actual_reducer_points[i] = 0;
            *previous_reducer_points[i] = 0;
        }
        Serial.println("Using default positions and limits");
    }

    // Ініціалізація моторів
    for (int i = 0; i < 5; i++) {
        steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
        steppers[i]->setSpeed(0);

        target_points[i] = *actual_reducer_points[i];
    }

    Serial.println("System initialized - 5 Motors Control with Calibration");
    Serial.println("Current positions and limits:");
    for (int i = 0; i < 5; i++) {
        Serial.print("Motor ");
        Serial.print(i + 1);
        Serial.print(" - Pos: ");
        Serial.print(*actual_reducer_points[i]);
        Serial.print(" | Min: ");
        Serial.print(reducer_min_value[i]);
        Serial.print(" | Max: ");
        Serial.println(reducer_max_value[i]);
    }
}

void parseSerialData() {
    if (newData) {
        // Команди для звичайного режиму
        if (serialBuffer.startsWith("motor")) {
            int motorNumber = serialBuffer.charAt(5) - '1';
            if (motorNumber >= 0 && motorNumber < 5) {
                int spaceIndex = serialBuffer.indexOf(' ');
                if (spaceIndex != -1) {
                    long newTarget = serialBuffer.substring(spaceIndex + 1).toInt();
                    // Обмежуємо в звичайному режимі, в калібрувальному - ні
                    if (!calibration_mode) {
                        newTarget = constrain(newTarget, reducer_min_value[motorNumber], reducer_max_value[motorNumber]);
                    }
                    target_points[motorNumber] = newTarget;
                    Serial.print("Motor ");
                    Serial.print(motorNumber + 1);
                    Serial.print(" target set to: ");
                    Serial.println(newTarget);
                }
            }
        }
        // Команди для калібрування
        else if (serialBuffer.startsWith("calibration ")) {
            if (serialBuffer.indexOf("start") != -1) {
                calibration_mode = true;
                Serial.println("CALIBRATION MODE STARTED - limits disabled");
            }
            else if (serialBuffer.indexOf("stop") != -1) {
                calibration_mode = false;
                Serial.println("CALIBRATION MODE STOPPED - limits enabled");
                // Зберігаємо нові межі
                savePositionsAndLimitsToSD();
            }
            else if (serialBuffer.startsWith("calibration set_min ")) {
                int motorNumber = serialBuffer.charAt(20) - '1';
                if (motorNumber >= 0 && motorNumber < 5) {
                    reducer_min_value[motorNumber] = *actual_reducer_points[motorNumber];
                    Serial.print("Motor ");
                    Serial.print(motorNumber + 1);
                    Serial.print(" min limit set to: ");
                    Serial.println(reducer_min_value[motorNumber]);
                }
            }
            else if (serialBuffer.startsWith("calibration set_max ")) {
                int motorNumber = serialBuffer.charAt(20) - '1';
                if (motorNumber >= 0 && motorNumber < 5) {
                    reducer_max_value[motorNumber] = *actual_reducer_points[motorNumber];
                    Serial.print("Motor ");
                    Serial.print(motorNumber + 1);
                    Serial.print(" max limit set to: ");
                    Serial.println(reducer_max_value[motorNumber]);
                }
            }
        }
        serialBuffer = "";
        newData = false;
    }
}

void loop() {
    // Обробка Serial даних
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

    // --- Читання енкодерів (кожні 10 мс) ---
    if (now - lastEncoderRead >= 10) {
        for (int i = 0; i < 5; i++) {
            PCA9548A(i);
            *actual_encoder_points[i] = encoder.rawAngle();

            delta = *actual_encoder_points[i] - *previous_encoder_points[i];
            if (delta > 2048) delta -= 4096;
            else if (delta < -2048) delta += 4096;
            if (abs(delta) < 2) delta = 0;

            *actual_reducer_points[i] += delta;

            // Обмеження позиції тільки в звичайному режимі
            if (!calibration_mode) {
                if (*actual_reducer_points[i] > reducer_max_value[i]) {
                    *actual_reducer_points[i] = reducer_max_value[i];
                } else if (*actual_reducer_points[i] < reducer_min_value[i]) {
                    *actual_reducer_points[i] = reducer_min_value[i];
                }
            }

            *previous_encoder_points[i] = *actual_encoder_points[i];
        }
        lastEncoderRead = now;
    }

    // --- Керування моторами (кожні 20 мс) ---
    if (now - lastControlUpdate >= 20) {
        for (int i = 0; i < 5; i++) {
            int error = target_points[i] - *actual_reducer_points[i];
            int abs_error = abs(error);
            
            // Перевірка меж тільки в звичайному режимі
            bool at_min_limit = false;
            bool at_max_limit = false;
            
            if (!calibration_mode) {
                at_min_limit = (*actual_reducer_points[i] <= reducer_min_value[i] && error < 0);
                at_max_limit = (*actual_reducer_points[i] >= reducer_max_value[i] && error > 0);
            }
            
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
                }
            } else {
                if (abs_error > hysteresis_range) {
                    int new_direction = (error > 0) ? 1 : -1;
                    float currentSpeed = (new_direction > 0) ? speedStepsPerSec : -speedStepsPerSec;
                    steppers[i]->setSpeed(currentSpeed);
                    motor_moving[i] = true;
                    motor_direction[i] = new_direction;
                }
            }
        }
        lastControlUpdate = now;
    }

    // --- Перевірка необхідності збереження (кожні 1000 мс) ---
    if (now - lastSaveCheck >= 1000) {
        bool should_save = false;
        
        for (int i = 0; i < 5; i++) {
            if (abs(*actual_reducer_points[i] - *previous_reducer_points[i]) > range) {
                should_save = true;
                *previous_reducer_points[i] = *actual_reducer_points[i];
            }
        }
        
        if (should_save) {
            savePositionsAndLimitsToSD();
        }
        
        lastSaveCheck = now;
    }

    // --- Виконання кроків ---
    for (int i = 0; i < 5; i++) {
        steppers[i]->runSpeed();
    }

    // --- Відладка (кожну секунду) ---
    static unsigned long lastDebug = 0;
    if (now - lastDebug >= 1000) {
        Serial.print("Mode: ");
        Serial.println(calibration_mode ? "CALIBRATION" : "NORMAL");
        for (int i = 0; i < 5; i++) {
            int current_error = target_points[i] - *actual_reducer_points[i];
            float currentSpeed = steppers[i]->speed();
            
            Serial.print("Motor ");
            Serial.print(i + 1);
            Serial.print(" | Target: ");
            Serial.print(target_points[i]);
            Serial.print(" | Actual: ");
            Serial.print(*actual_reducer_points[i]);
            Serial.print(" | Min: ");
            Serial.print(reducer_min_value[i]);
            Serial.print(" | Max: ");
            Serial.print(reducer_max_value[i]);
            Serial.print(" | Error: ");
            Serial.print(abs(current_error));
            Serial.print(" | Speed: ");
            Serial.print(currentSpeed);
            Serial.print(" | State: ");
            Serial.println(motor_moving[i] ? "MOVING" : "STOPPED");
        }
        Serial.println("========================================");
        lastDebug = now;
    }
}