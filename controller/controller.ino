//import
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include <ArduinoJson.h>

//structure
struct DroneStatus {
  float pitch;
  float roll;
  float yaw;
};
struct TxDataType {
  DroneStatus status;
  float basicThrust;
  bool stop;
};
struct RxDataType {
  DroneStatus status;
  int speed[4];
};

//const
#define CE_PIN 7
#define CSN_PIN 8
uint8_t address[][8] = { "droneRx","droneTx" };

//config
RF24 radio(CE_PIN, CSN_PIN);

//val
DroneStatus currentDroneStatus;
TxDataType payloadTx;
RxDataType payloadRx;
bool isReceive = true;
bool leftBtn = false;
bool rightBtn = true;
bool stopBtn = true;
float yaw = 0;
float pitch = 0;
float roll = 0;
float thrustBar = 0;

//func
RxDataType commute(TxDataType payloadTx){
  RxDataType payloadRx;

  radio.stopListening();
  bool ok = radio.write(&payloadTx, sizeof(payloadTx));

  if(radio.isAckPayloadAvailable()){
    radio.read(&payloadRx, sizeof(payloadRx));
    //Serial.print("Recieved ");
    //Serial.println(payloadRx.speed[3]);
    isReceive = true;
  }else{
    isReceive = false;
    Serial.println("Not Recieved");
  }

  radio.startListening();

  return payloadRx;
}

//setup
void setup() {
  Serial.begin(115200);

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
  radio.setPayloadSize(sizeof(RxDataType));
  radio.openWritingPipe(address[1]);
  radio.openReadingPipe(1, address[0]);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();

  payloadTx.status.pitch = 0;
  payloadTx.status.roll = 0;
  payloadTx.status.yaw = 0;
  payloadTx.basicThrust = 1000;
  payloadTx.stop = false;

  //pin
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
}

//start
void loop() {
  payloadTx.status.pitch = 0;
  payloadTx.status.roll = 0;
  payloadTx.status.yaw = 0;
  payloadTx.basicThrust = 1000;
  payloadTx.stop = false;

  leftBtn = digitalRead(2);
  rightBtn = digitalRead(3);
  stopBtn = digitalRead(4);
  pitch = map(analogRead(A2) - 9, 0, 1023, -10, 10);
  roll = map(analogRead(A1) + 11, 0, 1023, 10, -10);
  thrustBar = map(analogRead(A0), 0, 1023, 2000, 1000);

  if(rightBtn){
    yaw = 1;
  }else if(leftBtn){
    yaw = -1;
  }else{
    yaw = 0;
  }

  payloadTx.status.pitch = pitch;
  payloadTx.status.roll = roll;
  payloadTx.status.yaw = yaw;
  payloadTx.basicThrust = thrustBar;
  payloadTx.stop = stopBtn;

  Serial.print("CP:");
  Serial.print(payloadTx.status.pitch);
  Serial.print(" CR:");
  Serial.print(payloadTx.status.roll);
  Serial.print(" CYaw:");
  Serial.print(payloadTx.status.yaw);
  Serial.print(" T:");
  Serial.print(payloadTx.basicThrust);
  Serial.print(" S:");
  Serial.print(payloadTx.stop);
  Serial.print(" P:");
  Serial.print(payloadRx.status.pitch);
  Serial.print(" R:");
  Serial.print(payloadRx.status.roll);
  Serial.print(" Y:");
  Serial.print(payloadRx.status.yaw);
  Serial.print(" FL:");
  Serial.print(payloadRx.speed[0]);
  Serial.print(" FR:");
  Serial.print(payloadRx.speed[1]);
  Serial.print(" BR:");
  Serial.print(payloadRx.speed[2]);
  Serial.print(" BL:");
  Serial.println(payloadRx.speed[3]);

  //commute
  payloadRx = commute(payloadTx);

  //delay
  delay(10);
}
