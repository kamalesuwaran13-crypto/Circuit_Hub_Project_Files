#include <WiFi.h>
#include <WebServer.h>

// ================= WiFi =================
const char* apSsid = "ESP32_Car";
const char* apPassword = "12345678";
WebServer server(80);

// ================= L298N =================
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25
#define ENA_PIN 33
#define ENB_PIN 32

// ================= PWM =================
const int PWM_FREQ = 20000;
const int PWM_RES  = 8;
const int CH_A = 0;
const int CH_B = 1;

// ================= Speed =================
uint8_t manualSpeed = 230;
uint8_t autoSpeed   = 200;

// ================= Ultrasonic =================
#define TRIG_PIN 4
#define ECHO_PIN 5

// ================= Flags =================
bool forwardCmd = false;
bool backCmd    = false;
bool leftCmd    = false;
bool rightCmd   = false;

bool obstacleMode = false;
bool followMode   = false;

// ================= Auto Obstacle =================
enum AutoState { AUTO_IDLE, AUTO_FORWARD, AUTO_BACKWARD, AUTO_TURN };
AutoState autoState = AUTO_IDLE;

unsigned long autoStateStart = 0;
bool turnLeftNext = true;

const unsigned long BACK_TIME = 600;
const unsigned long TURN_TIME = 700;
const float OBSTACLE_DIST_CM = 20.0;

// ================= HTML =================
const char index_html[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{background:#111;color:#fff;font-family:Arial;text-align:center}
.btn{padding:16px 26px;margin:6px;border:none;border-radius:10px;background:#333;color:#fff;font-size:16px}
.btn:active{background:#555}
</style>
</head>
<body>

<h2>ESP32 Web Robot Car</h2>

<button class="btn" onmousedown="cmd('f',1)" onmouseup="cmd('f',0)" ontouchstart="cmd('f',1)" ontouchend="cmd('f',0)">Forward</button><br>

<button class="btn" onmousedown="cmd('l',1)" onmouseup="cmd('l',0)" ontouchstart="cmd('l',1)" ontouchend="cmd('l',0)">Left</button>
<button class="btn" onclick="cmd('s',1)">Stop</button>
<button class="btn" onmousedown="cmd('r',1)" onmouseup="cmd('r',0)" ontouchstart="cmd('r',1)" ontouchend="cmd('r',0)">Right</button><br>

<button class="btn" onmousedown="cmd('b',1)" onmouseup="cmd('b',0)" ontouchstart="cmd('b',1)" ontouchend="cmd('b',0)">Backward</button>

<h3>Obstacle Avoid Mode</h3>
<button class="btn" onclick="mode(1)">ON</button>
<button class="btn" onclick="mode(0)">OFF</button>

<h3>Follow Mode</h3>
<button class="btn" onclick="follow(1)">FOLLOW ON</button>
<button class="btn" onclick="follow(0)">FOLLOW OFF</button>

<script>
function cmd(d,s){ fetch(`/cmd?dir=${d}&state=${s}`); }
function mode(v){ fetch(`/mode?auto=${v}`); }
function follow(v){ fetch(`/follow?f=${v}`); }
</script>

</body>
</html>)=====";

// ================= Motor =================
void setMotorSpeed(uint8_t l, uint8_t r){
  ledcWrite(CH_A, l);
  ledcWrite(CH_B, r);
}

void driveStop(){
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
  setMotorSpeed(0,0);
}

void driveForward(uint8_t s){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  setMotorSpeed(s,s);
}

void driveBackward(uint8_t s){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  setMotorSpeed(s,s);
}

void driveLeft(uint8_t s){
  digitalWrite(IN1,LOW); digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);
  setMotorSpeed(s,s);
}

void driveRight(uint8_t s){
  digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,HIGH);
  setMotorSpeed(s,s);
}

// ================= Ultrasonic =================
float getDistanceCm(){
  digitalWrite(TRIG_PIN,LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN,LOW);
  unsigned long d=pulseIn(ECHO_PIN,HIGH,30000);
  if(d==0) return -1;
  return d*0.0343/2;
}

// ================= Manual =================
void handleManual(){
  if(forwardCmd) driveForward(manualSpeed);
  else if(backCmd) driveBackward(manualSpeed);
  else if(leftCmd) driveLeft(manualSpeed);
  else if(rightCmd) driveRight(manualSpeed);
  else driveStop();
}

// ================= Obstacle =================
void handleObstacle(){
  unsigned long now=millis();
  switch(autoState){
    case AUTO_IDLE: autoState=AUTO_FORWARD; break;
    case AUTO_FORWARD:{
      float d=getDistanceCm();
      if(d>0 && d<OBSTACLE_DIST_CM){
        driveStop();
        autoState=AUTO_BACKWARD;
        autoStateStart=now;
      } else driveForward(autoSpeed);
    }break;
    case AUTO_BACKWARD:
      if(now-autoStateStart>=BACK_TIME){
        autoState=AUTO_TURN;
        autoStateStart=now;
      } else driveBackward(autoSpeed);
      break;
    case AUTO_TURN:
      turnLeftNext?driveLeft(autoSpeed):driveRight(autoSpeed);
      if(now-autoStateStart>=TURN_TIME){
        driveStop();
        turnLeftNext=!turnLeftNext;
        autoState=AUTO_FORWARD;
      }
      break;
  }
}

// ================= Follow =================
void handleFollow(){
  float d=getDistanceCm();
  if(d<0){ driveStop(); return; }
  if(d<12) driveBackward(autoSpeed);
  else if(d<=20) driveStop();
  else if(d<=45) driveForward(autoSpeed);
  else driveStop();
}

// ================= HTTP =================
void handleRoot(){ server.send_P(200,"text/html",index_html); }

void handleCmd(){
  if(obstacleMode||followMode){
    server.send(200,"text/plain","IGNORED");
    return;
  }
  String d=server.arg("dir");
  bool p=server.arg("state").toInt();

  if(d=="f") forwardCmd=p;
  else if(d=="b") backCmd=p;
  else if(d=="l") leftCmd=p;
  else if(d=="r") rightCmd=p;
  else if(d=="s"){
    forwardCmd=backCmd=leftCmd=rightCmd=false;
    driveStop();
  }
  server.send(200,"text/plain","OK");
}

void handleMode(){
  obstacleMode=server.arg("auto").toInt();
  followMode=false;
  autoState=AUTO_FORWARD;
  forwardCmd=backCmd=leftCmd=rightCmd=false;
  driveStop();
  server.send(200,"text/plain","OK");
}

void handleFollowHttp(){
  followMode=server.arg("f").toInt();
  obstacleMode=false;
  forwardCmd=backCmd=leftCmd=rightCmd=false;
  driveStop();
  server.send(200,"text/plain","OK");
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);

  ledcSetup(CH_A,PWM_FREQ,PWM_RES);
  ledcAttachPin(ENA_PIN,CH_A);
  ledcSetup(CH_B,PWM_FREQ,PWM_RES);
  ledcAttachPin(ENB_PIN,CH_B);

  pinMode(TRIG_PIN,OUTPUT);
  pinMode(ECHO_PIN,INPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid,apPassword);

  server.on("/",handleRoot);
  server.on("/cmd",handleCmd);
  server.on("/mode",handleMode);
  server.on("/follow",handleFollowHttp);
  server.begin();
}

// ================= LOOP =================
void loop(){
  server.handleClient();
  if(followMode) handleFollow();
  else if(obstacleMode) handleObstacle();
  else handleManual();
  delay(5);
}
