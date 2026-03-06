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
};
struct RxDataType {
  DroneStatus status;
  float basicThrust;
  bool stop;
};
struct TxDataType {
  DroneStatus status;
  int speed[4];
};

//const
#define CE_PIN 7
#define CSN_PIN 8
uint8_t address[][8] = { "droneRx","droneTx" };
#define motorFRPin A0
#define motorFLPin A1
#define motorBRPin A2
#define motorBLPin A3
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
bool isReceive = true;
float basicThrust = 1000;
bool isReady = false;
float pitchErr = 0;
float rollErr = 0;

//func
void sstop(){
  motorFR.writeMicroseconds(0);
  motorFL.writeMicroseconds(0);
  motorBR.writeMicroseconds(0);
  motorBL.writeMicroseconds(0);

  while(1) {}
}

DroneStatus getStatus(DroneStatus currentDroneStatus){
  DroneStatus sta = currentDroneStatus;

  if(mpu.update()){
    if(isReady){
      sta.pitch = float(mpu.getPitch()) - pitchErr;
      sta.roll = float(mpu.getRoll()) - rollErr;
    }else{
      sta.pitch = float(mpu.getPitch());
      sta.roll = float(mpu.getRoll());
    }
    sta.yaw = float(mpu.getYaw());
  }

  return sta;
}

RxDataType commute(TxDataType payloadTx){
  RxDataType payloadRx;
  if(radio.available()){
    radio.read(&payloadRx, sizeof(payloadRx));
    isReceive = true;
    radio.writeAckPayload(1, &payloadTx, sizeof(payloadTx));
  }else{
    isReceive = false;
    //Serial.println("Not Received");
  }

  return payloadRx;
}

int checkSpeed(int speed) {
  if(speed > 2000){
    speed = 2000;
  }
  else if(speed < 1100){
    speed = 1100;
  }

  return speed;
}

//setup
void setup() {
  Serial.begin(115200);
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


  motorFR.writeMicroseconds(1000);
  motorFL.writeMicroseconds(1000);
  motorBR.writeMicroseconds(1000);
  motorBL.writeMicroseconds(1000);
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

  payloadRx = commute(payloadTx);

  if(currentDroneStatus.pitch > 30 || currentDroneStatus.roll > 30 || currentDroneStatus.pitch < -30 || currentDroneStatus.roll < -30){
    sstop();
  }

  if(!isReceive){
    if(micros() - lastReciveTime >= 5000000){
      sstop();
    }
  }else{
    basicThrust = payloadRx.basicThrust;
    lastReciveTime = micros();
    if(payloadRx.stop == 1){
      sstop();
    }
  }

  if(payloadRx.status.yaw == 1){
    isReady = true;
  }

  //apply control
  unsigned long currentTime = micros();
  float dt = (currentTime*1.0 - lastTime*1.0) / 1000000;
  if(dt <= 0) dt = 0.000001;
  lastTime = currentTime;

  error[0] = payloadRx.status.pitch - currentDroneStatus.pitch;
  error[1] = payloadRx.status.roll - currentDroneStatus.roll;
  pid[0] = (kp * error[0]) + (ki * integral[0]) + (kd * ((error[0] - lastErr[0]) / dt));
  pid[1] = (kp * error[1]) + (ki * integral[1]) + (kd * ((error[1] - lastErr[1]) / dt));
  lastErr[0] = error[0];
  lastErr[1] = error[1];

  currentSpeed[0] = basicThrust + (pid[0]) + (pid[1]);
  currentSpeed[1] = basicThrust + (pid[0]) + (-pid[1]);
  currentSpeed[2] = basicThrust + (-pid[0]) + (-pid[1]);
  currentSpeed[3] = basicThrust + (-pid[0]) + (pid[1]);

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

  if(isReady){
    motorFL.writeMicroseconds(currentSpeed[0]);
    motorFR.writeMicroseconds(currentSpeed[1]);
    motorBR.writeMicroseconds(currentSpeed[2]);
    motorBL.writeMicroseconds(currentSpeed[3]);
  }else{
    pitchErr = currentDroneStatus.pitch;
    rollErr = currentDroneStatus.roll;

    motorFL.writeMicroseconds(1000);
    motorFR.writeMicroseconds(1000);
    motorBR.writeMicroseconds(1000);
    motorBL.writeMicroseconds(1000);

    currentSpeed[0] = 1000;
    currentSpeed[1] = 1000;
    currentSpeed[2] = 1000;
    currentSpeed[3] = 1000;
  }


  // Serial.print("Pitch:");
  // Serial.print(currentDroneStatus.pitch);
  // Serial.print(" Row:");
  // Serial.print(currentDroneStatus.roll);
  // Serial.print(" Yaw:");
  // Serial.print(currentDroneStatus.yaw);

  //     Serial.print(" ErrorX");
  // Serial.print(error[0]);
  //       Serial.print(" ErrorY");
  // Serial.print(error[1]);
  //       Serial.print(" ErrorZ");
  // Serial.print(error[2]);
  //   Serial.print(" PIDX");
  // Serial.print(pid[0]);
  // Serial.print(" PIDY");
  // Serial.print(pid[1]);
  // Serial.print(" PIDZ");
  // Serial.print(pid[2]);

  // Serial.print(" BT");
  // Serial.print(basicThrust);
  // Serial.print(" FL");
  // Serial.print(currentSpeed[0]);
  // Serial.print(" FR");
  // Serial.print(currentSpeed[1]);
  // Serial.print(" BR");
  // Serial.print(currentSpeed[2]);
  // Serial.print(" BL");
  // Serial.println(currentSpeed[3]);

  //delay
  delay(10);
}
