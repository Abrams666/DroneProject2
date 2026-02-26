//import
#include <cvzone.h>

SerialData serialData(1, 1);
int mySwitch[1];

void setup() {
  pinMode(13, OUTPUT);
  serialData.begin();
}

void loop() {
  serialData.Get(mySwitch);
  digitalWrite(13, mySwitch[0]);
  //serialData.Send(mySwitch[0]);

  delay(10);
}
