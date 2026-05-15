#include <SoftwareSerial.h>

// Bluetooth module pins (HC-05/06)
SoftwareSerial BT(10, 11); // TX, RX

// Motor driver pins
int ENA = 5;   // Left motor speed (PWM)
int ENB = 6;   // Right motor speed (PWM)
int IN1 = 7;   // Left motor forward
int IN2 = 8;   // Left motor backward
int IN3 = 9;   // Right motor forward
int IN4 = 4;   // Right motor backward

// Ultrasonic sensor pins
int trigPin = 2;
int echoPin = 3;

// Buzzer pin
int buzzer = 12;

// Variables
char command;   
int Speed = 180;   // Default motor speed

bool buzzerState = false;  
unsigned long lastBeepTime = 0;  

void setup() {
  Serial.begin(9600);
  BT.begin(9600);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);

  stopCar();
  noTone(buzzer);
}

void loop() {
  if (BT.available()) {
    command = BT.read();
    Serial.print("Command: ");
    Serial.println(command);

    handleCommand(command);
  }

  // Handle buzzer beeping (if obstacle in front)
  if (buzzerState) {
    unsigned long now = millis();
    if (now - lastBeepTime > 500) {   // 0.5 sec interval
      tone(buzzer, 1000, 200);        // 1000Hz tone, 200ms long
      lastBeepTime = now;
    }
  }
}

// ---------------------- Distance Function ----------------------
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);  // timeout 20ms

  if (duration == 0) {
    return 400; // No echo → assume no obstacle
  }

  int distance = duration * 0.034 / 2;
  if (distance > 400) distance = 400; // max limit
  return distance;
}

// ---------------------- Handle Command ----------------------
void handleCommand(char cmd) {
  int dist = getDistance();

  switch (cmd) {
    case 'F': // Forward
      if (dist > 15) {
        forward();
        buzzerState = false;
        noTone(buzzer);
      } else {
        stopCar();
        buzzerState = true;  // enable beep-beep mode
      }
      break;

    case 'B': // Backward
      backward();
      buzzerState = false;
      noTone(buzzer);
      break;

    case 'L': // Left
      left();
      buzzerState = false;
      noTone(buzzer);
      break;

    case 'R': // Right
      right();
      buzzerState = false;
      noTone(buzzer);
      break;

    case 'S': // Stop
      stopCar();
      buzzerState = false;
      noTone(buzzer);
      break;

    // Speed control (slider)
    case '0': Speed = 100; break;
    case '1': Speed = 140; break;
    case '2': Speed = 153; break;
    case '3': Speed = 165; break;
    case '4': Speed = 178; break;
    case '5': Speed = 191; break;
    case '6': Speed = 204; break;
    case '7': Speed = 216; break;
    case '8': Speed = 229; break;
    case '9': Speed = 242; break;
    case 'q': Speed = 255; break;
  }
}

// ---------------------- Motor Functions ----------------------
void forward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, Speed);
  analogWrite(ENB, Speed);
}

void backward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, Speed);
  analogWrite(ENB, Speed);
}

void left() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, Speed);
  analogWrite(ENB, Speed);
}

void right() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, Speed);
  analogWrite(ENB, Speed);
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
