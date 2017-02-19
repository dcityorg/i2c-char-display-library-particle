/*
  Important NOTES:
    1. If using Arduino IDE, version 1.5.0 or higher is REQUIRED!
*/

/*
  i2c-char-display.cpp

  Written by: Gary Muhonen  gary@wht.io

  versions
    1.0.0 - 3/19/2016
      Original Release.
    1.0.1 - 4/6/2016
      Modified the cursorMove() function to start at (1,1) instead of (0,0).
      Modified the cursorMove() function to work with OLED modules correctly.
      Modified the home() function to work with the newly modified cursorMove().
      Modified the oledBegin() function to work with the Newhaven OLED modules.
    1.0.2 - 2/14/2017
      Made minor modifications to files to upgrade to Particle's Verson 2 Library format.
      Added these OLED "fade display" functions (not very useful for some types of OLED displays)
          void fadeOff();           // turns off the fade feature of the OLED
          void fadeOnce(uint8_t);   // fade out the display to off (fade time 0-16) - (on some display types, it doesn't work very well. It takes the display to half brightness and then turns off display)
          void fadeBlink(uint8_t);  // blinks the fade feature of the OLED (fade time 0-16) - (on some display types, it doesn't work very well. It takes the display to half brightness and then turns off display)


  Short Description:
    This library works with Arduino and Particle (Photon, Electron, and Core)
    microcontroller boards and it provides many functions to communicate with
    OLED and LCD character display modules that use the I2C communication
    protocol.

    The library will work with **LCD** and **OLED** character displays
    (e.g. 16x2, 20x2, 20x4, etc.). The LCD displays must use the the
    HD44780 controller chip and have a I2C PCA8574 i/o expander chip
    on a backpack board (which gives the display I2C capability).
    OLED display modules must have the US2066 controller chip
    (which has I2C built in).

    See the project details links below for installation and usage information.

    Github repositories:
    * Arduino library files:  https://github.com/wht-io/i2c-char-display-arduino.git
    * Particle library files: https://github.com/wht-io/i2c-char-display-particle.git

    Project Details:

    * Library installation and usage: http://wht.io/portfolio/i2c-display-library/
    * OLED hardware information for EastRising modules: http://wht.io/portfolio/i2c-oled-backpack-board-eastrising/
    * OLED hardware information for Newhaven modules: http://wht.io/portfolio/i2c-oled-backpack-board-newhaven/
    * LCD hardware information: http://wht.io/portfolio/i2c-lcd-backpack-board/
*/

