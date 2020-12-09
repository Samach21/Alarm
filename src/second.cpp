#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>

Servo myservo1;
Servo myservo2;

int x;

void setup() {
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Serial.begin(115200);
  myservo1.attach(3);
  myservo2.attach(4);
  myservo1.write(90);
  myservo2.write(160);
}

void loop() {
  if (x == 1) {
    myservo2.write(90);
    delay(1500);
    myservo2.write(160);
    delay(1500);
    myservo1.write(0);
    delay(1500);
    myservo1.write(90);
    x = 0;
  }
}

void receiveEvent(int howMany) {
  while (1 < Wire.available()) { 
    char c = Wire.read(); 
    Serial.print(c);
  }
  x = Wire.read();
  Serial.println(x);
}