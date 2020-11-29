// C++ interface for an I2C Display connected on /dev/i2c1 on address 0x27
// Created:  1.0, 20200926 - Ward Dewerchin
// Modified: 1.1, 20201029 - Ward Dewerchin, port to new development environment (ubuntu x64 LTS 20.08), eclipse 2020-09
//
// Hints:
// *  When copying a project and index error are reported, delete the pdom file which is located in the following directory:
//    ~/<workspace>/.metadata/.plugins/org.eclipse.cdt.core$ rm <projectname>.pdom

// * Setting for cross compile - prefix: arm-linux-gnueabihf- & -std=c++17 -> runtime not available on RPi2 (is on RPi3)
//                                                              -std=c++14 & /home/wd/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
// * To include cpp and h file from another location or project (e.g. tools, i2c - shared items):
//   right click on project ->new->folder->advanced->link to alternate location (linked folders)

#include <string>
#include <iostream>
#include <cmath>

using namespace std;

#include <stdio.h>
#include <unistd.h>				// Needed for I2C port
#include <fcntl.h>				// Needed for I2C port
#include <errno.h>				// include if errno is being used
#include <sys/ioctl.h>			// Needed for I2C port
#include <linux/i2c-dev.h>		// Needed for I2C port
#include <time.h>

#include <memory.h>

#include "../Tools/Tools.h"
#include "../I2C/I2C.h"

// ----------------------------------------------------------------------------------------------------------------------------
class CI2cDisplay
{
public:
   CI2cDisplay(const unsigned char cAddress);
   ~CI2cDisplay(void);

   void Init(void);
   void WriteString(const unsigned char cPos, const char * spData);
   void SetBackLight(bool bBackLight);

private:
   int           iI2cFile;
   unsigned char cBackLight;

   void WriteCommand(char c);
   void WriteData(char c);
};

CI2cDisplay::CI2cDisplay(const unsigned char cAddress)
{
   // Open the I2C on /dev/i2c-1
   // If the drivers is not loaded (e.g. ls /dev/i2c* yields nothing), enable I2C using 'sudo raspi-config'. Remark
   // that on the RPi (as of B+) only i2c-1 is available, i2c-0 is reservered (ID_SD & ID_SC) and used during start-up
   // to identify which HAT (Hardware Attached on Top) is present by checking the id written in the id eeprom on
   // address 0x50. (see also B+ Add-on boards and hats at https://github.com/raspberrypi/hats)

   iI2cFile = I2cOpen("1", cAddress);
   cBackLight = 0x00;
}

CI2cDisplay::~CI2cDisplay(void)
{
   I2cClose(iI2cFile);
}

void CI2cDisplay::Init(void)
{
   unsigned char spData[1];

   // Enable 4 bits mode by writing 8 bits (twice)
    spData[0] = 0x2C; I2cWrite(iI2cFile, spData, 1);
    usleep(1000);
    spData[0] = 0x28; I2cWrite(iI2cFile, spData, 1);
    usleep(1000);

    WriteCommand(0x28); // address?
    WriteCommand(0x0C); // enable display(bit 2), cursor (bit 1), blink (bit 0)
    WriteCommand(0x06); // cursor move increment (bit1),  scroll (bit0)
    WriteCommand(0x01); // clear Display
}

void CI2cDisplay::WriteString(const unsigned char cPos, const char * spData)
{
   WriteCommand(cPos);
   for (unsigned int i = 0; i < strlen((const char *) spData); ++i) WriteData(spData[i]);
}

// high nibble contains data (sent high then low)
// low nibble:
// - bit 0: command/data (0/1)
// - bit 1: write/read   (0/1)
// - bit 2: enable,      (data clocked in on falling edge)
// - bit 3: backlight    (0/1)


// write command in 4 bits mode
void CI2cDisplay::WriteCommand(char c)
{
   unsigned char spData[4];
   // high nibble
   spData[0] = ((c << 0) & 0xf0) | cBackLight | 0x04;
   spData[1] = spData[0] & 0b11111011; // reset enable
   // low nibble
   spData[2] = ((c << 4) & 0xf0) | cBackLight | 0x04;
   spData[3] = spData[2] & 0b11111011; // reset enable
   I2cWrite(iI2cFile, spData, 4);
}

// write data in 4 bits mode
void CI2cDisplay::WriteData(char c)
{
   unsigned char spData[4];
   // high nibble
   spData[0] = ((c << 0) & 0xf0) | cBackLight | 0x05;
   spData[1] = spData[0] & 0b11111011; // reset enable
   // low nibble
   spData[2] = ((c << 4) & 0xf0) | cBackLight | 0x05;
   spData[3] = spData[2] & 0b11111011; // reset enable
   I2cWrite(iI2cFile, spData, 4);
}

void CI2cDisplay::SetBackLight(bool bBackLight)
{
   bBackLight ? cBackLight = 0x08 : cBackLight = 0x00;
}

// ----------------------------------------------------------------------------------------------------------------------------

int main(int argc, char * argv[])
{
	bool bBackLight = false;
	try
	{
		CI2cDisplay cI2cDisplay(0x27);

		// Initialize
		if (argc == 2 && (string) "i" == argv[1])
		{
			cI2cDisplay.Init();
		}
		if (argc > 3)
		{
			if ((string) "1" == argv[2]) bBackLight = true;
			cI2cDisplay.SetBackLight(bBackLight);
			char cPos = atoi(argv[1]) + 0x80;
			cI2cDisplay.WriteString(cPos, argv[3]);
		}
	}
	catch (string &sMessage)
	{
		cout << "I2C Display - " << sMessage << endl;
	}
	return (0);
}
