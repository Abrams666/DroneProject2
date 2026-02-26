//import
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Servo.h>
#include <MPU9250.h>
#include <Wire.h>

//const
#define CE_PIN 7
#define CSN_PIN 8
uint8_t address[][8] = { "droneRx","droneTx" };
#define motorFRPin A0
#define motorFLPin A1
#define motorBRPin A2
#define motorBLPin A3
const int P = 10;
const int yawSpeed = 100;
struct RxDataType {
  int targetPitch;
  int targetRoll;
  int targetThrust;
  int yawDir;
  bool stop;
  bool isReceive;
};
struct TxDataType {
  int FR;
  int FL;
  int BR;
  int BL;
  int pitch;
  int roll;
  int heading;
};

//config
RF24 radio(CE_PIN, CSN_PIN);
MPU9250 mpu;
Servo motorFR;
Servo motorFL;
Servo motorBR;
Servo motorBL;

//val
bool comType = true;
int stop = 1;
int targetThrust = 0;
int targetPitch = 0;
int targetRoll = 0;
int yawDir = 1;
float currentPitch = 0;
float currentRoll = 0;
float currentHeading = 0;
RxDataType payloadRx;
TxDataType payloadTx;
float mainSpeed = 0;
int motorFRSpeed = 0;
int motorFLSpeed = 0;
int motorBRSpeed = 0;
int motorBLSpeed = 0;
float motorFRSpeedAddon = 0;
float motorFLSpeedAddon = 0;
float motorBRSpeedAddon = 0;
float motorBLSpeedAddon = 0;
float motorFRSpeedAddonAcc = 0;
float motorFLSpeedAddonAcc = 0;
float motorBRSpeedAddonAcc = 0;
float motorBLSpeedAddonAcc = 0;
unsigned long lastReciveTime = micros();

//func
void tx(TxDataType payload) {
  unsigned long start_timer = micros();
  bool report = radio.write(&payload, sizeof(TxDataType));
  unsigned long end_timer = micros();

  if (report) {
    // Serial.print(F("Transmission successful! "));
    // Serial.print(F("Time to transmit = "));
    // Serial.print(end_timer - start_timer);
    // Serial.print(F(" us. Sent: "));
    // Serial.println(payload);
  } else {
    Serial.println(F("Transmission failed or timed out"));
  }
}

RxDataType rx() {
  RxDataType payload;

  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&payload, bytes);
    // Serial.print(F("Received "));
    // Serial.print(bytes);
    // Serial.print(F(" bytes on pipe "));
    // Serial.print(pipe);
    // Serial.print(F(": "));
    // Serial.println(payload);
  }else{
    payload.isReceive = false;
  }

  return payload;
}

int checkSpeed(int speed) {
  if(speed > 2000){
    speed = 2000;
  }else if(speed < 1000){
    speed = 1000;
  }

  return speed;
}

//setup
void setup() {
  Serial.begin(9600);
  printf_begin();
  Wire.begin();

  //setup mpu9250
  if (!mpu.setup(0x68))
  {
    Serial.println("MPU9250初始化失敗!");  
    while (1){};  
  }

  //setup nrf24l01
  Serial.println("Waiting USB Connect...");
  while (!Serial) {
  }
  Serial.println("Connected");

  Serial.println("Initializing the transceiver on the SPI bus...");
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}
  }
  Serial.println("Succeed");

  radio.setPALevel(RF24_PA_MAX);
  radio.setPayloadSize(sizeof(TxDataType));
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  radio.stopListening();
  
  //setup motor
  motorFR.attach(motorFRPin);
  motorFL.attach(motorFLPin);
  motorBR.attach(motorBRPin);
  motorBL.attach(motorBLPin);
}

//start
void loop() {
  //rx
  radio.startListening();
  payloadRx = rx();
  if(!payloadRx.isReceive){
    if(micros() - lastReciveTime >= 50000){
      stop = 1;
    }
  }else{
    lastReciveTime = micros();

    yawDir = payloadRx.yawDir;
    targetRoll = payloadRx.targetRoll;
    targetPitch = payloadRx.targetPitch;
    targetThrust = payloadRx.targetThrust;
    stop = payloadRx.stop;
  }

  //MPU9250
  if(mpu.update()){
    currentRoll = float(mpu.getRoll());
    currentPitch = float(mpu.getPitch());
    currentHeading = float(mpu.getYaw());
  }

  //apply control
  if(stop == 1){
    motorFR.writeMicroseconds(1000);
    motorFL.writeMicroseconds(1000);
    motorBR.writeMicroseconds(1000);
    motorBL.writeMicroseconds(1000);

    while(1) {}
  }

  int pitchAcc = (currentPitch - float(targetPitch)) * P;
  int rollAcc = (currentRoll - float(targetRoll)) * P;
  mainSpeed = (targetThrust * 8) + 1000;

  motorFRSpeedAddonAcc = (-pitchAcc) + rollAcc;
  motorFLSpeedAddonAcc = (-pitchAcc) + (-rollAcc);
  motorBRSpeedAddonAcc = pitchAcc + rollAcc;
  motorBLSpeedAddonAcc = pitchAcc + (-rollAcc);

  motorFRSpeedAddon = motorFRSpeedAddonAcc;
  motorFLSpeedAddon = motorFLSpeedAddonAcc;
  motorBRSpeedAddon = motorBRSpeedAddonAcc;
  motorBLSpeedAddon = motorBLSpeedAddonAcc;

  motorFRSpeed = int(mainSpeed + motorFRSpeedAddon);
  motorFLSpeed = int(mainSpeed + motorFLSpeedAddon);
  motorBRSpeed = int(mainSpeed + motorBRSpeedAddon);
  motorBLSpeed = int(mainSpeed + motorBLSpeedAddon);

  if(yawDir == 0){
  }else if(yawDir == 1){
    motorFRSpeed += yawSpeed;
    motorBLSpeed += yawSpeed;
    motorFLSpeed -= yawSpeed;
    motorBRSpeed -= yawSpeed;
  }else if(yawDir == -1){
    motorFRSpeed -= yawSpeed;
    motorBLSpeed -= yawSpeed;
    motorFLSpeed += yawSpeed;
    motorBRSpeed += yawSpeed;
  }

  motorFRSpeed = checkSpeed(motorFRSpeed);
  motorFLSpeed = checkSpeed(motorFLSpeed);
  motorBRSpeed = checkSpeed(motorBRSpeed);
  motorBLSpeed = checkSpeed(motorBLSpeed);

  motorFR.writeMicroseconds(motorFRSpeed);
  motorFL.writeMicroseconds(motorFLSpeed);
  motorBR.writeMicroseconds(motorBRSpeed);
  motorBL.writeMicroseconds(motorBLSpeed);

  //tx
  payloadTx.FR = motorFRSpeed;
  payloadTx.FL = motorFLSpeed;
  payloadTx.BR = motorBRSpeed;
  payloadTx.BL = motorBLSpeed;
  payloadTx.pitch = int(currentPitch);
  payloadTx.roll = int(currentRoll);
  payloadTx.heading = int(currentHeading);

  radio.stopListening();
  tx(payloadTx);

  //delay
  delay(10);
}
