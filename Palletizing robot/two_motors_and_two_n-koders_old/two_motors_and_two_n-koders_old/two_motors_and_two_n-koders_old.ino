#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>

#define STEP_PIN_1 2
#define STEP_PIN_2 16
#define DIR_PIN_1 15
#define DIR_PIN_2 4

AS5600 encoder;
AccelStepper stepper_1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper_2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

const float stepsPerRevolution = 200;
int microstepSetting = 1;
float desiredRPM = 300;
float MaxRPM = 300;

float speedStepsPerSec_1 = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float speedStepsPerSec_2 = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = (microstepSetting * stepsPerRevolution * MaxRPM) / 60.0;

int reducer_1_point = 20000;
int reducer_2_point = 20000;
int range = 300;
int hysteresis = 10;      // додатковий запас для запуску

long actual_reducer_1_point = 30000;
long actual_reducer_2_point = 0;
int previous_encoder_1_point = 0;
int previous_encoder_2_point = 0;
int actual_encoder_1_point = 0;
int actual_encoder_2_point = 0;

AccelStepper* steppers[] = { &stepper_1, &stepper_2 };
int actual_encoder_points[] = { actual_encoder_1_point, actual_encoder_2_point };
int previous_encoder_points[] = { previous_encoder_1_point, previous_encoder_2_point };
long actual_reducer_points[] = { actual_reducer_1_point, actual_reducer_2_point };
int reducer_points[] = { reducer_1_point, reducer_2_point };
int error;
bool directions[2] = { false, false };
float speeds[2] = { speedStepsPerSec_1, speedStepsPerSec_2 };

unsigned long lastEncoderRead = 0;
int delta;

// --- межі редуктора (приблизно 10 обертів по 4096) ---
const long reducer_max_value = 40950;

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(2);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  encoder.begin();

  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

  for (int i = 0; i < 2; i++) {
    steppers[i]->setSpeed(0);
  }
}

void loop() {
  unsigned long now = millis();

  // --- Читання енкодерів раз на 10 мс ---
  if (now - lastEncoderRead >= 10) {
    for (int i = 0; i < 2; i++) {
      PCA9548A(i);
      actual_encoder_points[i] = encoder.rawAngle();

      // розрахунок delta з wrap-around
      delta = actual_encoder_points[i] - previous_encoder_points[i];
      if (delta > 2048) delta -= 4096;
      else if (delta < -2048) delta += 4096;

      // фільтр дрібних стрибків
      if (abs(delta) < 2) delta = 0;

      // оновлюємо "розгорнутий" кут
      actual_reducer_points[i] += delta;

      // --- КОРЕКЦІЯ ЦИКЛУ ---
      if (actual_reducer_points[i] > reducer_max_value) {
        actual_reducer_points[i] -= (reducer_max_value + 1);
      } else if (actual_reducer_points[i] < 0) {
        actual_reducer_points[i] += (reducer_max_value + 1);
      }

      previous_encoder_points[i] = actual_encoder_points[i];
    }
    lastEncoderRead = now;
  }

  // --- Логіка керування моторами ---
  for (int i = 0; i < 2; i++) {
    error = reducer_points[i] - actual_reducer_points[i];

    // Якщо мотор вже вибрав напрямок, просто рухаємо його
    if (directions[i]) {
        // Якщо досягли діапазону, зупиняємо мотор
        if (abs(error) <= range) {
            steppers[i]->setSpeed(0);
            //directions[i] = false;  // скидаємо прапор, щоб можна було вибрати напрямок знову
        } else {
            // Просто продовжуємо рух у вибраному напрямку
            //steppers[i]->runSpeed();
        }
    } 
    // Якщо напрямок ще не обрано і помилка велика
    else if (!directions[i] && !((error >= reducer_points[i] - range) && (error <= reducer_points[i] + range))) {
        speeds[i] = (error > 0) ? (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0
                                 : -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
        steppers[i]->setMaxSpeed(Max_Speed_StepsPerSec);
        steppers[i]->setSpeed(speeds[i]);
        directions[i] = true;  // напрямок обрано
        //steppers[i]->runSpeed();
    }
    for (int k = 0; k < 2; k++) {
      steppers[k]->runSpeed();
    }
}



  // --- Відладка ---
  if (now % 500 < 5) {
    for (int i = 0; i < 2; i++) {
      Serial.print("Enc ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(actual_encoder_points[i]);
      Serial.print(" | Red ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(actual_reducer_points[i]);
      Serial.println("___________________________");
      Serial.println(abs(error));
    }
  }
}
