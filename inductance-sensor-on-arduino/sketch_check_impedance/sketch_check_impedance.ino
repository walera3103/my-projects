double pulse, frequency, capacitance, inductance;
void setup(){
  Serial.begin(115200); // Для симуляции в Proteus заменить на 9600(115200 не поддерживается)
  pinMode(11, INPUT);
  pinMode(13, OUTPUT);
  Serial.println("************************ INDUCTANCE **********************");
  delay(300);
  Serial.println("");
  Serial.println(">>>>>>>>>>>>>>>>>>>>>>>> SOLAR CELLS <<<<<<<<<<<<<<<<<<<<<");
  Serial.println("");
  delay(500);}
void loop(){
  digitalWrite(13, HIGH);
  delay(5);
  digitalWrite(13,LOW);
  delayMicroseconds(100); 
  pulse = pulseIn(11,HIGH,5000);
  //Serial.println(pulse);
  if(pulse > 0.1)
{ 
  capacitance = 0.474E-6; // Вводим емкость конденсатора 0.515E-6 = 0,515 мКф(uF)
  frequency = 1.E6/(2*pulse);
  inductance = 1./(capacitance*frequency*frequency*4.*3.14159*3.14159);
  inductance *= 1E6;
  Serial.print("\tfrequency Hz:");
  Serial.print( frequency );
  Serial.print("\tinductance uH:");
  Serial.println( inductance );
  delay(300); //изменяем с 10 до 300 - кому как нравится.
  }
}
