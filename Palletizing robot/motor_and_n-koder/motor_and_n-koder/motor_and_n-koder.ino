#include <Wire.h>
#include <AS5600.h>
#include <AccelStepper.h>

#define STEP_PIN 2
#define DIR_PIN 15

AS5600 encoder;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

const float stepsPerRevolution = 200;
int microstepSetting = 1;
float desiredRPM = 300;
float MaxRPM = 300;

float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
float Max_Speed_StepsPerSec = (microstepSetting * stepsPerRevolution * MaxRPM) / 60.0;

int reducer_point = 20000;
int range = 16;

long actual_reducer_point = 30000;     // накопичений кут
int previous_encoder_point = 0;    // попереднє значення енкодера
int actual_encoder_point = 0;
int reducer_limit = 10;            // скільки обертів перед обнуленням
int reducer_max_value = 40950;
bool is_direction_chosen = false;

void PCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
  delay(5);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  PCA9548A(0);
  encoder.begin();

  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);

  previous_encoder_point = encoder.rawAngle();
}

void loop() {
  actual_encoder_point = encoder.rawAngle();

  // обчислення зміни кута (враховує wrap-around)
  int delta = actual_encoder_point - previous_encoder_point;

  if (delta > 2048) {
    // перехід через 0 у зворотному напрямку (4095 -> 0)
    delta -= 4096;
  } else if (delta < -2048) {
    // перехід через 0 у прямому напрямку (0 -> 4095)
    delta += 4096;
  }

  // оновлюємо "розгорнутий" кут
  actual_reducer_point += delta;

  // якщо перевищили 10 повних обертів (40960)
  if (actual_reducer_point >= 4096 * reducer_limit) {
    actual_reducer_point = 0;
  }
  if(actual_reducer_point <= -1) {
    actual_reducer_point = 40950;
  }
  // --- приклад логіки керування ---
  // if (abs(actual_encoder_point - encoder_point) < range) {
  //   stepper.setSpeed(0);
  // } else {
  //   stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  //   stepper.setSpeed(speedStepsPerSec);
  // }

  if((actual_reducer_point >= reducer_point - range) && (actual_reducer_point <= reducer_point + range)) { 
    stepper.setSpeed(0); 
    is_direction_chosen = false;
  } else {
    if(is_direction_chosen == false) {
      if(actual_reducer_point <= reducer_point) { 
        speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
      } else {
        speedStepsPerSec = -(microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
      }
      stepper.setMaxSpeed(Max_Speed_StepsPerSec); 
      stepper.setSpeed(speedStepsPerSec);
      is_direction_chosen = true;
    }
  }

  stepper.runSpeed();

  // --- відладка ---
  Serial.print("Encoder: ");
  Serial.print(actual_encoder_point);
  Serial.print("   Reducer: ");
  Serial.println(actual_reducer_point);

  previous_encoder_point = actual_encoder_point;
  delay(10);
}




// if (actual_reducer_points[i] >= 4096 * reducer_limit) {
//       actual_reducer_points[i] = 0;
//     }
//     if(actual_reducer_points[i] <= -1) {
//       actual_reducer_points[i] = 40950;
//     }
