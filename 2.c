#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ================================
// NEOPIXEL
// ================================
#define PIN 13
#define NUM_PIXELS 4

Adafruit_NeoPixel pixels(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

// ================================
// ULTRASONIC SENSOR HC-SR04
// ================================
const int trigPin = 2;
const int echoPin = 7;

const int OBSTACLE_DISTANCE = 13; // cm
unsigned long lastUltrasonicCheck = 0;
const unsigned long ULTRASONIC_INTERVAL = 100;

// ================================
// MOTORS
// ================================
const int motorPin1 = 3;
const int motorPin2 = 5;
const int motorPin3 = 6;
const int motorPin4 = 9;

// ================================
// GRIPPER
// ================================
const int gripperPin = 10;

// ================================
// LINE SENSORS
// ================================
const int sensorCount = 8;
const int sensorPins[sensorCount] = {A0, A1, A2, A3, A4, A5, A6, A7};
int sensorValues[sensorCount];

const int BLACK = 850;

// ================================
// STATE
// ================================
bool inTurn = false;
int lastDirection = 0; // 1 = right, -1 = left, 0 = forward
int rightDetectCount = 0;
int turnCommitCount = 0;

// ================================
// BLACK SQUARE DETECTION
// ================================
unsigned long allBlackStartTime = 0;
const unsigned long BLACK_SQUARE_THRESHOLD = 135;
const int MIN_BLACK_SENSORS = 6;

bool finishDetected = false;

// ================================
// PROTOTYPES
// ================================
void readSensors();
void handleNavigation();

void moveForward();
void moveBackward();
void moveLeft();
void moveRight();
void stopRobot();
void Uturn();

void startSequence();
void finishSequence();

long getDistance();
bool checkObstacle();

void setGripperPulse(int pulseWidthMicros);
void closeGripper();
void openGripper();

// ================================
// SETUP
// ================================
void setup() {
  // Configure motor pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  // Configure gripper pin as output
  pinMode(gripperPin, OUTPUT);

  // Configure ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Configure all line sensor pins as inputs
  for (int i = 0; i < sensorCount; i++) {
    pinMode(sensorPins[i], INPUT);
  }

  // Initialize NeoPixel LEDs
  pixels.begin();
  pixels.setBrightness(40);
  pixels.show();

  // Open gripper and start initial movement sequence
  openGripper();
  startSequence();
}

// ================================
// LOOP
// ================================
void loop() {
  // Stop execution if finish line detected
  if (finishDetected) {
    return;
  }

  // Read all line sensor values
  readSensors();

  // Check for obstacles periodically
  unsigned long currentTime = millis();
  if (currentTime - lastUltrasonicCheck >= ULTRASONIC_INTERVAL) {
    lastUltrasonicCheck = currentTime;

    if (checkObstacle()) {
      // Obstacle detected - stop and perform U-turn
      stopRobot();
      delay(200);

      Uturn();
      delay(800);

      // Reset navigation state
      inTurn = false;
      rightDetectCount = 0;
      turnCommitCount = 0;
      lastDirection = 0;
      return;
    }
  }

  // Process line following logic
  handleNavigation();
}

// ================================
// ULTRASONIC
// ================================
long getDistance() {
  // Send ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Measure echo duration and calculate distance in cm
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

bool checkObstacle() {
  long distance = getDistance();

  // Return true if obstacle is within threshold distance
  if (distance > 0 && distance < OBSTACLE_DISTANCE && distance < 300) {
    pixels.fill(pixels.Color(255, 0, 0)); // Red warning
    pixels.show();
    return true;
  }
  return false;
}

// ================================
// SENSOR READ
// ================================
void readSensors() {
  for (int i = 0; i < sensorCount; i++) {
    sensorValues[i] = analogRead(sensorPins[i]);
  }
}

// ================================
// NAVIGATION (RIGHT-HAND RULE)
// ================================
void handleNavigation() {
  // Count sensors detecting black surface
  int blackSensorCount = 0;

  for (int i = 0; i < sensorCount; i++) {
    if (sensorValues[i] > BLACK) {
      blackSensorCount++;
    }
  }

  // Check for finish line (black square detection)
  if (blackSensorCount >= MIN_BLACK_SENSORS) {
    if (allBlackStartTime == 0) {
      allBlackStartTime = millis();
    } else if (millis() - allBlackStartTime >= BLACK_SQUARE_THRESHOLD) {
      finishDetected = true;
      finishSequence();
      return;
    }
  } else {
    allBlackStartTime = 0;
  }

  // Determine line position based on sensor groups
  bool right  = sensorValues[0] > BLACK || sensorValues[1] > BLACK || sensorValues[2] > BLACK;
  bool center = sensorValues[3] > BLACK || sensorValues[4] > BLACK;
  bool left   = sensorValues[5] > BLACK || sensorValues[6] > BLACK || sensorValues[7] > BLACK;

  bool anyLine = right || center || left;

  // Handle turning state
  if (inTurn) {
    turnCommitCount++;

    // Commit to turn for minimum duration
    if (turnCommitCount < 20) {
      if (lastDirection == 1) moveRight();
      else if (lastDirection == -1) moveLeft();
      return;
    }

    // Exit turn when centered on line
    if (center && !right && !left) {
      inTurn = false;
      rightDetectCount = 0;
      turnCommitCount = 0;
      lastDirection = 0;
      moveForward();
    } else {
      if (lastDirection == 1) moveRight();
      else if (lastDirection == -1) moveLeft();
    }
    return;
  }

  // Right-hand rule: prioritize right turns
  if (right) {
    rightDetectCount++;
    if (rightDetectCount >= 2) {
      inTurn = true;
      lastDirection = 1;
      moveRight();
      return;
    }
  } else {
    rightDetectCount = 0;
  }

  // Follow center line
  if (center) {
    moveForward();
    lastDirection = 0;
    return;
  }

  // Turn left if line on left side
  if (left) {
    inTurn = true;
    lastDirection = -1;
    moveLeft();
    return;
  }

  // No line detected - perform U-turn
  if (!anyLine) {
    Uturn();
  }
}

// ================================
// MOVEMENT
// ================================
void moveForward() {
  analogWrite(motorPin1, 190);
  digitalWrite(motorPin2, LOW);
  analogWrite(motorPin3, 234);
  digitalWrite(motorPin4, LOW);

  pixels.clear();
  pixels.setPixelColor(2, pixels.Color(255, 255, 255));
  pixels.setPixelColor(3, pixels.Color(255, 255, 255));
  pixels.show();
}

void moveBackward() {
  digitalWrite(motorPin1, LOW);
  analogWrite(motorPin2, 220);
  digitalWrite(motorPin3, LOW);
  analogWrite(motorPin4, 255);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 255, 0));
  pixels.setPixelColor(1, pixels.Color(0, 255, 0));
  pixels.show();
}

