#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;  // створюємо об'єкт

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL ESP32
  Serial.println("AS5600 test start...");
}

void loop() {
  int angle = encoder.rawAngle();      // читає “сирий” кут (0–4095)
  float deg = angle * 360.0 / 4096.0;  // перетворюємо в градуси

  Serial.print("Raw: ");
  Serial.print(angle);
  Serial.print("   Deg: ");
  Serial.println(deg, 2);

  delay(200);
}
