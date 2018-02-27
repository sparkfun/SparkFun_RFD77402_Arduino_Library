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

#pragma once

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>

#define RFD77402_ADDR 0x4C //7-bit unshifted default I2C Address

//Register addresses
#define RFD77402_ICSR 0x00
#define RFD77402_INTERRUPTS 0x02
#define RFD77402_COMMAND 0x04
#define RFD77402_DEVICE_STATUS 0x06
#define RFD77402_RESULT 0x08
#define RFD77402_RESULT_CONFIDENCE 0x0A
#define RFD77402_CONFIGURE_A 0x0C
#define RFD77402_CONFIGURE_B 0x0E
#define RFD77402_HOST_TO_MCPU_MAILBOX 0x10
#define RFD77402_MCPU_TO_HOST_MAILBOX 0x12
#define RFD77402_CONFIGURE_PMU 0x14
#define RFD77402_CONFIGURE_I2C 0x1C
#define RFD77402_CONFIGURE_HW_0 0x20
#define RFD77402_CONFIGURE_HW_1 0x22
#define RFD77402_CONFIGURE_HW_2 0x24
#define RFD77402_CONFIGURE_HW_3 0x26
#define RFD77402_MOD_CHIP_ID 0x28

#define RFD77402_MODE_MEASUREMENT 0x01
#define RFD77402_MODE_STANDBY 0x10
#define RFD77402_MODE_OFF 0x11
#define RFD77402_MODE_ON 0x12

#define CODE_VALID_DATA 0x00
#define CODE_FAILED_PIXELS 0x01
#define CODE_FAILED_SIGNAL 0x02
#define CODE_FAILED_SATURATED 0x03
#define CODE_FAILED_NOT_NEW 0x04
#define CODE_FAILED_TIMEOUT 0x05

#define I2C_SPEED_STANDARD        100000
#define I2C_SPEED_FAST            400000

#define INT_CLR_REG 1 //tells which register read clears the interrupt (Default: 1, Result Register)
#define INT_CLR 0 << 1 //tells whether or not to clear when register is read (Default: 0, cleared upon register read)
#define INT_PIN_TYPE 1 << 2 //tells whether int is push-pull or open drain (Default: 1, push-pull)
#define INT_LOHI 0 << 3 //tells whether the interrupt is active low or high (Default: 0, active low)

//Setting any of the following bits to 1 enables an interrupt when that event occurs
#define INTSRC_DATA 1 //Interrupt fires with newly available data
#define INTSRC_M2H 0 << 1//Interrupt fires with newly available data in M2H mailbox register
#define INTSRC_H2M 0 << 2//Interrupt fires when H2M register is read
#define INTSRC_RST 0 << 3 //Interrupt fires when HW reset occurs


class RFD77402 {
  public:

    //By default use Wire, standard I2C speed, and the defaul AK9750 address
    boolean begin(TwoWire &wirePort = Wire, uint32_t i2cSpeed = I2C_SPEED_STANDARD);

    uint8_t takeMeasurement(); //Takes a single measurement and sets the global variables with new data
	uint16_t getDistance(); //Returns the local variable to the caller
	uint8_t getValidPixels(); //Returns the number of valid pixels found when taking measurement
	uint16_t getConfidenceValue(); //Returns the qualitative value representing how confident the sensor is about its reported distance
	uint8_t getMode(); //Read the command opcode and covert to mode

	boolean goToStandbyMode(); //Tell MCPU to go to standby mode. Return true if successful
	boolean goToOffMode(); //Tell MCPU to go to off state. Return true if successful
	boolean goToOnMode(); //Tell MCPU to go to on state. Return true if successful
	boolean goToMeasurementMode(); //Tell MCPU to go to measurement mode. Takes a measurement. If measurement data is ready, return true
	
	uint8_t getPeak(); //Returns the VCSEL peak 4-bit value
	void setPeak(uint8_t peakValue); //Sets the VCSEL peak 4-bit value
	uint8_t getThreshold(); //Returns the VCSEL Threshold 4-bit value
	void setThreshold(uint8_t threshold); //Sets the VCSEL Threshold 4-bit value
	uint8_t getFrequency(); //Returns the VCSEL Frequency 4-bit value
	void setFrequency(uint8_t threshold); //Sets the VCSEL Frequency 4-bit value

	uint16_t getMailbox(); //Gets whatever is in the 'MCPU to Host' mailbox. Check ICSR bit 5 before reading.
	void reset(); //Software reset the device
	uint16_t getChipID(); //Returns the chip ID. Should be 0xAD01 or higher.

	//Retreive 2*27 bytes from MCPU for computation of calibration parameters
	//Reads 54 bytes into the calibration[] array
	//Returns true if new cal data is loaded
	boolean getCalibrationData();

	uint16_t readRegister16(uint8_t addr); //Reads two bytes from a given location from the RFD77402
	uint8_t readRegister(uint8_t addr); //Reads from a given location from the RFD77402
	void writeRegister16(uint8_t addr, uint16_t val); //Write a 16 bit value to a spot in the RFD77402
	void writeRegister(uint8_t addr, uint8_t val); //Write a value to a spot in the RFD77402

    //Variables
	uint16_t distance;
	uint8_t validPixels;
	uint16_t confidenceValue;
	uint8_t calibrationData[54]; //Loaded by the 0x006 mailbox command

  private:

    //Variables
    TwoWire *_i2cPort; //The generic connection to user's chosen I2C hardware
};