/*
  Windy Hill Technology LLC code, firmware, and software is released under the
  MIT License (http://opensource.org/licenses/MIT).

  The MIT License (MIT)

  Copyright (c) 2016 Windy Hill Technology LLC

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/


#ifdef ARDUINO_ARCH_AVR        // if using an arduino
#include "i2c-char-display.h"
#elif SPARK                    // if using a core, photon, or electron (by particle.io)
#include "i2c-char-display.h"
#else                          // if using something else
#endif



// class constructors
I2cCharDisplay::I2cCharDisplay(uint8_t displayType, uint8_t i2cAddress, uint8_t rows)
{
  _displayType         = displayType;
  _i2cAddress          = i2cAddress;
  _rows                = rows;
  _lcdBacklightControl = LCD_BACKLIGHTON;
}


// public functions


void I2cCharDisplay::begin()
{
  switch (_displayType)
  {
  case LCD_TYPE:
    lcdBegin();
    break;

  case OLED_TYPE:
    oledBegin();
    break;

  default:

    break;
  }

}



// functions to interface with higher level Arduino and Particle functions (like Print)

// this allows the print command to be used to write to the display...
// e.g. if your display class is myLcd, then you can use  myLcd.print("hello world");  to write to the lcd
inline size_t I2cCharDisplay::write(uint8_t value)
{
  sendData(value);
  return 1;         // we have printed one character
}


// functions that work with both OLED and LCD

void I2cCharDisplay::clear()
{
  sendCommand(LCD_CLEARDISPLAYCOMMAND); // clear display
  delayMicroseconds(2000);              // 1.53ms required
}


void I2cCharDisplay::home()
{
  cursorMove(1, 1);    // use move command instead to avoid flicker on oled displays
  //  sendCommand(LCD_RETURNHOMECOMMAND); // move cursor to home
  //  delayMicroseconds(2000);            // 1.53ms required
}


// move cursor to new postion row,col  (both start at 1)
void I2cCharDisplay::cursorMove(uint8_t row, uint8_t col)
{

  if (row > _rows)              // if user points to a row too large, change row to the bottom row
  {
    row = _rows;
  }

  if (_rows <= 2)               // if we have a 1 or 2 row display
  {
    uint8_t moveRowOffset2Rows[] =  { 0x00, 0x40};
    sendCommand(LCD_SETDDRAMADDRCOMMAND | (col-1 + moveRowOffset2Rows[row-1]));
  }
  else                          // if we have a 3 or 4 line display
  {
    if (_displayType == LCD_TYPE)           // if using an LCD
    {
      uint8_t moveRowOffset4RowsLcd[] =  { 0x00, 0x40, 0x14, 0x54 };
      sendCommand(LCD_SETDDRAMADDRCOMMAND | (col-1 + moveRowOffset4RowsLcd[row-1]));
    }
    else                                    // if using an OLED
    {
      uint8_t moveRowOffset4RowsOled[] = { 0x00, 0x20, 0x40, 0x60 };
      sendCommand(LCD_SETDDRAMADDRCOMMAND | (col-1 + moveRowOffset4RowsOled[row-1]));
    }
  }
}


// display on/off
void I2cCharDisplay::displayOff()
{
  _lcdDisplayControlCommand &= ~LCD_DISPLAYON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


void I2cCharDisplay::displayOn()
{
  _lcdDisplayControlCommand |= LCD_DISPLAYON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


// cursor on/off
void I2cCharDisplay::cursorOff()
{
  _lcdDisplayControlCommand &= ~LCD_CURSORON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


void I2cCharDisplay::cursorOn()
{
  _lcdDisplayControlCommand |= LCD_CURSORON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


// cursor blink on/off
void I2cCharDisplay::cursorBlinkOff()
{
  _lcdDisplayControlCommand &= ~LCD_CURSORBLINKON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


void I2cCharDisplay::cursorBlinkOn()
{
  _lcdDisplayControlCommand |= LCD_CURSORBLINKON;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);
}


// display shift left/right (also shifts the cursor)
void I2cCharDisplay::displayShiftLeft(void)
{
  sendCommand(LCD_SHIFTCOMMAND | LCD_DISPLAYSHIFT | LCD_SHIFTLEFT);
}


void I2cCharDisplay::displayShiftRight(void)
{
  sendCommand(LCD_SHIFTCOMMAND | LCD_DISPLAYSHIFT | LCD_SHIFTRIGHT);
}


// cursor shift left/right and change the address counter
void I2cCharDisplay::cursorShiftLeft(void)
{
  sendCommand(LCD_SHIFTCOMMAND | LCD_CURSORSHIFT | LCD_SHIFTLEFT);
}


void I2cCharDisplay::cursorShiftRight(void)
{
  sendCommand(LCD_SHIFTCOMMAND | LCD_CURSORSHIFT | LCD_SHIFTRIGHT);
}


// set text to flow left to right (DEFAULT MODE)
void I2cCharDisplay::displayLeftToRight(void)
{
  _lcdEntryModeCommand |= LCD_DISPLAYLEFTTORIGHT;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);
}


// set text to flow right to left
void I2cCharDisplay::displayRightToLeft(void)
{
  _lcdEntryModeCommand &= ~LCD_DISPLAYLEFTTORIGHT;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);
}


// entire display shifts when new data is written to display
void I2cCharDisplay::displayShiftOn(void)
{
  _lcdEntryModeCommand |= LCD_DISPLAYSHIFTON;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);
}


// entire display does not shift when new data is written to display (NORMAL MODE)
void I2cCharDisplay::displayShiftOff(void)
{
  _lcdEntryModeCommand &= ~LCD_DISPLAYSHIFTON;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);
}


// Fill one of the 8 CGRAM memory addresses (0-7) to create custom characters
void I2cCharDisplay::createCharacter(uint8_t address, uint8_t characterMap[])
{
  address &= 0x7;       // limit to the first 8 addresses
  sendCommand(LCD_SETCGRAMADDRCOMMAND | (address << 3));
  for (uint8_t i = 0; i < 8; i++)
  {
    write(characterMap[i]);
  }
}


// functions specific to LCD displays



// Turn the lcd backlight off
void I2cCharDisplay::backlightOff(void)
{
  _lcdBacklightControl = LCD_BACKLIGHTOFF;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(_lcdBacklightControl));
  Wire.endTransmission();
}

// Turn the lcd backlight on
void I2cCharDisplay::backlightOn(void)
{
  _lcdBacklightControl = LCD_BACKLIGHTON;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(_lcdBacklightControl));
  Wire.endTransmission();
}


// functions specific to OLED displays

// Set the oled brightness
void I2cCharDisplay::setBrightness(uint8_t value)
{
  sendCommand(0x80);        // set RE=1
  sendCommand(0x2A);

  sendCommand(0x80);        // set SD=1
  sendCommand(0x79);

  sendCommand(OLED_SETBRIGHTNESSCOMMAND);
  sendCommand(value);

  sendCommand(0x80);        // set SD=0
  sendCommand(0x78);

  sendCommand(0x80);        // set RE=0
  sendCommand(0x28);
}

// Set the oled fade out feature to OFF
void I2cCharDisplay::fadeOff()
{
  sendCommand(0x80);        // set RE=1
  sendCommand(0x2A);

  sendCommand(0x80);        // set SD=1
  sendCommand(0x79);

  sendCommand(OLED_SETFADECOMMAND);
  sendCommand(OLED_FADEOFF);                     // set fade feature to OFF

  sendCommand(0x80);        // set SD=0
  sendCommand(0x78);

  sendCommand(0x80);        // set RE=0
  sendCommand(0x28);
}

// Set the oled fade out feature to ON
void I2cCharDisplay::fadeOnce(uint8_t value)
{
  sendCommand(0x80);        // set RE=1
  sendCommand(0x2A);

  sendCommand(0x80);        // set SD=1
  sendCommand(0x79);

  sendCommand(OLED_SETFADECOMMAND);
  sendCommand(OLED_FADEON | (0x0f & value));      // set fade feature to ON with a delay interval of value

  sendCommand(0x80);        // set SD=0
  sendCommand(0x78);

  sendCommand(0x80);        // set RE=0
  sendCommand(0x28);
}

// Set the oled fade out feature to BLINK
void I2cCharDisplay::fadeBlink(uint8_t value)
{
  sendCommand(0x80);        // set RE=1
  sendCommand(0x2A);

  sendCommand(0x80);        // set SD=1
  sendCommand(0x79);

  sendCommand(OLED_SETFADECOMMAND);
  sendCommand(OLED_FADEBLINK | (0x0f & value));      // set fade feature to BLINK with a delay interval of value

  sendCommand(0x80);        // set SD=0
  sendCommand(0x78);

  sendCommand(0x80);        // set RE=0
  sendCommand(0x28);
}



// private functions ********************************

void I2cCharDisplay::sendCommand(uint8_t value)
{
  switch (_displayType)
  {
  case LCD_TYPE:
    sendLcdCommand(value);
    break;

  case OLED_TYPE:
    sendOledCommand(value);
    break;

  default:

    break;
  }
}


void I2cCharDisplay::sendData(uint8_t value)
{
  switch (_displayType)
  {
  case LCD_TYPE:
    sendLcdData(value);
    break;

  case OLED_TYPE:
    sendOledData(value);
    break;

  default:

    break;
  }
}


// sendCommand - send command to the display
// value is what is sent
void I2cCharDisplay::sendLcdCommand(uint8_t value)
{
  // we need to break the value into 2 bytes, high nibble and a low nibble to send to lcd
  // and set the backlight bit which always need to be included.
  uint8_t dataToSend[2];

  dataToSend[0] = (value & 0xf0) | _lcdBacklightControl | LCD_COMMAND;
  dataToSend[1] = ((value << 4) & 0xf0) | _lcdBacklightControl | LCD_COMMAND;


  // write the 2 bytes of data to the display
  for (uint8_t i = 0; i < 2; ++i)
  {
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]));
    Wire.endTransmission();
    // set the enable bit and write again
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]) | LCD_ENABLEON);
    Wire.endTransmission();
    delayMicroseconds(1);               // hold enable high for 1us
    // clear the enable bit and write again
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]) | LCD_ENABLEOFF);
    Wire.endTransmission();
    delayMicroseconds(1);           //
  }
}


// sendData - send data to the display
// value is what is sent
void I2cCharDisplay::sendLcdData(uint8_t value)
{
  // we need to break the value into 2 bytes, high nibble and a low nibble to send to lcd
  // and set the backlight bit which always need to be included.
  uint8_t dataToSend[2];

  // add in the backlight setting and the data bit
  dataToSend[0] = (value & 0xf0) | _lcdBacklightControl | LCD_DATA;
  dataToSend[1] = ((value << 4) & 0xf0) | _lcdBacklightControl | LCD_DATA;


  // write the 2 bytes of data to the display
  for (uint8_t i = 0; i < 2; ++i)
  {
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]));
    Wire.endTransmission();
    // set the enable bit and write again
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]) | LCD_ENABLEON);
    Wire.endTransmission();
    delayMicroseconds(1);               // hold enable high for 1us
    // clear the enable bit and write again
    Wire.beginTransmission(_i2cAddress);
    Wire.write((int)(dataToSend[i]) | LCD_ENABLEOFF);
    Wire.endTransmission();
    delayMicroseconds(1);
  }
}


void I2cCharDisplay::sendOledCommand(uint8_t value)
{
  Wire.beginTransmission(_i2cAddress);           // **** Start I2C
  Wire.write(OLED_COMMANDMODE);                  // **** Set OLED Command mode
  Wire.write(value);
  Wire.endTransmission();                        // **** End I2C
  delay(10);
}


void I2cCharDisplay::sendOledData(uint8_t value)
{
  Wire.beginTransmission(_i2cAddress);        // **** Start I2C
  Wire.write(OLED_DATAMODE);                  // **** Set OLED Data mode
  Wire.write(value);
  Wire.endTransmission();                     // **** End I2C
}


void I2cCharDisplay::oledBegin()
{
  delay(100);       // wait for the display to power up

  // begin OLED setup
  sendCommand(0x2A); // Set RE bit (RE=1, IS=0, SD=0)

  sendCommand(0x71); // Function Selection A
  sendData(0x5C);    // 5C = enable regulator (for 5V I/O), 00 = disable regulator (for 3.3V I/O)
                     //      Leave at 5C and then you can operate at either 3.3 or 5 volts.
                     //      The current draw of the regulator is minimal

  sendCommand(0x28); // Clear RE bit (RE=0, IS=0, SD=0)

  sendCommand(0x08); // Sleep Mode On (display, cursor & blink are off) during this setup

  sendCommand(0x2A); // Set RE bit (RE=1, IS=0, SD=0)
  sendCommand(0x79); // Set SD bit (RE=1, IS=0, SD=1)

  sendCommand(0xD5); // Set Display Clock Divide Ratio/ Oscillator Frequency
  sendCommand(0x70); //     set the Freq to 70h

  sendCommand(0x78); // Clear SD bit (RE=1, IS=0, SD=0)

  // Extended Function Set:
    if (_rows > 2)
    {
      sendCommand(0x09); // Set 5x8 chars, display inversion cleared, 3/4 line display
    }
    else
    {
      sendCommand(0x08); // Set 5x8 chars, cursor inversion cleared, 1/2 line display
    }

  sendCommand(0x06); // Set Advanced Entry Mode: COM0 -> COM31, SEG99 -> SEG0

  sendCommand(0x72); // Function Selection B:
  sendData(0x00);    //    Select ROM A and CGRAM 8 (which allows for custom characters)

  sendCommand(0x79); // Set SD bit  (RE=1, IS=0, SD=1)

  sendCommand(0xDA); // Set SEG Pins Hardware Configuration:
  sendCommand(0x10); //      Enable SEG Left, Seq SEG pin config

  sendCommand(0xDC); // Function Selection C
  sendCommand(0x00); //   Internal VSL, GPIO pin HiZ, input disabled

  sendCommand(0x81); // Set Contrast (brightness)
  sendCommand(0xFF); //       max value = 0xFF

  sendCommand(0xD9); // Set Phase Length
  sendCommand(0xF1); //       Phase 2 = 15(F), Phase 1 = 1   (power on = 0x78)

  sendCommand(0xDB); // set VCOMH deselect Level
  sendCommand(0x40); //       1 x Vcc  (previously 0x30)

  // Done with OLED Setup

  sendCommand(0x78);    // Clear SD bit  (RE=1, IS=0, SD=0)
  sendCommand(0x28);    // Clear RE and IS (RE=0, IS=0, SD=0)

  sendCommand(0x01);   // clear display
  sendCommand(0x80);   // Set DDRAM Address to 0x80 (line 1 start)

  delay(100);

  // send the function set command
  _lcdFunctionSetCommand = LCD_1LINES | LCD_5x8DOTS;
  if (_rows > 1)
  {
    _lcdFunctionSetCommand |= LCD_2LINES;
  }
  sendCommand(LCD_FUNCTIONSETCOMMAND | _lcdFunctionSetCommand);

  // send the display command
  // display on, no cursor and no blinking
  _lcdDisplayControlCommand = LCD_DISPLAYON | LCD_CURSOROFF | LCD_CURSORBLINKOFF;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);

  // send the entry mode command
  _lcdEntryModeCommand = LCD_DISPLAYLEFTTORIGHT | LCD_DISPLAYSHIFTOFF;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);

  // clear display and home cursor
  clear();
  home();
}


void I2cCharDisplay::lcdBegin()
{
  uint8_t data;


/*
 * // enable i2c if it is not already
 * if (!Wire.isEnabled())
 * {
 *  Wire.begin();
 * }
 */
