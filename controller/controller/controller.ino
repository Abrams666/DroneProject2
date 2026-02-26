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
};

//config
RF24 radio(CE_PIN, CSN_PIN);
SerialData serialData(1, 1);

//val
bool comType = true;
int targetPitch = 0;
int targetRoll = 0;
int yawDir = 1;
int mySwitch[1];
TxDataType payloadTx;
RxDataType payloadRx;

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
  
  //setup cvzone
  serialData.begin();
}

//start
void loop() {
  //get control data
  //serialData.Get(payloadTx);

  //tx
  radio.stopListening();
  tx(payloadTx);

  //rx
  radio.startListening();
  payloadRx = rx();

  //send current status
  //serialData.Send(payloadRx);

  //delay
  delay(10);
}
