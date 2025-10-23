#include <Wire.h>
#include <AS5600.h>

AS5600 encoder;  // створюємо об'єкт
int angle_encoder_1;
int angle_encoder_2;
int counter = 0;
float deg_encoder_1;
float deg_encoder_2;

void encoder_check(int &angle, int counter, float &deg) {
  angle = encoder.rawAngle();      // читає “сирий” кут (0–4095)
  deg = angle * 360.0 / 4096.0;  // перетворюємо в градуси

  Serial.print("Encoder ");
  Serial.println(counter);
  Serial.print("Raw: ");
  Serial.print(angle);
  Serial.print("   Deg: ");
  Serial.println(deg, 2);

  delay(1000); // MIN 10!!!
}

void PCA9548A(uint8_t bus){
  Wire.beginTransmission(0x70);  // PCA9548A address
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
  delay(1);
  //Serial.print(bus);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL ESP32
  Serial.println("AS5600 test start...");
}

void loop() {
  PCA9548A(counter);
  encoder_check(angle_encoder_1, counter, deg_encoder_1);
  counter = 1;
  PCA9548A(counter);
  encoder_check(angle_encoder_2, counter, deg_encoder_2);
  counter = 0;

  // int angle = encoder.rawAngle();      // читає “сирий” кут (0–4095)
  // float deg = angle * 360.0 / 4096.0;  // перетворюємо в градуси

  // Serial.print("Raw: ");
  // Serial.print(angle);
  // Serial.print("   Deg: ");
  // Serial.println(deg, 2);

  //delay(1000);
}
