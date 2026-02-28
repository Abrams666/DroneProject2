//import
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <Servo.h>
#include <MPU9250.h>
#include <Wire.h>

//structure
struct DroneStatus {
  float pitch;
  float roll;
  float yaw;
  float a[3];
  float v[3];
  unsigned long lastTime;
};
struct RxDataType {
  DroneStatus status;
  bool stop;
  bool isReceive;
};
struct TxDataType {
  DroneStatus status;
  int speed[4];
  bool isReceive;
};

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
const float kp = 10;
const float ki = 0;
const float kd = 0;

//config
RF24 radio(CE_PIN, CSN_PIN);
MPU9250 mpu;
Servo motorFR;
Servo motorFL;
Servo motorBR;
Servo motorBL;

//val
int stop = 1;
DroneStatus targetDroneStatus;
DroneStatus currentDroneStatus;
RxDataType payloadRx;
TxDataType payloadTx;
int targetSpeed[4];
int currentSpeed[4];
float error[3];
float pid[3];
float integral[3];
float lastErr[3];
unsigned long lastReciveTime = micros();
unsigned long lastTime = micros();

//func
DroneStatus getStatus(DroneStatus currentDroneStatus){
  DroneStatus sta = currentDroneStatus;

  if(mpu.update()){
    unsigned long currentTime = micros();
    unsigned long dt = (currentTime - sta.lastTime)/1000000;
    if(dt <= 0) dt = 0.000001;

    sta.pitch = float(mpu.getPitch());
    sta.roll = float(mpu.getRoll());
    sta.yaw = float(mpu.getYaw());
    sta.a[0] = float(mpu.getAccX());
    sta.a[1] = float(mpu.getAccY());
    sta.a[2] = float(mpu.getAccZ());
    sta.v[0] += sta.a[0] * dt;
    sta.v[1] += sta.a[1] * dt;
    sta.v[2] += sta.a[2] * dt;
    sta.lastTime = currentTime;
  }

  return sta;
}

RxDataType commute(TxDataType payloadTx){
  RxDataType payloadRx;
  if(radio.available()){
    radio.read(&payloadRx, sizeof(payloadRx));
    Serial.print(F("Received"));
    radio.writeAckPayload(1, &payloadTx, sizeof(payloadTx));
  }else{
    payloadRx.isReceive = false;
    Serial.print(F("Not Received"));
  }

  return payloadRx;
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
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.startListening();
  
  //setup motor
  motorFR.attach(motorFRPin);
  motorFL.attach(motorFLPin);
  motorBR.attach(motorBRPin);
  motorBL.attach(motorBLPin);
}

//start
void loop() {
  //MPU9250
  currentDroneStatus = getStatus(currentDroneStatus);

  //commute
  payloadTx.status = currentDroneStatus;
  payloadTx.speed[0] = currentSpeed[0];
  payloadTx.speed[1] = currentSpeed[1];
  payloadTx.speed[2] = currentSpeed[2];
  payloadTx.speed[3] = currentSpeed[3];
  payloadTx.isReceive = true;

  payloadRx = commute(payloadTx);

  if(!payloadRx.isReceive){
    if(micros() - lastReciveTime >= 5000000){
      stop = 0;
    }
  }else{
    lastReciveTime = micros();
    stop = payloadRx.stop;
  }

  //apply control
  if(stop == 1){
    motorFR.writeMicroseconds(1000);
    motorFL.writeMicroseconds(1000);
    motorBR.writeMicroseconds(1000);
    motorBL.writeMicroseconds(1000);

    while(1) {}
  }

  unsigned long currentTime = micros();
  unsigned long dt = (currentTime - lastTime) / 1000000;
  if(dt <= 0) dt = 0.000001;
  lastTime = currentTime;
  for(int i = 0; i < 3; i++){
    error[i] = payloadRx.status.v[i] - currentDroneStatus.v[i];
    integral[i] += error[i] * dt;
    pid[i] = (kp * error[i]) + (ki * integral[i]) + (kd * ((error[i] - lastErr[i]) / dt));
    lastErr[i] = error[i];
  }

  currentSpeed[0] = (-pid[0]) + (-pid[1]) + (pid[2]);
  currentSpeed[1] = (-pid[0]) + (pid[1]) + (pid[2]);
  currentSpeed[2] = (pid[0]) + (pid[1]) + (pid[2]);
  currentSpeed[3] = (pid[0]) + (-pid[1]) + (pid[2]);

  if(payloadRx.status.yaw == 0){
  }else if(payloadRx.status.yaw == 1){
    currentSpeed[0] += yawSpeed;
    currentSpeed[1] -= yawSpeed;
    currentSpeed[2] += yawSpeed;
    currentSpeed[3] -= yawSpeed;
  }else if(payloadRx.status.yaw == -1){
    currentSpeed[0] -= yawSpeed;
    currentSpeed[1] += yawSpeed;
    currentSpeed[2] -= yawSpeed;
    currentSpeed[3] += yawSpeed;
  }

  for(int i = 0; i < 4; i++){
    currentSpeed[i] = checkSpeed(currentSpeed[i]);
  }

  motorFL.writeMicroseconds(currentSpeed[0]);
  motorFR.writeMicroseconds(currentSpeed[1]);
  motorBR.writeMicroseconds(currentSpeed[2]);
  motorBL.writeMicroseconds(currentSpeed[3]);

  //delay
  delay(10);
}
