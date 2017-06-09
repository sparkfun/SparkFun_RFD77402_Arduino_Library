/*
  Getting distance from the RFD77402 Time of Flight Sensor
  By: Nathan Seidle
  SparkFun Electronics
  Date: June 6th, 2017
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!

  This example shows how to maximize the read speed. Connect to sensor at 400kHz I2C and increase
  serial to 115200.

  This doesn't save much. Normal is ~54ms, fast is ~51ms but you can hook in different Wire ports
  like Wire1, Wire2, softWire, etc.

  Hardware Connections:
  Attach the Qwiic Shield to your Arduino/Photon/ESP32 or other
  Plug the sensor onto the shield (any port)
  Open the serial monitor at 115200 baud

  Available:
  takeMeasurement() - Initiates measurement. Read via getDistance(), etc

  getDistance() - Returns the last measurement value
  getValidPixels() - Returns number of valid pixels
  getConfidenceValue() - A qualitative value of the distance measurement

  getPeak/setPeak(byte) - Gets/sets the vertical-cavity surface-emitting laser (VCSEL) peak
  getThreshold/setThreshold(byte) - Gets/sets the VCSEL threshold
  getFrequency/setFrequency(byte) - Gets/sets the modulation frequency

  goToOffMode() - Turn off MCPU
  goToOnMode() - Wake MCPU to ON Mode
  goToStandbyMode() - Low power standby mode
  getMeasurement() - Once sensor is configured, issue this command to take measurement

  getCalibrationData() - reads 27 messages of MPU mailbox data and loads calibration data array
  getMailbox() - returns the 16-bits in the MPU mailbox

  reset() - Resets IC to initial POR settings

*/

#include <SparkFun_RFD77402_Arduino_Library.h> //Use Library Manager or download here: https://github.com/sparkfun/SparkFun_RFD77402_Arduino_Library
RFD77402 myDistance; //Hook object to the library

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("RFD77402 Read Example");

  //Initialize sensor. Tell it use the Wire port (Wire1, Wire2, softWire, etc) and at 400kHz (I2C_SPEED_FAST or _NORMAL)
  if (myDistance.begin(Wire, I2C_SPEED_FAST) == false)
  {
    Serial.println("Sensor failed to initialize. Check wiring.");
    while (1); //Freeze!
  }
  Serial.println("Sensor online!");
}

void loop()
{
  long startTimer = millis();
  byte errorCode = myDistance.takeMeasurement();
  long timeDelta = millis() - startTimer;
  
  if (errorCode == CODE_VALID_DATA)
  {
    unsigned int distance = myDistance.getDistance();
    byte pixels = myDistance.getValidPixels();
    unsigned int confidence = myDistance.getConfidenceValue();

    Serial.print("distance: ");
    Serial.print(distance);
    Serial.print("mm timeDelta: ");
    Serial.print(timeDelta);

    if(distance > 2000) Serial.print(" Nothing sensed");
  }
  else if (errorCode == CODE_FAILED_PIXELS)
  {
    Serial.print("Not enough pixels valid");
  }
  else if (errorCode == CODE_FAILED_SIGNAL)
  {
    Serial.print("Not enough signal");
  }
  else if (errorCode == CODE_FAILED_SATURATED)
  {
    Serial.print("Sensor pixels saturated");
  }
  else if (errorCode == CODE_FAILED_NOT_NEW)
  {
    Serial.print("New measurement failed");
  }
  else if (errorCode == CODE_FAILED_TIMEOUT)
  {
    Serial.print("Sensors timed out");
  }

  Serial.println();
}