void moveLeft() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  analogWrite(motorPin3, 255);
  digitalWrite(motorPin4, LOW);

  pixels.clear();
  pixels.setPixelColor(3, pixels.Color(255, 0, 0));
  pixels.show();
}

void moveRight() {
  analogWrite(motorPin1, 255);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);

  pixels.clear();
  pixels.setPixelColor(2, pixels.Color(255, 0, 0));
  pixels.show();
}

void Uturn() {
  analogWrite(motorPin1, 255);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  analogWrite(motorPin4, 255);

  pixels.fill(pixels.Color(255, 255, 0));
  pixels.show();
}

void stopRobot() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);

  pixels.clear();
  pixels.show();
}

// ================================
// FINISH
// ================================
void finishSequence() {
  stopRobot();
  delay(300);

  for (int i = 0; i < 3; i++) {
    pixels.fill(pixels.Color(255, 0, 0));
    pixels.show();
    delay(200);
    pixels.clear();
    pixels.show();
    delay(200);
  }

  moveLeft();
  delay(175);
  stopRobot();

  openGripper();
  delay(500);

  moveBackward();
  delay(2000);

  stopRobot();
  pixels.fill(pixels.Color(0, 0, 255));
  pixels.show();
}

// ================================
// GRIPPER
// ================================
void setGripperPulse(int pulseWidthMicros) {
  for (int i = 0; i < 25; i++) {
    digitalWrite(gripperPin, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(gripperPin, LOW);
    delayMicroseconds(20000 - pulseWidthMicros);
  }
}

void closeGripper() {
  setGripperPulse(1100);
}

void openGripper() {
  setGripperPulse(1900);
}

// ================================
// START
// ================================
void startSequence() {
  delay(2000);
  moveForward();
  delay(1500);
  stopRobot();

  closeGripper();

  moveForward();
  delay(250);
  stopRobot();

  moveLeft();
  delay(900);
  stopRobot();

  moveForward();
}