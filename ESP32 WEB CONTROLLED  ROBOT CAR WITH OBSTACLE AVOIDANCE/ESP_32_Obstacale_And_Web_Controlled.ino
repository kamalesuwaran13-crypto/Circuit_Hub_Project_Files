#include <WiFi.h>
#include <WebServer.h>

// ========== WiFi AP Settings ==========
const char* apSsid     = "ESP32_Car";
const char* apPassword = "12345678";

// ========== Web server ==========
WebServer server(80);

// ========== Motor Pins (L298N) ==========
#define IN1 14   // Left motor
#define IN2 27
#define IN3 26   // Right motor
#define IN4 25

// PWM enable pins (connect L298N ENA -> ENA_PIN, ENB -> ENB_PIN)
#define ENA_PIN 33
#define ENB_PIN 32

// ledc PWM channels and settings
const int PWM_FREQ = 20000;   // 20 kHz
const int PWM_RES  = 8;       // 8-bit resolution (0-255)
const int CH_A = 0;           // channel for ENA
const int CH_B = 1;           // channel for ENB

// Speed values (0-255)
uint8_t manualSpeed = 230;    // manual control speed (high)
uint8_t autoSpeed   = 200;    // obstacle-avoidance speed (lower)

/* ---------- rest of pins ---------- */
// Ultrasonic Sensor
#define TRIG_PIN 4
#define ECHO_PIN 5

// ========== Control and Mode Flags ==========
bool forwardCmd = false;
bool backCmd    = false;
bool leftCmd    = false;
bool rightCmd   = false;

bool obstacleMode = false;   // false = manual, true = auto obstacle mode

// Obstacle avoiding state machine
enum AutoState {
  AUTO_IDLE,
  AUTO_FORWARD,
  AUTO_BACKWARD,
  AUTO_TURN
};

AutoState autoState = AUTO_IDLE;
unsigned long autoStateStart = 0;
bool turnLeftNext = true;   // alternate turn direction

// Timings (ms) for auto mode
const unsigned long BACK_TIME = 600;
const unsigned long TURN_TIME = 700;

// Distance threshold (cm)
const float OBSTACLE_DIST_CM = 20.0;

// ========== Simple HTML UI (Web Page) ==========
const char index_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8" />
  <title>ESP32 Web Car</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background: #111;
      color: #eee;
    }
    h1 {
      margin-top: 10px;
    }
    .btn {
      display: inline-block;
      padding: 18px 26px;
      margin: 8px;
      font-size: 18px;
      border-radius: 10px;
      border: none;
      cursor: pointer;
      min-width: 110px;
      background: #333;
      color: #fff;
    }
    .btn:active {
      transform: scale(0.97);
      background: #555;
    }
    .grid {
      margin-top: 25px;
    }
    .row {
      margin: 6px 0;
    }
    .mode-btn-on {
      background: #1b5e20;
    }
    .mode-btn-off {
      background: #b71c1c;
    }
  </style>
</head>
<body>
  <h1>ESP32 Web Controlled Car</h1>
  <p>Hold the direction buttons to move. Toggle Obstacle Mode for auto driving.</p>

  <div class="grid">
    <div class="row">
      <button class="btn" 
        onmousedown="sendCmd('f',1)" 
        onmouseup="sendCmd('f',0)"
        ontouchstart="sendCmd('f',1)" 
        ontouchend="sendCmd('f',0)">
        Forward
      </button>
    </div>
    <div class="row">
      <button class="btn"
        onmousedown="sendCmd('l',1)" 
        onmouseup="sendCmd('l',0)"
        ontouchstart="sendCmd('l',1)" 
        ontouchend="sendCmd('l',0)">
        Left
      </button>

      <button class="btn"
        onmousedown="sendCmd('s',1)" 
        onmouseup="sendCmd('s',0)"
        ontouchstart="sendCmd('s',1)" 
        ontouchend="sendCmd('s',0)">
        Stop
      </button>

      <button class="btn"
        onmousedown="sendCmd('r',1)" 
        onmouseup="sendCmd('r',0)"
        ontouchstart="sendCmd('r',1)" 
        ontouchend="sendCmd('r',0)">
        Right
      </button>
    </div>
    <div class="row">
      <button class="btn"
        onmousedown="sendCmd('b',1)" 
        onmouseup="sendCmd('b',0)"
        ontouchstart="sendCmd('b',1)" 
        ontouchend="sendCmd('b',0)">
        Backward
      </button>
    </div>
  </div>

  <h2>Obstacle Avoiding Mode</h2>
  <p>When ON, car moves forward and avoids obstacles automatically.</p>
  <div class="row">
    <button class="btn mode-btn-on" onclick="setMode(1)">Mode ON</button>
    <button class="btn mode-btn-off" onclick="setMode(0)">Mode OFF</button>
  </div>

  <script>
    function sendCmd(dir, state) {
      fetch(`/cmd?dir=${dir}&state=${state}`)
        .catch(e => console.log(e));
    }

    function setMode(auto) {
      fetch(`/mode?auto=${auto}`)
        .catch(e => console.log(e));
    }
  </script>
