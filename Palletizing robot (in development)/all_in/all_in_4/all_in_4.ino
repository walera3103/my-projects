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
int target_1_point = 20000;
int target_2_point = 20000;
int target_3_point = 20000;
int target_4_point = 20000;
int target_5_point = 20000;
const int range = 500;
const int hysteresis_range = 800;

// Глобальні змінні - мають бути volatile для багатозадачності
volatile int previous_reducer_1_point;
volatile int previous_reducer_2_point;
volatile int previous_reducer_3_point;
volatile int previous_reducer_4_point;
volatile int previous_reducer_5_point;
volatile int actual_reducer_1_point;
volatile int actual_reducer_2_point;
volatile int actual_reducer_3_point;
volatile int actual_reducer_4_point;
volatile int actual_reducer_5_point;
volatile int previous_encoder_1_point;
volatile int previous_encoder_2_point;
volatile int previous_encoder_3_point;
volatile int previous_encoder_4_point;
volatile int previous_encoder_5_point;
volatile int actual_encoder_1_point;
volatile int actual_encoder_2_point;
volatile int actual_encoder_3_point;
volatile int actual_encoder_4_point;
volatile int actual_encoder_5_point;

AccelStepper* steppers[] = { &stepper_1, &stepper_2, &stepper_3, &stepper_4, &stepper_5 };

// Масиви вказівників на окремі змінні
volatile int* actual_encoder_points[] = { &actual_encoder_1_point, &actual_encoder_2_point, &actual_encoder_3_point, &actual_encoder_4_point, &actual_encoder_5_point };
volatile int* previous_encoder_points[] = { &previous_encoder_1_point, &previous_encoder_2_point, &previous_encoder_3_point, &previous_encoder_4_point, &previous_encoder_5_point };
volatile int* actual_reducer_points[] = { &actual_reducer_1_point, &actual_reducer_2_point, &actual_reducer_3_point, &actual_reducer_4_point, &actual_reducer_5_point };
volatile int* previous_reducer_points[] = { &previous_reducer_1_point, &previous_reducer_2_point, &previous_reducer_3_point, &previous_reducer_4_point, &previous_reducer_5_point };

volatile int target_points[] = { target_1_point, target_2_point, target_3_point, target_4_point, target_5_point };
volatile bool motor_moving[5] = { false, false, false, false, false };
volatile int motor_direction[5] = { 0, 0, 0, 0, 0 };

const int reducer_max_value = 40950;

// Змінні для синхронізації між задачами
SemaphoreHandle_t xMutex;
QueueHandle_t xSaveQueue;
TaskHandle_t xControlTaskHandle;
TaskHandle_t xSaveTaskHandle;

volatile bool is_card_here = true;
volatile bool need_save = false;

String serialBuffer = "";
bool newData = false;

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(1);
}

// Функції роботи з SD картою
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

