//import
#include <cvzone.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

//structure
struct DroneStatus {
  float pitch;
  float roll;
  float yaw;
  float a[3];
  float v[3];
  unsigned long lastTime;
};
struct TxDataType {
  DroneStatus status;
  float basicThrust;
  bool stop;
  bool isReceive;
};
struct RxDataType {
  DroneStatus status;
  int speed[4];
  bool isReceive;
};

//const
#define CE_PIN 7
#define CSN_PIN 8
uint8_t address[][8] = { "droneRx","droneTx" };

//config
RF24 radio(CE_PIN, CSN_PIN);
SerialData serialData(1, 1);

//val
DroneStatus currentDroneStatus;
TxDataType payloadTx;
RxDataType payloadRx;
int keyPressed[1];

//func
RxDataType commute(TxDataType payloadTx){
  RxDataType payloadRx;

  radio.stopListening();
  bool ok = radio.write(&payloadTx, sizeof(payloadTx));

  if(radio.isAckPayloadAvailable()){
    radio.read(&payloadRx, sizeof(payloadRx));
    Serial.println("Recieved");
    radio.writeAckPayload(1, &payloadTx, sizeof(payloadTx));
  }else{
    payloadRx.isReceive = false;
    Serial.println("Not Recieved");
  }

  radio.startListening();

  return payloadRx;
}

//setup
void setup() {
  Serial.begin(9600);

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
  
  //setup cvzone
  serialData.begin();
}

//start
void loop() {
  //get control data
  serialData.Get(keyPressed);
  if (keyPressed[0] == 87) {
    payloadTx.status.v[0] = 1;
  } else if (keyPressed[0] == 83) {
    payloadTx.status.v[0] = -1;
  } else {
    payloadTx.status.v[0] = 0;
  }

  if (keyPressed[0] == 68) {
    payloadTx.status.v[1] = 1;
  } else if (keyPressed[0] == 65) {
    payloadTx.status.v[1] = -1;
  } else {
    payloadTx.status.v[1] = 0;
  }

  if (keyPressed[0] == 69) {
    payloadTx.status.v[2] = 1;
  } else if (keyPressed[0] == 81) {
    payloadTx.status.v[2] = -1;
  } else {
    payloadTx.status.v[2] = 0;
  }

  if (keyPressed[0] == 67) {
    payloadTx.status.yaw = 1;
  } else if (keyPressed[0] == 90) {
    payloadTx.status.yaw = -1;
  } else {
    payloadTx.status.yaw = 0;
  }

  if (keyPressed[0] == 16777248) {
    payloadTx.basicThrust += 10;
  } else if (keyPressed[0] == 16777249) {
    payloadTx.basicThrust -= 10;
  }

  if(keyPressed[0] == 88) {
    payloadTx.stop = true;
  } else {
    payloadTx.stop = false;
  }

  payloadTx.isReceive = true;

  //commute
  payloadRx = commute(payloadTx);

  //delay
  delay(10);
}
