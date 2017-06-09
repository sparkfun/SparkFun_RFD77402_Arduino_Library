/*
  Fast tester for testing production units
  By: Nathan Seidle
  SparkFun Electronics
  Date: June 6th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Repeatedly looks for and reads distance from the RFD77402
*/

#include <SparkFun_RFD77402_Arduino_Library.h> //Use Library Manager or download here: https://github.com/sparkfun/SparkFun_RFD77402_Arduino_Library
RFD77402 myDistance; //Hook object to the library

void setup()
{
  Serial.begin(9600);
  while (!Serial);
  Serial.println("RFD77402 Testing");
}

void loop()
{
  if (myDistance.begin() == false)
  {
    Serial.println("Sensor failed to initialize. Check wiring.");
  }
  else
  {
    myDistance.takeMeasurement(); //Tell sensor to take measurement

    unsigned int distance = myDistance.getDistance(); //Retrieve the distance value

    Serial.print("distance: ");
    Serial.print(distance);
    Serial.print("mm");
    Serial.println();
  }
}
