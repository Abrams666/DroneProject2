//import
#include <cvzone.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

//const
#define CE_PIN 7
#define CSN_PIN 8
uint8_t address[][8] = { "droneRx","droneTx" };
struct TxDataType {
  int targetPitch;
  int targetRoll;
  int targetThrust;
  int yawDir;
  bool stop;
  bool isReceive;
};
struct RxDataType {
  int FR;
  int FL;
  int BR;
  int BL;
  int pitch;
  int roll;
  int heading;
  bool isReceive;
};

//config
RF24 radio(CE_PIN, CSN_PIN);
SerialData serialData(1, 1);

//val
bool comType = true;
int targetPitch = 0;
int targetRoll = 0;
int yawDir = 1;
int keyPressed[1];
TxDataType payloadTx;
RxDataType payloadRx;
bool xxx = true;

//func
void tx(TxDataType payload) {
  radio.stopListening();
  unsigned long start_timer = micros();
  bool report = radio.write(&payload, sizeof(TxDataType));
  unsigned long end_timer = micros();

  if (report) {
    Serial.println(F("Send"));
  } else {
    Serial.println(F("Transmission failed or timed out"));
  }
  radio.startListening();
}

RxDataType rx() {
  RxDataType payload;

  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize();
    radio.read(&payload, bytes);
    xxx = false;
  }else{
    payload.isReceive = false;
  }

  return payload;
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
  radio.stopListening();
  payloadRx.isReceive = false;
  
  //setup cvzone
  serialData.begin();
}

//start
void loop() {
  //get control data
  serialData.Get(keyPressed);
  if (keyPressed[0] == 87) {
    payloadTx.targetPitch = 10;
  } else if (keyPressed[0] == 83) {
    payloadTx.targetPitch = -10;
  } else {
    payloadTx.targetPitch = 0;
  }

  if (keyPressed[0] == 65) {
    payloadTx.targetRoll = -10;
  } else if (keyPressed[0] == 68) {
    payloadTx.targetRoll = 10;
  } else {
    payloadTx.targetRoll = 0;
  }

  if (keyPressed[0] == 81) {
    payloadTx.yawDir = -1;
  } else if (keyPressed[0] == 69) {
    payloadTx.yawDir = 1;
  } else {
    payloadTx.yawDir = 0;
  }

  if(keyPressed[0] == 88) {
    payloadTx.stop = true;
  } else {
    payloadTx.stop = false;
  }

  payloadTx.isReceive = true;

  Serial.println(xxx);
  //tx
  if(payloadRx.isReceive || xxx){
    tx(payloadTx);
  }

  //rx
  radio.startListening();
  payloadRx = rx();

  //send current status
  //serialData.Send(payloadRx);

  //delay
  delay(10);
}