// initialize the lcd
  delay(100);           // wait for lcd to power up

  // set all of the outputs on the PCA8574 chip to 0, except the backlight bit if on
  data = _lcdBacklightControl;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data));
  Wire.endTransmission();
  delay(1000);

  // put lcd in 4 bit mode
  data = 0x30 | _lcdBacklightControl;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data));
  Wire.endTransmission();
  // set the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEON));
  Wire.endTransmission();
  delayMicroseconds(1);                 // hold enable high for 1us
  // clear the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEOFF));
  Wire.endTransmission();
  delayMicroseconds(1);
  delayMicroseconds(4300);  // wait min 4.1ms

  // put lcd in 4 bit mode again
  data = 0x30 | _lcdBacklightControl;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data));
  Wire.endTransmission();
  // set the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEON));
  Wire.endTransmission();
  delayMicroseconds(1);                 // hold enable high for 1us
  // clear the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEOFF));
  Wire.endTransmission();
  delayMicroseconds(1);
  delayMicroseconds(4300);   // wait min 4.1ms

  // put lcd in 4 bit mode again
  data = 0x30 | _lcdBacklightControl;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data));
  Wire.endTransmission();
  // set the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEON));
  Wire.endTransmission();
  delayMicroseconds(1);                 // hold enable high for 1us
  // clear the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEOFF));
  Wire.endTransmission();
  delayMicroseconds(1);
  delayMicroseconds(4300);   // wait min 4.1ms


  // set up 4 bit interface
  data = 0x20 | _lcdBacklightControl;
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data));
  Wire.endTransmission();
  // set the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEON));
  Wire.endTransmission();
  delayMicroseconds(1);                 // hold enable high for 1us
  // clear the enable bit and write again
  Wire.beginTransmission(_i2cAddress);
  Wire.write((int)(data | LCD_ENABLEOFF));
  Wire.endTransmission();
  delayMicroseconds(1);



  // send the function set command
  _lcdFunctionSetCommand = LCD_4BITMODE | LCD_1LINES | LCD_5x8DOTS;
  if (_rows > 1)
  {
    _lcdFunctionSetCommand |= LCD_2LINES;
  }
  sendCommand(LCD_FUNCTIONSETCOMMAND | _lcdFunctionSetCommand);

  // send the display command
  // display on, no cursor and no blinking
  _lcdDisplayControlCommand = LCD_DISPLAYON | LCD_CURSOROFF | LCD_CURSORBLINKOFF;
  sendCommand(LCD_DISPLAYCONTROLCOMMAND | _lcdDisplayControlCommand);

  // send the entry mode command
  _lcdEntryModeCommand = LCD_DISPLAYLEFTTORIGHT | LCD_DISPLAYSHIFTOFF;
  sendCommand(LCD_ENTRYMODECOMMAND | _lcdEntryModeCommand);

  // clear display and home cursor
  clear();
  home();
}