/******************************************************************************
ISL29125_basics.ino
Simple example for using the ISL29125 RGB sensor library.
Jordan McConnell @ SparkFun Electronics
11 Apr 2014
https://github.com/sparkfun/ISL29125_Breakout

This example declares an SFE_ISL29125 object called RGB_sensor. The 
object/sensor is initialized with a basic configuration so that it continuously
samples the light intensity of red, green and blue spectrums. These values are
read from the sensor every 2 seconds and printed to the Serial monitor.

Developed/Tested with:
Arduino Uno
Arduino IDE 1.0.5

Requires:
SFE_ISL29125_Library

This code is beerware.
Distributed as-is; no warranty is given. 
******************************************************************************/

#include <Wire.h>
#include "SFE_ISL29125.h"

// Declare sensor object
SFE_ISL29125 RGB_sensor;

unsigned int red_low = 656; // 656 -> 800
unsigned int red_high = 2173; // 2173
unsigned int green_low = 1687; // 1687 -> 1950 -> 2200 -> 3200 -> 3295
unsigned int green_high = 4007; // 4007
unsigned int blue_low = 1447; // 1447 -> 1680 -> 2000 -> 2020 -> 1765
unsigned int blue_high = 3515;

int red_Val = 0;
int red_V = 0;
int average_red_value = 0;
int number_of_red_color = 0;
int green_Val = 0;
int green_V = 0;
int average_green_value = 0;
int number_of_green_color = 0;
int blue_Val = 0;
int blue_V = 0;
int average_blue_value = 0;
int number_of_blue_color = 0;
int start_value = 0;
int final_color = 0;
bool is_first_filtr_work = true;
bool is_second_filtr_work = true;

int red_array[10];
int green_array[12];
int blue_array[12];

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the ISL29125 with simple configuration so it starts sampling
  if (RGB_sensor.init())
  {
    Serial.println("Sensor Initialization Successful\n\r");
  }
}

