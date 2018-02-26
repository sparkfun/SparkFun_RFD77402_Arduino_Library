/*
  This is a library written for the RFD77402 Time of Flight sensor.
  SparkFun sells these at its website: www.sparkfun.com

  Written by Nathan Seidle @ SparkFun Electronics, June 9th, 2017

  The VCSEL (vertical-cavity surface-emitting laser) Time of Flight sensor 
  can accurately measure from 10cm to 200cm (2m) with milimeter precision.

  This library handles the initialization of the RFD77402 and bringing in of
  various readings.

  https://github.com/sparkfun/SparkFun_RFD77402_Arduino_Library

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.1

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "SparkFun_RFD77402_Arduino_Library.h"

//Sets up the sensor for constant read
//Returns false if sensor does not respond
boolean RFD77402::begin(TwoWire &wirePort, uint32_t i2cSpeed)
{
  //Bring in the user's choices
  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2cPort->begin();
  _i2cPort->setClock(i2cSpeed);

  if (getChipID() < 0xAD00) return (false); //Chip ID failed. Should be 0xAD01 or 0xAD02

  //Put chip into standby
  if (goToStandbyMode() == false) return (false); //Chip timed out before going to standby

  //Drive INT_PAD high
  uint8_t setting = readRegister(RFD77402_ICSR);
  setting &= 0b11110000; //clears writable bits
  setting |= INT_CLR_REG | INT_CLR | INT_PIN_TYPE | INT_LOHI; //change bits to enable interrupt
  writeRegister(RFD77402_ICSR, setting);
  setting = readRegister(RFD77402_INTERRUPTS);
  setting &= 0b00000000; //Clears bits
  setting |= INTSRC_DATA | INTSRC_M2H | INTSRC_H2M | INTSRC_RST; //Enables interrupt when data is ready
  writeRegister(RFD77402_INTERRUPTS, setting);

  //Configure I2C Interface
  writeRegister(RFD77402_CONFIGURE_I2C, 0x65); //0b.0110.0101 = Address increment, auto increment, host debug, MCPU debug

  //Set initialization - Magic from datasheet. Write 0x05 to 0x15 location.
  writeRegister16(RFD77402_CONFIGURE_PMU, 0x0500); //0b.0000.0101.0000.0000 //Patch_code_id_en, Patch_mem_en

  if (goToOffMode() == false) return (false); //MCPU never turned off

  //Read Module ID
  //Skipped

  //Read Firmware ID
  //Skipped

  //Set initialization - Magic from datasheet. Write 0x06 to 0x15 location.
  writeRegister16(RFD77402_CONFIGURE_PMU, 0x0600); //MCPU_Init_state, Patch_mem_en

  if (goToOnMode() == false) return (false); //MCPU never turned on

  //ToF Configuration
  //writeRegister16(RFD77402_CONFIGURE_A, 0xE100); //0b.1110.0001 = Peak is 0x0E, Threshold is 1.
  setPeak(0x0E); //Suggested values from page 20
  setThreshold(0x01);

  writeRegister16(RFD77402_CONFIGURE_B, 0x10FF); //Set valid pixel. Set MSP430 default config.
  writeRegister16(RFD77402_CONFIGURE_HW_0, 0x07D0); //Set saturation threshold = 2,000.
  writeRegister16(RFD77402_CONFIGURE_HW_1, 0x5008); //Frequecy = 5. Low level threshold = 8.
  writeRegister16(RFD77402_CONFIGURE_HW_2, 0xA041); //Integration time = 10 * (6500-20)/15)+20 = 4.340ms. Plus reserved magic.
  writeRegister16(RFD77402_CONFIGURE_HW_3, 0x45D4); //Enable harmonic cancellation. Enable auto adjust of integration time. Plus reserved magic.

  if (goToStandbyMode() == false) return (false); //Error - MCPU never went to standby

  //Whew! We made it through power on configuration

  //Get the calibration data via the 0x0006 mailbox command
  //getCalibrationData(); //Skipped
  
  //Put device into Standby mode
  if (goToStandbyMode() == false) return (false); //Error - MCPU never went to standby

  //Now assume user will want sensor in measurement mode

  //Set initialization - Magic from datasheet. Write 0x05 to 0x15 location.
  writeRegister16(RFD77402_CONFIGURE_PMU, 0x0500); //Patch_code_id_en, Patch_mem_en

  if (goToOffMode() == false) return (false); //Error - MCPU never turned off

  //Write calibration data
  //Skipped

  //Set initialization - Magic from datasheet. Write 0x06 to 0x15 location.
  writeRegister16(RFD77402_CONFIGURE_PMU, 0x0600); //MCPU_Init_state, Patch_mem_en

  if (goToOnMode() == false) return (false); //Error - MCPU never turned on

  return (true); //Success! Sensor is ready for measurements
}

//Takes a single measurement and sets the global variables with new data
//Returns zero if reading is good, otherwise return the errorCode from the result register.
uint8_t RFD77402::takeMeasurement(void)
{
  if (goToMeasurementMode() == false) return (CODE_FAILED_TIMEOUT); //Error - Timeout
  //New data is now available!

  //Read result
  uint16_t resultRegister = readRegister16(RFD77402_RESULT);

  if (resultRegister & 0x7FFF) //Reading is valid
  {
    uint8_t errorCode = (resultRegister >> 13) & 0x03;

    if (errorCode == 0)
    {
      distance = (resultRegister >> 2) & 0x07FF; //Distance is good. Read it.
      //Serial.println("Distance field valid");

      //Read confidence register
      uint16_t confidenceRegister = readRegister16(RFD77402_RESULT_CONFIDENCE);
      validPixels = confidenceRegister & 0x0F;
      confidenceValue = (confidenceRegister >> 4) & 0x07FF;
    }

    return (errorCode);

  }
  else
  {
    //Reading is not vald
    return (CODE_FAILED_NOT_NEW); //Error code for reading is not new
  }

}

//Returns the local variable to the caller
uint16_t RFD77402::getDistance()
{
  return (distance);
}

//Returns the number of valid pixels found when taking measurement
uint8_t RFD77402::getValidPixels()
{
  return (validPixels);
}

//Returns the qualitative value representing how confident the sensor is about its reported distance
uint16_t RFD77402::getConfidenceValue()
{
  return (confidenceValue);
}

//Read the command opcode and covert to mode
uint8_t RFD77402::getMode()
{
  return (readRegister(RFD77402_COMMAND) & 0x3F);
}

//Tell MCPU to go to standby mode
//Return true if successful
boolean RFD77402::goToStandbyMode()
{
  //Set Low Power Standby
  writeRegister(RFD77402_COMMAND, 0x90); //0b.1001.0000 = Go to standby mode. Set valid command.

  //Check MCPU_ON Status
  for (uint8_t x = 0 ; x < 10 ; x++)
  {
    if ( (readRegister16(RFD77402_DEVICE_STATUS) & 0x001F) == 0x0000) return (true); //MCPU is now in standby
    delay(10); //Suggested timeout for status checks from datasheet
  }

  return (false); //Error - MCPU never went to standby
}

//Tell MCPU to go to off state
//Return true if successful
boolean RFD77402::goToOffMode()
{
  //Set MCPU_OFF
  writeRegister(RFD77402_COMMAND, 0x91); //0b.1001.0001 = Go MCPU off state. Set valid command.

  //Check MCPU_OFF Status
  for (uint8_t x = 0 ; x < 10 ; x++)
  {
    if ( (readRegister16(RFD77402_DEVICE_STATUS) & 0x001F) == 0x0010) return (true); //MCPU is now off
    delay(10); //Suggested timeout for status checks from datasheet
  }

  return (false); //Error - MCPU never turned off
}

//Tell MCPU to go to on state
//Return true if successful
boolean RFD77402::goToOnMode()
{
  //Set MCPU_ON
  writeRegister(RFD77402_COMMAND, 0x92); //0b.1001.0010 = Wake up MCPU to ON mode. Set valid command.

  //Check MCPU_ON Status
  for (uint8_t x = 0 ; x < 10 ; x++)
  {
    if ( (readRegister16(RFD77402_DEVICE_STATUS) & 0x001F) == 0x0018) return (true); //MCPU is now on
    delay(10); //Suggested timeout for status checks from datasheet
  }

  return (false); //Error - MCPU never turned on
}

//Tell MCPU to go to measurement mode
//Takes a measurement. If measurement data is ready, return true
boolean RFD77402::goToMeasurementMode()
{
  //Single measure command
  writeRegister(RFD77402_COMMAND, 0x81); //0b.1000.0001 = Single measurement. Set valid command.

  //Read ICSR Register - Check to see if measurement data is ready
  for (uint8_t x = 0 ; x < 10 ; x++)
  {
    if ( (readRegister(RFD77402_ICSR) & (1 << 4)) != 0) return (true); //Data is ready!
    delay(10); //Suggested timeout for status checks from datasheet
  }

  return (false); //Error - Timeout
}

//Returns the VCSEL peak 4-bit value
uint8_t RFD77402::getPeak(void)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_A);
  return ((configValue >> 12) & 0x0F);
}

//Sets the VCSEL peak 4-bit value
void RFD77402::setPeak(uint8_t peakValue)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_A); //Read
  configValue &= ~0xF000;// Zero out the peak configuration bits
  configValue |= (uint16_t)peakValue << 12; //Mask in user's settings
  writeRegister16(RFD77402_CONFIGURE_A, configValue); //Write in this new value
}

//Returns the VCSEL Threshold 4-bit value
uint8_t RFD77402::getThreshold(void)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_A);
  return ((configValue >> 8) & 0x0F);
}

//Sets the VCSEL Threshold 4-bit value
void RFD77402::setThreshold(uint8_t thresholdValue)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_A); //Read
  configValue &= ~0x0F00;// Zero out the threshold configuration bits
  configValue |= thresholdValue << 8; //Mask in user's settings
  writeRegister16(RFD77402_CONFIGURE_A, configValue); //Write in this new value
}

//Returns the VCSEL Frequency 4-bit value
uint8_t RFD77402::getFrequency(void)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_HW_1);
  return ((configValue >> 12) & 0x0F);
}

//Sets the VCSEL Frequency 4-bit value
void RFD77402::setFrequency(uint8_t thresholdValue)
{
  uint16_t configValue = readRegister16(RFD77402_CONFIGURE_HW_1); //Read
  configValue &= ~0xF000;// Zero out the threshold configuration bits
  configValue |= thresholdValue << 12; //Mask in user's settings
  writeRegister16(RFD77402_CONFIGURE_HW_1, configValue); //Write in this new value
}

//Gets whatever is in the 'MCPU to Host' mailbox
//Check ICSR bit 5 before reading
uint16_t RFD77402::getMailbox(void)
{
  return (readRegister16(RFD77402_MCPU_TO_HOST_MAILBOX));
}

//Software reset the device
void RFD77402::reset(void)
{
  writeRegister(RFD77402_COMMAND, 1<<6);
  delay(100);
}

//Returns the chip ID
//Should be 0xAD01 or higher
uint16_t RFD77402::getChipID()
{
  return(readRegister16(RFD77402_MOD_CHIP_ID));
}

//Retreive 2*27 bytes from MCPU for computation of calibration parameters
//This is 9.2.2 from datasheet
//Reads 54 bytes into the calibration[] array
//Returns true if new cal data is loaded
boolean RFD77402::getCalibrationData(void)
{
  if (goToOnMode() == false) return (false); //Error - sensor timed out before getting to On Mode

  //Check ICSR Register and read Mailbox until it is empty
  uint8_t messages = 0;
  while (1)
  {
    if ( (readRegister(RFD77402_ICSR) & (1 << 5)) == 0) break; //Mailbox interrupt is cleared

    //Mailbox interrupt (Bit 5) is set so read the M2H mailbox register
    getMailbox(); //Throw it out. Just read to clear the register.

    if (messages++ > 27) return (false); //Error - Too many messages

    delay(10); //Suggested timeout for status checks from datasheet
  }

  //Issue mailbox command
  writeRegister16(RFD77402_HOST_TO_MCPU_MAILBOX, 0x0006); //Send 0x0006 mailbox command

  //Check to see if Mailbox can be read
  //Read 54 bytes of payload into the calibration[54] array
  for (uint8_t message = 0 ; message < 27 ; message++)
  {
    //Wait for bit to be set
    uint8_t x = 0;
    while (1)
    {
      uint8_t icsr = readRegister(RFD77402_ICSR);
      if ( (icsr & (1 << 5)) != 0) break; //New message in available

      if (x++ > 10) return (false); //Error - Timeout

      delay(10); //Suggested timeout for status checks from datasheet
    }

    uint16_t incoming = getMailbox(); //Get 16-bit message

    //Put message into larger calibrationData array
    calibrationData[message * 2] = incoming >> 8;
    calibrationData[message * 2 + 1] = incoming & 0xFF;
  }

  /*Serial.println("Calibration data:");
    for (int x = 0 ; x < 54 ; x++)
    {
    Serial.print("[");
    Serial.print(x);
    Serial.print("]=0x");
    if (calibrationData[x] < 0x10) Serial.print("0"); //Pretty print
    Serial.println(calibrationData[x], HEX);
    }*/
}



