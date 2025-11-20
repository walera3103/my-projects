#include <Arduino.h>
#include <AccelStepper.h>

// ====== Піни крокового двигуна ======
#define STEP_PIN 2     // Step pin
#define DIR_PIN 15       // Direction pin
#define ENABLE_PIN   0   // Pin for ENABLE (active LOW)

// ====== Керування мікрокроком ======
// #define M0_PIN 13
// #define M1_PIN 12
// #define M2_PIN 14

// ====== Потенціометр ======
#define POT_PIN 34   // аналоговий вхід ESP32 (GPIO34)

// ====== Параметри двигуна ======
const float stepsPerRevolution = 200;
int microstepSetting = 1;  // множник мікрокроку

// ====== Параметри PWM ======
const int pwmFreq = 30000;     // частота ~3 кГц (оптимально для DRV8825)
const int pwmResolution = 8;  // 8-біт (0–255)
int pwmValue = 255;           // початкове значення (100% потужність)

// ====== Ініціалізація AccelStepper ======
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

void setup() {
  Serial.begin(115200);

  // Встановлення пінів
  // pinMode(M0_PIN, OUTPUT);
  // pinMode(M1_PIN, OUTPUT);
  // pinMode(M2_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  // Встановлюємо мікрокрок (LOW/LOW/LOW = 1/1)
  // digitalWrite(M0_PIN, LOW);
  // digitalWrite(M1_PIN, LOW);
  // digitalWrite(M2_PIN, LOW);

  // ====== PWM для ENABLE (новий LEDC API) ======
  // ledcAttach(pin, freq, resolution_bits)
  ledcAttach(ENABLE_PIN, pwmFreq, pwmResolution);

  // ledcWrite(pin, duty)
  ledcWrite(ENABLE_PIN, pwmValue);  // 100% потужність на старті

  // ====== Налаштування двигуна ======
  float desiredRPM = 300;
  float maxRPM = 300;
  float speedStepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
  float maxSpeedStepsPerSec = (microstepSetting * stepsPerRevolution * maxRPM) / 60.0;

  stepper.setMaxSpeed(maxSpeedStepsPerSec);
  stepper.setSpeed(speedStepsPerSec);

  Serial.println("DRV8825 PWM control via potentiometer (new LEDC API) ready!");
}

void loop() {
  // ====== Зчитуємо потенціометр ======
  int potValue = analogRead(POT_PIN);     // 0–4095 на ESP32
  pwmValue = map(potValue, 0, 4095, 0, 255); // мапимо у 0–255 для PWM duty

  // ====== Застосовуємо PWM на ENABLE ======
  ledcWrite(ENABLE_PIN, pwmValue);

  // ====== (Опціонально) показуємо відсотки у Serial ======
  int percent = map(pwmValue, 0, 255, 0, 100);
  Serial.print("PWM Duty: %3d /255  (%2d%%)\r");
  Serial.println(pwmValue);
  //Serial.println(percent);

  // ====== Рух (або утримування) ======
  // Якщо хочеш рухати мотор — розкоментуй рядок нижче:
  //stepper.runSpeed();

  //delay(50); // невелика затримка для плавності читання
}
