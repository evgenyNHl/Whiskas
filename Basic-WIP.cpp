#include <Arduino.h>

int motorOneSpeed = 0;
int motorTwoSpeed = 0;

//Motor Number, Speed is a percentage between 0 and 1, forward yes no, distance
void motorSpeedAdjuster (int, float, boolean);
//stop
void stop (void);
//RPM
void rpmMachine (void);
//wait
long howLong(void);

//A1 and B1 are speed
//A2 and B2 are direction

//motor 1 is left, motor 2 is right

int motorSpeed1 = 10;
int motorSpeed2 = 7;
int motorSpeedReverse1 = 9;
int motorSpeedReverse2 = 6;
int motor1 = 9;
int motor2 = 6;

int speedcheck1 = 2;
int speedcheck2 = 3;

float RPM = 0;

long timer;

void setup() {
  attachInterrupt(digitalPinToInterrupt(speedcheck1), rpmMachine, INPUT_PULLUP);

  pinMode(motorSpeed1, OUTPUT);
  pinMode(motorSpeedReverse1, OUTPUT);
  pinMode(motorSpeed2, OUTPUT);
  pinMode(motorSpeedReverse2, OUTPUT);

  pinMode(speedcheck1, INPUT);
  pinMode(speedcheck2, INPUT);
  stop();
}

void loop() {
  howLong();
  while (millis() <= timer + 1000){
    motorSpeedAdjuster(motor1, .76, true);
    motorSpeedAdjuster(motor2, .75, true);
  }
  howLong();
  while (millis() <= timer + 1000){
    motorSpeedAdjuster(motor1, .76, false);
    motorSpeedAdjuster(motor2, .75, false);
  }
  howLong();
  while (millis() <= timer + 1000){
    stop();
  }
  howLong();
  while (millis() <= timer + 1000){
    motorSpeedAdjuster(motor1, .75, true);
    motorSpeedAdjuster(motor2, .50, true);
  }
  howLong();
  while (millis() <= timer + 1000){
    stop();
  }
  howLong();
  while (millis() <= timer + 1000){
    motorSpeedAdjuster(motor1, .50, true);
    motorSpeedAdjuster(motor2, .75, true);
  }
  howLong();
  while (millis() <= timer + 1000){ 
    stop();
  }
}

// put function definitions here:
void motorSpeedAdjuster (int Number, float Speed, boolean Forward) {
  int AnalogSpeed = 255 * Speed;
  
    if(Forward){
      analogWrite(Number, 0);
      analogWrite(Number + 1, AnalogSpeed);
    }

    else{
      analogWrite(Number + 1, 0);
      analogWrite(Number, AnalogSpeed);
    }
}

long howLong(void){
  if(millis() >= timer){
    timer = millis();
  }
}

void stop (void){
  analogWrite(motorSpeed1, 0);
  analogWrite(motorSpeedReverse1, 0);
  analogWrite(motorSpeed2, 0);
  analogWrite(motorSpeedReverse2, 0);
}

void rpmMachine (void){
  static long RPMTimer;
  static long RPMTimer2;

  
}