</body>
</html>)=====";

// ========== Motor Helper Functions with PWM ==========
void setMotorSpeed(uint8_t leftSpeed, uint8_t rightSpeed) {
  // leftSpeed -> ENA (CH_A), rightSpeed -> ENB (CH_B)
  ledcWrite(CH_A, leftSpeed);
  ledcWrite(CH_B, rightSpeed);
}

void driveStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  setMotorSpeed(0, 0);
}

void driveForward(uint8_t speed) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  setMotorSpeed(speed, speed);
}

void driveBackward(uint8_t speed) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  setMotorSpeed(speed, speed);
}

void driveLeft(uint8_t speed) {
  // left motor backward, right motor forward
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  // you can use different speeds for pivot if desired
  setMotorSpeed(speed, speed);
}

void driveRight(uint8_t speed) {
  // left motor forward, right motor backward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  setMotorSpeed(speed, speed);
}

// ========== Ultrasonic Distance Measurement ==========
float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return -1.0;
  float distance = (duration * 0.0343) / 2.0;
  return distance;
}

// ========== Manual Movement Handler ==========
void handleManualMovement() {
  if (forwardCmd) {
    driveForward(manualSpeed);
  } else if (backCmd) {
    driveBackward(manualSpeed);
  } else if (leftCmd) {
    driveLeft(manualSpeed);
  } else if (rightCmd) {
    driveRight(manualSpeed);
  } else {
    driveStop();
  }
}

// ========== Auto Obstacle Avoiding State Machine ==========
void handleAutoObstacle() {
  unsigned long now = millis();

  switch (autoState) {
    case AUTO_IDLE:
      driveStop();
      autoState = AUTO_FORWARD;
      break;

    case AUTO_FORWARD: {
      float d = getDistanceCm();
      if (d > 0 && d < OBSTACLE_DIST_CM) {
        driveStop();
        autoState = AUTO_BACKWARD;
        autoStateStart = now;
      } else {
        driveForward(autoSpeed); // use autoSpeed (slower)
      }
      break;
    }

    case AUTO_BACKWARD:
      if (now - autoStateStart >= BACK_TIME) {
        driveStop();
        autoState = AUTO_TURN;
        autoStateStart = now;
      } else {
        driveBackward(autoSpeed);
      }
      break;

    case AUTO_TURN:
      if (turnLeftNext) {
        driveLeft(autoSpeed);
      } else {
        driveRight(autoSpeed);
      }
      if (now - autoStateStart >= TURN_TIME) {
        driveStop();
        turnLeftNext = !turnLeftNext;
        autoState = AUTO_FORWARD;
      }
      break;
  }
}

// ========== HTTP Handlers ==========

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleCmd() {
  if (!server.hasArg("dir") || !server.hasArg("state")) {
    server.send(400, "text/plain", "Missing params");
    return;
  }

  String dir = server.arg("dir");
  int state  = server.arg("state").toInt();

  if (!obstacleMode) {
    bool pressed = (state != 0);
    if (dir == "f") {
      forwardCmd = pressed;
      if (pressed) { backCmd = leftCmd = rightCmd = false; }
    } else if (dir == "b") {
      backCmd = pressed;
      if (pressed) { forwardCmd = leftCmd = rightCmd = false; }
    } else if (dir == "l") {
      leftCmd = pressed;
      if (pressed) { forwardCmd = backCmd = rightCmd = false; }
    } else if (dir == "r") {
      rightCmd = pressed;
      if (pressed) { forwardCmd = backCmd = leftCmd = false; }
    } else if (dir == "s") {
      forwardCmd = backCmd = leftCmd = rightCmd = false;
      driveStop();
    }
  }

  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (!server.hasArg("auto")) {
    server.send(400, "text/plain", "Missing auto param");
    return;
  }

  int autoVal = server.arg("auto").toInt();
  if (autoVal == 1) {
    obstacleMode = true;
    forwardCmd = backCmd = leftCmd = rightCmd = false;
    autoState = AUTO_FORWARD;
    autoStateStart = millis();
  } else {
    obstacleMode = false;
    autoState = AUTO_IDLE;
    driveStop();
  }

  server.send(200, "text/plain", "Mode set");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

// ========== Setup & Loop ==========

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // PWM pins
  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);

  // setup ledc PWM channels
  ledcSetup(CH_A, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENA_PIN, CH_A);
  ledcSetup(CH_B, PWM_FREQ, PWM_RES);
  ledcAttachPin(ENB_PIN, CH_B);

  driveStop();

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  Serial.println();
  Serial.println("AP started");
  Serial.print("SSID: ");
  Serial.println(apSsid);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/mode", handleMode);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  if (obstacleMode) {
    handleAutoObstacle();
  } else {
    handleManualMovement();
  }

  delay(5);
}