bool loadPositionsFromLog() {
    
    Serial.println("Loading positions from log.txt...");
    
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
            
            if (position >= 0 && position <= reducer_max_value) {
                // Використовуємо розіменування вказівників
                *actual_reducer_points[i] = position;
                *previous_reducer_points[i] = position;

                // --- ОГРОМНО ВАЖЛИВО ---
                // Синхронізуємо previous_encoder та actual_encoder з реальним значенням енкодера
                // щоб при першому читанні delta було коректним і не породжувало стрибків.
                PCA9548A(i);
                int ang = encoder.rawAngle();
                *previous_encoder_points[i] = ang;
                *actual_encoder_points[i] = ang;

                Serial.print("Loaded position for motor ");
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(position);
                Serial.print("  (encoder=");
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
    
    return true;
}

// у глобальному:
// збільши розмір буфера, якщо потрібно
const size_t SAVE_BUFFER_SIZE = 1024;
char saveBuffer[SAVE_BUFFER_SIZE];

void saveTask(void * parameter) {
  uint8_t dummy;

  for (;;) {
    if (xQueueReceive(xSaveQueue, &dummy, portMAX_DELAY)) {

      // ---------------------------------------------
      // 1) Копіюємо дані під м’ютексом у локальний буфер
      // ---------------------------------------------
      String localData = "";

      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {

        for (int k = 0; k < 5; k++) {
          localData += "reducer position ";
          localData += (k + 1);
          localData += ": ";
          localData += String(*actual_reducer_points[k]);
          localData += "\n";
        }

        xSemaphoreGive(xMutex);
      }

      // ---------------------------------------------
      // 2) ПЕРЕЗАПИС ФАЙЛУ (truncate)
      //    ВАЖЛИВО: БЕЗ м’ютекса!
      // ---------------------------------------------
      if (is_card_here) {
        File file = SD.open("/log.txt", FILE_WRITE);  // перезапис!

        if (file) {
          file.print(localData);   // один великий запис
          file.close();
          Serial.println("File overwritten");
        } else {
          Serial.println("Failed to open file for rewrite");
        }
      }
    }

    vTaskDelay(1);
  }
}

// Задача для керування моторами (високий пріоритет)
void controlTask(void * parameter) {
  unsigned long lastEncoderRead = 0;
  unsigned long lastControlUpdate = 0;
  unsigned long lastSaveCheck = 0;
  int delta;
  uint8_t dummy = 1;
  
  for(;;) {
    unsigned long now = millis();

    // --- Читання енкодерів (кожні 10 мс) ---
    if (now - lastEncoderRead >= 10) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 5; i++) {
          PCA9548A(i);
          // Розіменування вказівника для отримання значення
          *actual_encoder_points[i] = encoder.rawAngle();

          // Розіменування вказівників для обчислення delta
          delta = *actual_encoder_points[i] - *previous_encoder_points[i];
          if (delta > 2048) delta -= 4096;
          else if (delta < -2048) delta += 4096;
          if (abs(delta) < 2) delta = 0;

          // Оновлення позиції редуктора
          *actual_reducer_points[i] += delta;

          // Обмеження позиції
          if (*actual_reducer_points[i] > reducer_max_value) {
            *actual_reducer_points[i] = reducer_max_value;
          } else if (*actual_reducer_points[i] < 0) {
            *actual_reducer_points[i] = 0;
          }

          // Оновлення попереднього значення енкодера
          *previous_encoder_points[i] = *actual_encoder_points[i];
        }
        xSemaphoreGive(xMutex);
      }
      lastEncoderRead = now;
    }

    // --- Керування моторами (кожні 10 ms) ---
    if (now - lastControlUpdate >= 10) {
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 5; i++) {
          // Розіменування для отримання значення
          int error = target_points[i] - *actual_reducer_points[i];
          int abs_error = abs(error);
          
          bool at_min_limit = (*actual_reducer_points[i] <= 0 && error < 0);
          bool at_max_limit = (*actual_reducer_points[i] >= reducer_max_value && error > 0);
          
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
        xSemaphoreGive(xMutex);
      }
      lastControlUpdate = now;
    }

    // --- Перевірка необхідності збереження (кожні 1000 мс) ---
    if (now - lastSaveCheck >= 1000) {
      bool should_save = false;
      
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        for (int i = 0; i < 5; i++) {
          if (abs(*actual_reducer_points[i] - *previous_reducer_points[i]) > range) {
            should_save = true;
            *previous_reducer_points[i] = *actual_reducer_points[i];
          }
        }
        xSemaphoreGive(xMutex);
      }
      
      if (should_save) {
        xQueueSend(xSaveQueue, &dummy, 0);
      }
      
      lastSaveCheck = now;
    }

    // --- Виконання кроків (найвищий пріоритет) ---
    for (int i = 0; i < 5; i++) {
      steppers[i]->runSpeed();
    }

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
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
    }
  }

  // Завантаження позицій (без тасків — безпечне)
  if (is_card_here && SD.exists("/log.txt")) {
    loadPositionsFromLog();
  } else {
    // якщо немає файлу — ініціалізуємо енкодери поточними значеннями
    for (int i = 0; i < 5; i++) {
      PCA9548A(i);
      int ang = encoder.rawAngle();
      *previous_encoder_points[i] = ang;
      *actual_encoder_points[i] = ang;
      *actual_reducer_points[i] = 0;
      *previous_reducer_points[i] = 0;
    }
  }

  // Ініціалізація моторів
  for (int i = 0; i < 5; i++) {
    steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
    steppers[i]->setSpeed(0);
  }

  // Створення м'ютексу для синхронізації
  xMutex = xSemaphoreCreateMutex();
  
  // Створення черги для сигналів збереження
  xSaveQueue = xQueueCreate(10, sizeof(uint8_t));

  if (xMutex == NULL) {
    Serial.println("Failed to create mutex");
    while(1);
  }
  
  if (xSaveQueue == NULL) {
    Serial.println("Failed to create queue");
    while(1);
  }

  // Створення задач FreeRTOS
  xTaskCreatePinnedToCore(
    controlTask,
    "ControlTask",
    10000,
    NULL,
    3,
    &xControlTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    saveTask,
    "SaveTask",
    8000,
    NULL,
    1,
    &xSaveTaskHandle,
    0
  );

  Serial.println("System initialized with FreeRTOS - 5 Motors Control");
  Serial.println("Current positions:");
  for (int i = 0; i < 5; i++) {
    Serial.print("Motor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(*actual_reducer_points[i]); // Розіменування
  }
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
          
          if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            target_points[motorNumber] = newTarget;
            xSemaphoreGive(xMutex);
          }
          
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

  static unsigned long lastDebug = 0;
  unsigned long now = millis();
  if (now - lastDebug >= 1000) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      for (int i = 0; i < 5; i++) {
        int current_error = target_points[i] - *actual_reducer_points[i]; // Розіменування
        float currentSpeed = steppers[i]->speed();
        
        Serial.print("Motor ");
        Serial.print(i + 1);
        Serial.print(" | Target: ");
        Serial.print(target_points[i]);
        Serial.print(" | Actual: ");
        Serial.print(*actual_reducer_points[i]); // Розіменування
        Serial.print(" | Error: ");
        Serial.print(abs(current_error));
        Serial.print(" | Speed: ");
        Serial.print(currentSpeed);
        Serial.print(" | State: ");
        Serial.println(motor_moving[i] ? "MOVING" : "STOPPED");
      }
      Serial.println("========================================");
      xSemaphoreGive(xMutex);
    }
    lastDebug = now;
  }

  vTaskDelay(10 / portTICK_PERIOD_MS);

  for (int i = 0; i < 5; i++) steppers[i]->runSpeed();

}