// Read sensor values for each color and print them to serial monitor
void loop()
{
  // Read sensor values (16 bit integers)
  unsigned int red = RGB_sensor.readRed();
  unsigned int green = RGB_sensor.readGreen();
  unsigned int blue = RGB_sensor.readBlue();
  average_red_value = 0;
  number_of_red_color = 0;
  average_green_value = 0;
  number_of_green_color = 0;
  average_blue_value = 0;
  number_of_blue_color = 0;
  start_value = 0;
  final_color = 0;
  is_first_filtr_work = true;
  is_second_filtr_work = true;

  for(int i = 0; i < 5; i++) {
    red_V = map(red, red_low, red_high, 0, 255);
    green_V = map(green, green_low, green_high, 0, 255);
    blue_V = map(blue, blue_low, blue_high, 0, 255);

    red_Val = constrain(red_V, 0, 255);
    green_Val = constrain(green_V, 0, 255);
    blue_Val = constrain(blue_V, 0, 255);

    average_red_value = average_red_value + red_Val;
    average_green_value = average_green_value + green_Val;
    average_blue_value = average_blue_value + blue_Val;
    delay(200);
  }

  average_red_value = average_red_value / 5;
  average_green_value = average_green_value / 5;
  average_blue_value = average_blue_value / 5;

  if(average_red_value <= 10 && average_green_value <= 10 && average_blue_value <= 10) {
    final_color = 0;
  } else if(average_red_value <= 255 && average_red_value >= 245 && average_green_value <= 255 && average_green_value >= 245 && average_blue_value <= 255 && average_blue_value >= 145) {
    final_color = 2;
  } else if(average_red_value <= 255 && average_red_value >= 245 && average_green_value <= 255 && average_green_value >= 245 && average_blue_value >= 113 && average_blue_value <= 133) {
    final_color = 7;
  } else if(average_red_value <= 255 && average_red_value >= 245 && average_green_value >= 110 && average_green_value <= 130 && average_blue_value >= 41 && average_blue_value <= 61) {
    final_color = 8;
  } else if(average_red_value <= 255 && average_red_value >= 245 && average_green_value <= 10 && average_blue_value >= 25 && average_blue_value <= 45) {
    final_color = 9;
  } else if(average_red_value >= 220 && average_red_value <= 240 && average_green_value >= 90 && average_green_value <= 110 && average_blue_value >= 165 && average_blue_value <= 185) {
    final_color = 1;
  }
  // red_array[0] = abs(0 - average_red_value);
  // red_array[1] = abs(167 - average_red_value);
  // red_array[2] = abs(255 - average_red_value);
  // red_array[3] = abs(46 - average_red_value);
  // red_array[4] = abs(141 - average_red_value);
  // red_array[5] = abs(255 - average_red_value);
  // red_array[6] = abs(241 - average_red_value);
  // red_array[7] = abs(237 - average_red_value);
  // red_array[8] = abs(247 - average_red_value);
  // red_array[9] = abs(146 - average_red_value);

  // green_array[0] = abs(0 - average_green_value);
  // green_array[1] = abs(169 - average_green_value);
  // green_array[2] = abs(255 - average_green_value);
  // green_array[3] = abs(49 - average_green_value);
  // green_array[4] = abs(174 - average_green_value);
  // green_array[5] = abs(161 - average_green_value);
  // green_array[6] = abs(198 - average_green_value);
  // green_array[7] = abs(222 - average_green_value);
  // green_array[8] = abs(90 - average_green_value);
  // green_array[9] = abs(28 - average_green_value);
  // green_array[10] = abs(166 - average_green_value);
  // green_array[11] = abs(39 - average_green_value);

  // blue_array[0] = abs(0 - average_blue_value);
  // blue_array[1] = abs(172 - average_blue_value);
  // blue_array[2] = abs(255 - average_blue_value);
  // blue_array[3] = abs(146 - average_blue_value);
  // blue_array[4] = abs(239 - average_blue_value);
  // blue_array[5] = abs(75 - average_blue_value);
  // blue_array[6] = abs(63 - average_blue_value);
  // blue_array[7] = abs(23 - average_blue_value);
  // blue_array[8] = abs(41 - average_blue_value);
  // blue_array[9] = abs(36 - average_blue_value);
  // blue_array[10] = abs(218 - average_blue_value);
  // blue_array[11] = abs(143 - average_blue_value);

  // start_value = red_array[0];
  // for(int i = 1; i < 10; i++) {
  //   if(red_array[i] < start_value) {
  //     start_value = red_array[i];
  //     number_of_red_color = i;
  //   }
  // }

  // start_value = green_array[0];
  // for(int i = 1; i < 12; i++) {
  //   if(green_array[i] < start_value) {
  //     start_value = green_array[i];
  //     number_of_green_color = i;
  //   }
  // }

  // start_value = blue_array[0];
  // for(int i = 1; i < 12; i++) {
  //   if(green_array[i] < start_value) {
  //     start_value = blue_array[i];
  //     number_of_blue_color = i;
  //   }
  // }

  // if(number_of_red_color == 0 && number_of_green_color == 0 && number_of_blue_color == 0) {
  //   final_color = 0;
  // } else if(number_of_red_color == 0 && number_of_green_color == 4 && number_of_blue_color == 4 ) {
  //   final_color = 4;
  // } else if(number_of_red_color == 0 && number_of_green_color == 5 && number_of_blue_color == 5) {
  //   final_color = 5;
  // } else if(number_of_red_color == number_of_green_color && number_of_red_color == number_of_blue_color) {
  //   final_color = number_of_red_color;
  // } else if(number_of_red_color == number_of_green_color || number_of_red_color == number_of_blue_color){
  //   final_color = number_of_red_color;
  // } else if(number_of_green_color == number_of_blue_color) {
  //   final_color = number_of_green_color;
  // } else {
  //   final_color = (number_of_red_color + number_of_green_color + number_of_blue_color) / 3;
  // }
  
  // Print out readings, change HEX to DEC if you prefer decimal output
  Serial.print("Red: "); Serial.println(red_Val);
  Serial.print("Red: "); Serial.println(average_red_value);
  Serial.print("Green: "); Serial.println(green_Val);
  Serial.print("Green: "); Serial.println(average_green_value);
  Serial.print("Blue: "); Serial.println(blue_Val);
  Serial.print("Blue: "); Serial.println(average_blue_value);
  Serial.println();
  //delay(2000);
}
