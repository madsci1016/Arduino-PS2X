/******************************************************************
*  Super amazing PS2 controller Arduino Library v1.8
*		details and example sketch: 
*			http://www.billporter.info/?p=240
*
*    Original code by Shutter on Arduino Forums
*
*    Revamped, made into lib by and supporting continued development:
*              Bill Porter
*              www.billporter.info
*
*	 Contributers:
*		Eric Wetzel (thewetzel@gmail.com)
*		Kurt Eckhardt
*
*  Lib version history
*    0.1 made into library, added analog stick support. 
*    0.2 fixed config_gamepad miss-spelling
*        added new functions:
*          NewButtonState();
*          NewButtonState(unsigned int);
*          ButtonPressed(unsigned int);
*          ButtonReleased(unsigned int);
*        removed 'PS' from beginning of ever function
*    1.0 found and fixed bug that wasn't configuring controller
*        added ability to define pins
*        added time checking to reconfigure controller if not polled enough
*        Analog sticks and pressures all through 'ps2x.Analog()' function
*        added:
*          enableRumble();
*          enablePressures();
*    1.1  
*        added some debug stuff for end user. Reports if no controller found
*        added auto-increasing sentence delay to see if it helps compatibility.
*    1.2
*        found bad math by Shutter for original clock. Was running at 50kHz, not the required 500kHz. 
*        fixed some of the debug reporting. 
*	1.3 
*	    Changed clock back to 50kHz. CuriousInventor says it's suppose to be 500kHz, but doesn't seem to work for everybody. 
*	1.4
*		Removed redundant functions.
*		Fixed mode check to include two other possible modes the controller could be in.
*       Added debug code enabled by compiler directives. See below to enable debug mode.
*		Added button definitions for shapes as well as colors.
*	1.41
*		Some simple bug fixes
*		Added Keywords.txt file
*	1.5
*		Added proper Guitar Hero compatibility
*		Fixed issue with DEBUG mode, had to send serial at once instead of in bits
*	1.6
*		Changed config_gamepad() call to include rumble and pressures options
*			This was to fix controllers that will only go into config mode once
*			Old methods should still work for backwards compatibility 
*    1.7
*		Integrated Kurt's fixes for the interrupts messing with servo signals
*		Reorganized directory so examples show up in Arduino IDE menu
*    1.8
*		Added Arduino 1.0 compatibility. 
*    1.9
*       Kurt - Added detection and recovery from dropping from analog mode, plus
*       integrated Chipkit (pic32mx...) support
*
*
*
*This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
<http://www.gnu.org/licenses/>
*  
******************************************************************/

// $$$$$$$$$$$$ DEBUG ENABLE SECTION $$$$$$$$$$$$$$$$
// to debug ps2 controller, uncomment these two lines to print out debug to uart
//#define PS2X_DEBUG
//#define PS2X_COM_DEBUG

#ifndef PS2X_lib_h
  #define PS2X_lib_h

#if ARDUINO > 22
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <SPI.h>

/* SPI timing configuration */
#define CTRL_BITRATE        250000UL // SPI bitrate (Hz). Please note that on AVR Arduinos, the lowest bitrate possible is 125kHz.
#if (1000000UL / (2 * CTRL_BITRATE) > 0)
#define CTRL_CLK      (1000000UL / (2 * CTRL_BITRATE)) // delay duration between SCK high and low
#else
#define CTRL_CLK      1
#endif
#define CTRL_BYTE_DELAY     10 // delay duration between byte reads (uS)
#define CTRL_PACKET_DELAY   16 // delay duration between packets (mS) - according to playstation.txt this should be set to 16mS, but it seems that it can go down to 4mS without problems
#if !defined(SPI_HAS_TRANSACTION) && defined(__AVR__)
// SPI divider for AVR
#if (F_CPU / CTRL_BITRATE < 3)
#define CTRL_DIVIDER    SPI_CLOCK_DIV2
#elif (F_CPU / CTRL_BITRATE < 6)
#define CTRL_DIVIDER    SPI_CLOCK_DIV4
#elif (F_CPU / CTRL_BITRATE < 12)
#define CTRL_DIVIDER    SPI_CLOCK_DIV8
#elif (F_CPU / CTRL_BITRATE < 24)
#define CTRL_DIVIDER    SPI_CLOCK_DIV16
#elif (F_CPU / CTRL_BITRATE < 48)
#define CTRL_DIVIDER    SPI_CLOCK_DIV32
#elif (F_CPU / CTRL_BITRATE < 96)
#define CTRL_DIVIDER    SPI_CLOCK_DIV64
#else
#define CTRL_DIVIDER    SPI_CLOCK_DIV128
#endif
#endif

