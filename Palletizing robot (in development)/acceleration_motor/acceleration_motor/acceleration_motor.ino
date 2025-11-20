#include <AccelStepper.h>

#define STEP_PIN 0
#define DIR_PIN 2
#define MS1_PIN 16
#define MS2_PIN 4

const float stepsPerRevolution = 200;
int microstepSetting = 2;
int accelerationRPM = 100;
float accelerationSpeed;   
bool is_it_start = false;

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);

  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, LOW);

  float desiredRPM = 300;
  float MaxRPM = 320;

  float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
  float Max_Speed_StepsPerSec = microstepSetting * stepsPerRevolution * MaxRPM / 60;
  accelerationSpeed = (microstepSetting * stepsPerRevolution * accelerationRPM) / 60.0; // <— Тепер глобальна змінна використовується

  stepper.setMaxSpeed(Max_Speed_StepsPerSec);
  stepper.setAcceleration(accelerationSpeed);
  stepper.moveTo(5000);
}

void loop() {
  stepper.setSpeed(accelerationSpeed);
  stepper.runSpeed();
}