//Reads two bytes from a given location from the RFD77402
uint16_t RFD77402::readRegister16(uint8_t addr)
{
  _i2cPort->beginTransmission(RFD77402_ADDR);
  _i2cPort->write(addr);
  _i2cPort->endTransmission();

  _i2cPort->requestFrom(RFD77402_ADDR, 2);

  if (_i2cPort->available() != 2) return (0xFFFF); //Error

  uint8_t lower = _i2cPort->read();
  uint8_t higher = _i2cPort->read();

  return ((uint16_t)higher << 8 | lower);
}

//Reads from a given location from the RFD77402
uint8_t RFD77402::readRegister(uint8_t addr)
{
  _i2cPort->beginTransmission(RFD77402_ADDR);
  _i2cPort->write(addr);
  _i2cPort->endTransmission();

  _i2cPort->requestFrom(RFD77402_ADDR, 1);
  if (_i2cPort->available()) return (_i2cPort->read());

  return (0xFF); //Error
}

//Write a 16 bit value to a spot in the RFD77402
void RFD77402::writeRegister16(uint8_t addr, uint16_t val)
{
  _i2cPort->beginTransmission(RFD77402_ADDR);
  _i2cPort->write(addr);
  _i2cPort->write(val & 0xFF); //Lower byte
  _i2cPort->write(val >> 8); //Uper byte
  _i2cPort->endTransmission();
}

//Write a value to a spot in the RFD77402
void RFD77402::writeRegister(uint8_t addr, uint8_t val)
{
  _i2cPort->beginTransmission(RFD77402_ADDR);
  _i2cPort->write(addr);
  _i2cPort->write(val);
  _i2cPort->endTransmission();
}