/* port register data types */
#if defined(__AVR__)
typedef volatile uint8_t port_reg_t;
typedef uint8_t port_mask_t;
#define HAVE_PORTREG_IO
#elif defined(__SAM3X8E__)
typedef volatile RwReg port_reg_t;
typedef uint32_t port_mask_t;
#define HAVE_PORTREG_IO
#elif defined(__PIC32__) // TODO: is this how we're supposed to detect PIC32?
typedef volatile uint32_t port_reg_t;
typedef uint16_t port_mask_t;
#define HAVE_PORTREG_SC
#elif (defined(__arm__) || defined(ARDUINO_FEATHER52)) &&                      \
    !defined(ARDUINO_ARCH_MBED) && !defined(ARDUINO_ARCH_RP2040)
typedef volatile uint32_t port_reg_t;
typedef uint32_t port_mask_t;
#define HAVE_PORTREG_IO

#endif

//These are our button constants
#define PSB_SELECT      0x0001
#define PSB_L3          0x0002
#define PSB_R3          0x0004
#define PSB_START       0x0008
#define PSB_PAD_UP      0x0010
#define PSB_PAD_RIGHT   0x0020
#define PSB_PAD_DOWN    0x0040
#define PSB_PAD_LEFT    0x0080
#define PSB_L2          0x0100
#define PSB_R2          0x0200
#define PSB_L1          0x0400
#define PSB_R1          0x0800
#define PSB_GREEN       0x1000
#define PSB_RED         0x2000
#define PSB_BLUE        0x4000
#define PSB_PINK        0x8000
#define PSB_TRIANGLE    0x1000
#define PSB_CIRCLE      0x2000
#define PSB_CROSS       0x4000
#define PSB_SQUARE      0x8000

//Guitar  button constants
#define UP_STRUM		0x0010
#define DOWN_STRUM		0x0040
#define LEFT_STRUM		0x0080
#define RIGHT_STRUM		0x0020
#define STAR_POWER		0x0100
#define GREEN_FRET		0x0200
#define YELLOW_FRET		0x1000
#define RED_FRET		0x2000
#define BLUE_FRET		0x4000
#define ORANGE_FRET		0x8000
#define WHAMMY_BAR		8

//These are stick values
#define PSS_RX 5
#define PSS_RY 6
#define PSS_LX 7
#define PSS_LY 8

//These are analog buttons
#define PSAB_PAD_RIGHT    9
#define PSAB_PAD_UP      11
#define PSAB_PAD_DOWN    12
#define PSAB_PAD_LEFT    10
#define PSAB_L2          19
#define PSAB_R2          20
#define PSAB_L1          17
#define PSAB_R1          18
#define PSAB_GREEN       13
#define PSAB_RED         14
#define PSAB_BLUE        15
#define PSAB_PINK        16
#define PSAB_TRIANGLE    13
#define PSAB_CIRCLE      14
#define PSAB_CROSS       15
#define PSAB_SQUARE      16

#define SET(x,y) (x|=(1<<y))
#define CLR(x,y) (x&=(~(1<<y)))
#define CHK(x,y) (x & (1<<y))
#define TOG(x,y) (x^=(1<<y))

