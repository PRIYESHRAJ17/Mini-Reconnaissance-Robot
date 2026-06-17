#include <SoftwareSerial.h>
#include <Wire.h>
#include <MPU6050.h>

SoftwareSerial espSerial(2, 3);

#define ENA 5
#define IN1 6
#define IN2 7
#define IN3 8
#define IN4 9
#define ENB 10

#define IR_RIGHT 4
#define IR_LEFT  11
#define FLAME    12
#define SOUND    13
#define TRIG_R   A0
#define ECHO_R   A1
#define TRIG_L   A2
#define ECHO_L   A3

MPU6050 mpu;
int motorSpeed = 178; // default 70%
char currentDir = 'S';
bool obstacleBlocking = false;

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);
  Wire.begin();
  mpu.initialize();

  pinMode(ENA,OUTPUT); pinMode(ENB,OUTPUT);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  pinMode(IR_RIGHT,INPUT_PULLUP);
  pinMode(IR_LEFT, INPUT_PULLUP);
  pinMode(FLAME,   INPUT_PULLUP);
  pinMode(SOUND,   INPUT_PULLUP);
  pinMode(TRIG_R,OUTPUT); pinMode(ECHO_R,INPUT);
  pinMode(TRIG_L,OUTPUT); pinMode(ECHO_L,INPUT);
  stopMotors();
  Serial.println("Ready");
}

void loop() {
  static unsigned long lastUS = 0;
  static long distR = 0, distL = 0;
  static int irR = 1, irL = 1;

  // Read ultrasonics every 100ms only
  if (millis() - lastUS > 100) {
    lastUS = millis();
    distR = readUltrasonic(TRIG_R, ECHO_R);
    distL = readUltrasonic(TRIG_L, ECHO_L);
    irR   = digitalRead(IR_RIGHT);
    irL   = digitalRead(IR_LEFT);
    bool obsR = (distR > 0 && distR < 30);
    bool obsL = (distL > 0 && distL < 30);
    bool irBlocked = (irR == LOW || irL == LOW);
    obstacleBlocking = (obsR || obsL || irBlocked);
    if (obstacleBlocking && currentDir == 'F') stopMotors();
  }

  // Commands checked EVERY loop — instant
  while (espSerial.available()) {
    char cmd = espSerial.read();
    if (cmd == 'V') {
      delay(10);
      String num = "";
      while (espSerial.available()) {
        char c = espSerial.read();
        if (c >= '0' && c <= '9') num += c;
        else break;
      }
      if (num.length() > 0) {
        int spd = num.toInt();
        if (spd >= 25 && spd <= 255) {

  motorSpeed = spd;

  switch(currentDir){

    case 'F':
      moveForward();
      break;

    case 'B':
      moveBackward();
      break;

    case 'L':
      turnLeft();
      break;

    case 'R':
      turnRight();
      break;
  }
}
      }
    } else {
      executeCommand(cmd);
    }
  }

  static unsigned long lastSensor = 0;
  if (millis() - lastSensor > 500) {
    lastSensor = millis();
    sendSensorData(distR, distL, irR, irL);
  }
}

void executeCommand(char cmd) {

  currentDir = cmd;

  switch(cmd){

    case 'F':

      if(!obstacleBlocking)
        moveForward();

      break;

    case 'B':

      moveBackward();

      break;

    case 'L':

      turnLeft();

      break;

    case 'R':

      turnRight();

      break;

    default:

      stopMotors();

      currentDir='S';

      break;
  }
}

void moveForward()  {
  analogWrite(ENA,motorSpeed);
  analogWrite(ENB,motorSpeed);
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,HIGH);
}
void moveBackward() {
  analogWrite(ENA,motorSpeed);
  analogWrite(ENB,motorSpeed);
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
}
void turnLeft() {
  analogWrite(ENA,motorSpeed);
  analogWrite(ENB,motorSpeed);
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,HIGH);
}
void turnRight() {
  analogWrite(ENA,motorSpeed);
  analogWrite(ENB,motorSpeed);
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
}
void stopMotors()   { analogWrite(ENA,0); analogWrite(ENB,0); digitalWrite(IN1,LOW); digitalWrite(IN2,LOW); digitalWrite(IN3,LOW); digitalWrite(IN4,LOW); }

long readUltrasonic(int trig, int echo) {
  digitalWrite(trig,LOW); delayMicroseconds(2);
  digitalWrite(trig,HIGH); delayMicroseconds(10);
  digitalWrite(trig,LOW);
  long d = pulseIn(echo,HIGH,20000);
  return d * 0.034 / 2;
}

void sendSensorData(long distR, long distL, int irR, int irL) {
  int flame = digitalRead(FLAME);
  int sound = digitalRead(SOUND);
  int16_t ax,ay,az,gx,gy,gz;
  mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz);
  espSerial.print("{\"dr\":"); espSerial.print(distR);
  espSerial.print(",\"dl\":"); espSerial.print(distL);
  espSerial.print(",\"fl\":"); espSerial.print(flame==LOW?1:0);
  espSerial.print(",\"sn\":"); espSerial.print(sound==LOW?1:0);
  espSerial.print(",\"irR\":"); espSerial.print(irR);
  espSerial.print(",\"irL\":"); espSerial.print(irL);
  espSerial.print(",\"ax\":"); espSerial.print(ax);
  espSerial.print(",\"ay\":"); espSerial.print(ay);
  espSerial.print(",\"ob\":"); espSerial.print(obstacleBlocking?1:0);
  espSerial.println("}");
} 