#include <TMCStepper.h>
#include <AccelStepper.h>

#define STEP_PIN 0
#define DIR_PIN 2
#define EN_PIN 5
#define MS1_PIN 16
#define MS2_PIN 4

#define UART_PIN 17      // PDN_UART TMC2208 підключений сюди (half-duplex)
#define R_SENSE 0.1f    // опір резистора (залежить від плати)

HardwareSerial TMCserial(2);
TMC2208Stepper driver(&TMCserial, R_SENSE);
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

const float stepsPerRevolution = 200.0;
int microstepSetting = 4;         // перевір джампери на драйвері
float desiredRPM = 300.0;
float maxRPM = 320.0;
float accelTimeSeconds = 1.0;

int accelerationRPM = 300;
float accelerationSpeed;

void setup() {
  Serial.begin(115200);
  Serial.println("=== Init TMC2208 + AccelStepper ===");

  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  // Конфігурація мікрокроків
  digitalWrite(MS1_PIN, HIGH);
  digitalWrite(MS2_PIN, LOW);

  // --- UART half-duplex ---
  TMCserial.begin(115200, SERIAL_8N1, UART_PIN, UART_PIN); // TX = RX = PDN_UART
  driver.begin();
  driver.toff(1);
  driver.blank_time(24);
  driver.rms_current(1200);      
  driver.microsteps(microstepSetting);
  driver.pwm_autoscale(false);
  driver.en_spreadCycle(true); // StealthChop (тихо працює)
  Serial.println("TMC2208 configured.");

  driver.irun(31);       // 100% струму під час руху
  driver.ihold(31);      // 50% струму при зупинці
  driver.iholddelay(10); // затримка ~5-10 сек

  // --- Розрахунок швидкостей ---
  float stepsPerSec = (microstepSetting * stepsPerRevolution * desiredRPM) / 60.0;
  float maxStepsPerSec = (microstepSetting * stepsPerRevolution * maxRPM) / 60.0;
  float accelStepsPerSec2 = maxStepsPerSec / accelTimeSeconds;
  accelerationSpeed = (microstepSetting * stepsPerRevolution * accelerationRPM) / 60.0; // <— Тепер глобальна змінна використовується

  stepper.setMaxSpeed(maxStepsPerSec);
  stepper.setAcceleration(accelStepsPerSec2);
  stepper.moveTo(5000);
}

void loop() {
  // if (stepper.distanceToGo() == 0) {
  //   stepper.moveTo(stepper.currentPosition());
  // }
  // stepper.run();

  stepper.setSpeed(accelerationSpeed);
  stepper.runSpeed();

  //stepper.setSpeed(0);
}