class PS2X {
  public:
    boolean Button(uint16_t);                //will be TRUE if button is being pressed
    unsigned int ButtonDataByte();
    boolean NewButtonState();
    boolean NewButtonState(unsigned int);    //will be TRUE if button was JUST pressed OR released
    boolean ButtonPressed(unsigned int);     //will be TRUE if button was JUST pressed
    boolean ButtonReleased(unsigned int);    //will be TRUE if button was JUST released
    void read_gamepad();
    boolean  read_gamepad(boolean, byte);
    byte readType();
    /* config_gamepad for software SPI */
    byte config_gamepad(uint8_t, uint8_t, uint8_t, uint8_t); // specify pins, pressure and rumble disabled
    byte config_gamepad(uint8_t, uint8_t, uint8_t, uint8_t, bool, bool); // specify pins AND pressure&rumble mode
    /* config_gamepad for hardware SPI */
    byte config_gamepad(SPIClass*, uint8_t); // specify SPIClass and ATT pin, begins SPI by itself
    byte config_gamepad(SPIClass*, uint8_t, bool); // specify SPIClass and ATT pin, as well as whether to begin SPI
    byte config_gamepad(SPIClass*, uint8_t, bool, bool); // specify SPIClass, ATT pin and pressure&rumble mode, begins SPI by itself
    byte config_gamepad(SPIClass*, uint8_t, bool, bool, bool); // specify SPIClass, ATT pin, pressure&rumble mode, and whether to begin SPI
    // ready-to-use config functions for select boards (right now only supports Arduino with default SPI port and ESP32 HSPI and VSPI)
    byte config_gamepad_arduino_spi(uint8_t); // specify ATT pin. please note that using this on ESP32 is functionally similar to config_gamepad_esp32_vspi(uint8_t)
    byte config_gamepad_arduino_spi(uint8_t, bool, bool); // specify ATT pin and pressure&rumble mode. please note that using this on ESP32 is functionally similar to config_gamepad_esp32_vspi(uint8_t, bool, bool)
#if defined(ESP32)
    // HSPI
    byte config_gamepad_esp32_hspi(uint8_t); // use HSPI with custom ATT pin
    byte config_gamepad_esp32_hspi(uint8_t, bool, bool); // use HSPI with custom ATT pin, also specify whether to enable pressure and rumble
    byte config_gamepad_esp32_hspi(uint8_t, uint8_t, uint8_t, uint8_t); // use HSPI with custom pins
    byte config_gamepad_esp32_hspi(uint8_t, uint8_t, uint8_t, uint8_t, bool, bool); // use HSPI with custom pins, also specify whether to enable pressure and rumble
    // VSPI
    byte config_gamepad_esp32_vspi(uint8_t); // use VSPI with custom ATT pin
    byte config_gamepad_esp32_vspi(uint8_t, bool, bool); // use VSPI with custom ATT pin, also specify whether to enable pressure and rumble
    byte config_gamepad_esp32_vspi(uint8_t, uint8_t, uint8_t, uint8_t); // use VSPI with custom pins
    byte config_gamepad_esp32_vspi(uint8_t, uint8_t, uint8_t, uint8_t, bool, bool); // use VSPI with custom pins, also specify whether to enable pressure and rumble
#endif

    void enableRumble();
    bool enablePressures();
    byte Analog(byte);
    void reconfig_gamepad();

  private:
    inline void CLK_SET(void);
    inline void CLK_CLR(void);
    inline void CMD_SET(void);
    inline void CMD_CLR(void);
    inline void ATT_SET(void);
    inline void ATT_CLR(void);
    inline bool DAT_CHK(void);

    inline void BEGIN_SPI_NOATT(void);
    inline void END_SPI_NOATT(void);

    inline void BEGIN_SPI(void);
    inline void END_SPI(void);
    
    byte config_gamepad_stub(bool, bool); // common gamepad initialization sequence

    unsigned char _gamepad_shiftinout (char);
    unsigned char PS2data[21];
    void sendCommandString(byte*, byte);
    unsigned char i;
    unsigned int last_buttons;
    unsigned int buttons;

    /* pin I/O configuration, mostly relevant to software SPI support (except ATT which is used in both software and hardware SPI) */
    #if defined(HAVE_PORTREG_IO) // platform has port registers in input/output configuration (eg. AVR, STM32)
      port_mask_t _clk_mask; 
      port_reg_t *_clk_oreg;
      port_mask_t _cmd_mask; 
      port_reg_t *_cmd_oreg;
      port_mask_t _att_mask; 
      port_reg_t *_att_oreg;
      port_mask_t _dat_mask; 
      port_reg_t *_dat_ireg;
    #elif defined(HAVE_PORTREG_SC) // platform has port registers in set/clear configuration (eg. PIC32)
      port_mask_t _clk_mask; 
      port_reg_t *_clk_lport_set;
      port_reg_t *_clk_lport_clr;
      port_mask_t _cmd_mask; 
      port_reg_t *_cmd_lport_set;
      port_reg_t *_cmd_lport_clr;
      port_mask_t _att_mask; 
      port_reg_t *_att_lport_set;
      port_reg_t *_att_lport_clr;
      port_mask_t _dat_mask; 
      port_reg_t *_dat_lport;
    #else // platform does not have port registers (eg. ESP8266, ESP32)

      int _clk_pin;
      int _cmd_pin;
      int _att_pin;
      int _dat_pin;
    #endif

    /* SPI configuration */
    SPIClass* _spi; // hardware SPI class (null = software SPI)
    #if defined(SPI_HAS_TRANSACTION)
      SPISettings _spi_settings; // hardware SPI transaction settings
    #endif

    volatile unsigned long t_last_att; // time since last ATT inactive

	
    unsigned long last_read;
    byte read_delay;
    byte controller_type;
    boolean en_Rumble;
    boolean en_Pressures;
};

#endif



