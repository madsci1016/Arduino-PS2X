#include "PS2X_lib.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#ifdef __AVR__
#include <avr/io.h>
#endif
#if ARDUINO > 22
  #include "Arduino.h"
#else
  #include "WProgram.h"
  #include "pins_arduino.h"
#endif

static byte enter_config[]={0x01,0x43,0x00,0x01,0x00};
static byte set_mode[]={0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00};
static byte set_bytes_large[]={0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
static byte exit_config[]={0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
static byte enable_rumble[]={0x01,0x4D,0x00,0x00,0x01};
static byte type_read[]={0x01,0x45,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};

/****************************************************************************************/
boolean PS2X::NewButtonState() {
  return ((last_buttons ^ buttons) > 0);
}

/****************************************************************************************/
boolean PS2X::NewButtonState(unsigned int button) {
  return (((last_buttons ^ buttons) & button) > 0);
}

/****************************************************************************************/
boolean PS2X::ButtonPressed(unsigned int button) {
  return(NewButtonState(button) & Button(button));
}

/****************************************************************************************/
boolean PS2X::ButtonReleased(unsigned int button) {
  return((NewButtonState(button)) & ((~last_buttons & button) > 0));
}

/****************************************************************************************/
boolean PS2X::Button(uint16_t button) {
  return ((~buttons & button) > 0);
}

/****************************************************************************************/
unsigned int PS2X::ButtonDataByte() {
   return (~buttons);
}

/****************************************************************************************/
byte PS2X::Analog(byte button) {
   return PS2data[button];
}

/****************************************************************************************/
unsigned char PS2X::_gamepad_shiftinout (char byte) {
  if(_spi == NULL) {
    /* software SPI */
    unsigned char tmp = 0;

   for(unsigned char i=0;i<8;i++) {
      if(CHK(byte,i)) CMD_SET();
      else CMD_CLR();
	  
      CLK_CLR();
      delayMicroseconds(CTRL_CLK);

      //if(DAT_CHK()) SET(tmp,i);
      if(DAT_CHK()) bitSet(tmp,i);

      CLK_SET();
      delayMicroseconds(CTRL_CLK);

   }
   CMD_SET();
   delayMicroseconds(CTRL_BYTE_DELAY);
   return tmp;
  } else {
    unsigned char tmp = _spi->transfer(byte); // hardware SPI
    delayMicroseconds(CTRL_BYTE_DELAY);
    return tmp;
  }

}

/****************************************************************************************/
void PS2X::read_gamepad() {
   read_gamepad(false, 0x00);
}

/****************************************************************************************/
boolean PS2X::read_gamepad(boolean motor1, byte motor2) {
   double temp = millis() - last_read;

   if (temp > 1500) //waited to long
      reconfig_gamepad();

   if(temp < read_delay)  //waited too short
      delay(read_delay - temp);

   if(motor2 != 0x00)
      motor2 = map(motor2,0,255,0x40,0xFF); //noting below 40 will make it spin

   byte dword[9] = {0x01,0x42,0,motor1,motor2,0,0,0,0};
   byte dword2[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

   // Try a few times to get valid data...
   for (byte RetryCnt = 0; RetryCnt < 5; RetryCnt++) {
      BEGIN_SPI();
      //Send the command to send button and joystick data;
      for (int i = 0; i<9; i++) {
         PS2data[i] = _gamepad_shiftinout(dword[i]);
      }

      if(PS2data[1] == 0x79) {  //if controller is in full data return mode, get the rest of data
         for (int i = 0; i<12; i++) {
            PS2data[i+9] = _gamepad_shiftinout(dword2[i]);
         }
      }

      END_SPI();

      // Check to see if we received valid data or not.  
	  // We should be in analog mode for our data to be valid (analog == 0x7_)
      if ((PS2data[1] & 0xf0) == 0x70)
         break;

      // If we got to here, we are not in analog mode, try to recover...
      reconfig_gamepad(); // try to get back into Analog mode.
      delay(read_delay);
   }

   // If we get here and still not in analog mode (=0x7_), try increasing the read_delay...
   if ((PS2data[1] & 0xf0) != 0x70) {
      if (read_delay < 10)
         read_delay++;   // see if this helps out...
   }

#ifdef PS2X_COM_DEBUG
   Serial.print("OUT:IN ");
   for(int i=0; i<9; i++){
      Serial.print(dword[i], HEX);
      Serial.print(":");
      Serial.print(PS2data[i], HEX);
      Serial.print(" ");
   }
   for (int i = 0; i<12; i++) {
      Serial.print(dword2[i], HEX);
      Serial.print(":");
      Serial.print(PS2data[i+9], HEX);
      Serial.print(" ");
   }
   Serial.println("");
#endif

   last_buttons = buttons; //store the previous buttons states

#if defined(__AVR__)
   buttons = *(uint16_t*)(PS2data+3);   //store as one value for multiple functions
#else
   buttons =  (uint16_t)(PS2data[4] << 8) + PS2data[3];   //store as one value for multiple functions
#endif
   last_read = millis();
   return ((PS2data[1] & 0xf0) == 0x70);  // 1 = OK = analog mode - 0 = NOK
}

/****************************************************************************************/
byte PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {
   return config_gamepad(clk, cmd, att, dat, false, false);
}

/****************************************************************************************/
byte PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat, bool pressures, bool rumble) {
#if defined(HAVE_PORTREG_IO)
  _clk_mask = (port_mask_t) digitalPinToBitMask(clk);
  _clk_oreg = (port_reg_t*) portOutputRegister(digitalPinToPort(clk));
  _cmd_mask = (port_mask_t) digitalPinToBitMask(cmd);
  _cmd_oreg = (port_reg_t*) portOutputRegister(digitalPinToPort(cmd));
  _att_mask = (port_mask_t) digitalPinToBitMask(att);
  _att_oreg = (port_reg_t*) portOutputRegister(digitalPinToPort(att));
  _dat_mask = (port_mask_t) digitalPinToBitMask(dat);
  _dat_ireg = (port_reg_t*) portInputRegister(digitalPinToPort(dat));
#elif defined(HAVE_PORTREG_SC) // well it seems that this varies from platform to platform so...
#if defined(__PIC32__)
  uint32_t            lport;                   // Port number for this pin
  _clk_mask = (port_mask_t) digitalPinToBitMask(clk);
  lport = digitalPinToPort(clk);
  _clk_lport_set = (port_reg_t*) portOutputRegister(lport) + 2;
  _clk_lport_clr = (port_reg_t*) portOutputRegister(lport) + 1;

  _cmd_mask = (port_mask_t) digitalPinToBitMask(cmd);
  lport = digitalPinToPort(cmd);
  _cmd_lport_set = (port_reg_t*) portOutputRegister(lport) + 2;
  _cmd_lport_clr = (port_reg_t*) portOutputRegister(lport) + 1;

  _att_mask = (port_mask_t) digitalPinToBitMask(att);
  lport = digitalPinToPort(att);
  _att_lport_set = (port_reg_t*) portOutputRegister(lport) + 2;
  _att_lport_clr = (port_reg_t*) portOutputRegister(lport) + 1;

  _dat_mask = (port_mask_t) digitalPinToBitMask(dat);
  _dat_lport = (port_reg_t*) portInputRegister(digitalPinToPort(dat));
#endif
#else
  _clk_pin = clk;
  _cmd_pin = cmd;
  _att_pin = att;
  _dat_pin = dat;
#endif

  pinMode(clk, OUTPUT); //configure ports
  pinMode(att, OUTPUT); ATT_SET();
  pinMode(cmd, OUTPUT);
  pinMode(dat, INPUT_PULLUP); // enable pull-up

  // CMD_SET(); // SET(*_cmd_oreg,_cmd_mask);
  CLK_SET();

  return config_gamepad_stub(pressures, rumble);
}

byte PS2X::config_gamepad(SPIClass* spi, uint8_t att) {
  return config_gamepad(spi, att, false, false, true);
}

byte PS2X::config_gamepad(SPIClass* spi, uint8_t att, bool begin) {
  return config_gamepad(spi, att, false, false, begin);
}

byte PS2X::config_gamepad(SPIClass* spi, uint8_t att, bool pressures, bool rumble) {
  return config_gamepad(spi, att, pressures, rumble, true);
}

byte PS2X::config_gamepad(SPIClass* spi, uint8_t att, bool pressures, bool rumble, bool begin) {
  _spi = spi;
  #if defined(HAVE_PORTREG_IO)
  _att_mask = (port_mask_t) digitalPinToBitMask(att);
  _att_oreg = (port_reg_t*) portOutputRegister(digitalPinToPort(att));
#elif defined(HAVE_PORTREG_SC) // well it seems that this varies from platform to platform so...
#if defined(__PIC32__)
  uint32_t            lport;                   // Port number for this pin
  _att_mask = (port_mask_t) digitalPinToBitMask(att);
  lport = digitalPinToPort(att);
  _att_lport_set = (port_reg_t*) portOutputRegister(lport) + 2;
  _att_lport_clr = (port_reg_t*) portOutputRegister(lport) + 1;
#endif
#else
  _att_pin = att;
#endif

  pinMode(att, OUTPUT); ATT_SET();

#if defined(SPI_HAS_TRANSACTION)
  _spi_settings = SPISettings(CTRL_BITRATE, LSBFIRST, SPI_MODE2);
#endif

  if(begin) _spi->begin(); // begin SPI with default settings

  /* some hardware SPI implementations incorrectly hold CLK low before the first transaction, so we'll try to fix that */
  BEGIN_SPI_NOATT();
  _spi->transfer(0x55); // anything will work here
  END_SPI_NOATT();

  return config_gamepad_stub(pressures, rumble);
}

byte PS2X::config_gamepad_stub(bool pressures, bool rumble) {
  byte temp[sizeof(type_read)];


  //new error checking. First, read gamepad a few times to see if it's talking
  read_gamepad();
  read_gamepad();

  //see if it talked - see if mode came back. 
  //If still anything but 41, 73 or 79, then it's not talking
  if(PS2data[1] != 0x41 && PS2data[1] != 0x42 && PS2data[1] != 0x73 && PS2data[1] != 0x79){ 
#ifdef PS2X_DEBUG
    Serial.println("Controller mode not matched or no controller found");
    Serial.print("Expected 0x41, 0x42, 0x73 or 0x79, but got ");
    Serial.println(PS2data[1], HEX);
#endif
    return 1; //return error code 1
  }

  //try setting mode, increasing delays if need be.
  read_delay = 1;

  t_last_att = millis() + CTRL_PACKET_DELAY; // start reading right away

  for(int y = 0; y <= 10; y++) {
    sendCommandString(enter_config, sizeof(enter_config)); //start config run

    //read type
    delayMicroseconds(CTRL_BYTE_DELAY);

    //CLK_SET(); // CLK should've been set to HIGH already
    BEGIN_SPI();


    for (int i = 0; i<9; i++) {
      temp[i] = _gamepad_shiftinout(type_read[i]);
    }

    END_SPI();


    controller_type = temp[3];

    sendCommandString(set_mode, sizeof(set_mode));
    if(rumble){ sendCommandString(enable_rumble, sizeof(enable_rumble)); en_Rumble = true; }
    if(pressures){ sendCommandString(set_bytes_large, sizeof(set_bytes_large)); en_Pressures = true; }
    sendCommandString(exit_config, sizeof(exit_config));

    read_gamepad();

    if(pressures){
      if(PS2data[1] == 0x79)
        break;
      if(PS2data[1] == 0x73)
        return 3;
    }

    if(PS2data[1] == 0x73)
      break;

    if(y == 10){
#ifdef PS2X_DEBUG
      Serial.println("Controller not accepting commands");
      Serial.print("mode still set at");
      Serial.println(PS2data[1], HEX);
#endif
      return 2; //exit function with error
    }
    read_delay += 1; //add 1ms to read_delay
  }
  return 0; //no error if here
}

/****************************************************************************************/
void PS2X::sendCommandString(byte string[], byte len) {
#ifdef PS2X_COM_DEBUG
  byte temp[len];
  BEGIN_SPI();

  for (int y=0; y < len; y++)
    temp[y] = _gamepad_shiftinout(string[y]);

  END_SPI();

  delay(read_delay); //wait a few

  Serial.println("OUT:IN Configure");
  for(int i=0; i<len; i++) {
    Serial.print(string[i], HEX);
    Serial.print(":");
    Serial.print(temp[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#else
  BEGIN_SPI();
  for (int y=0; y < len; y++)
    _gamepad_shiftinout(string[y]);
  END_SPI();

  delay(read_delay);                  //wait a few
#endif
}

/****************************************************************************************/
byte PS2X::readType() {
/*
  byte temp[sizeof(type_read)];

  sendCommandString(enter_config, sizeof(enter_config));

  delayMicroseconds(CTRL_BYTE_DELAY);

  CMD_SET();
  CLK_SET();
  ATT_CLR(); // low enable joystick

  delayMicroseconds(CTRL_BYTE_DELAY);

  for (int i = 0; i<9; i++) {
    temp[i] = _gamepad_shiftinout(type_read[i]);
  }

  sendCommandString(exit_config, sizeof(exit_config));

  if(temp[3] == 0x03)
    return 1;
  else if(temp[3] == 0x01)
    return 2;

  return 0;
*/
  Serial.print("Controller_type: ");
  Serial.println(controller_type, HEX);
  if(controller_type == 0x03)
    return 1;
  else if(controller_type == 0x01 && PS2data[1] == 0x42)
	return 4;
  else if(controller_type == 0x01 && PS2data[1] != 0x42)
    return 2;
  else if(controller_type == 0x0C)  
    return 3;  //2.4G Wireless Dual Shock PS2 Game Controller
	
  return 0;
}

/****************************************************************************************/
void PS2X::enableRumble() {
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(enable_rumble, sizeof(enable_rumble));
  sendCommandString(exit_config, sizeof(exit_config));
  en_Rumble = true;
}

/****************************************************************************************/
bool PS2X::enablePressures() {
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(set_bytes_large, sizeof(set_bytes_large));
  sendCommandString(exit_config, sizeof(exit_config));

  read_gamepad();
  read_gamepad();

  if(PS2data[1] != 0x79)
    return false;

  en_Pressures = true;
    return true;
}

/****************************************************************************************/
void PS2X::reconfig_gamepad(){
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(set_mode, sizeof(set_mode));
  if (en_Rumble)
    sendCommandString(enable_rumble, sizeof(enable_rumble));
  if (en_Pressures)
    sendCommandString(set_bytes_large, sizeof(set_bytes_large));
  sendCommandString(exit_config, sizeof(exit_config));
}

/****************************************************************************************/
#if defined(HAVE_PORTREG_IO)
inline void  PS2X::CLK_SET(void) {
#if defined(__AVR__) // there should be a platform-independent way to do this
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_clk_oreg |= _clk_mask;
#if defined(__AVR__)
  SREG = old_sreg;
#endif
}

inline void  PS2X::CLK_CLR(void) {
#if defined(__AVR__)
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_clk_oreg &= ~_clk_mask;
#if defined(__AVR__)
  SREG = old_sreg;
#endif
}

inline void  PS2X::CMD_SET(void) {
#if defined(__AVR__)
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_cmd_oreg |= _cmd_mask; // SET(*_cmd_oreg,_cmd_mask);
#if defined(__AVR__)
  SREG = old_sreg;
#endif
}

inline void  PS2X::CMD_CLR(void) {
#if defined(__AVR__)
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_cmd_oreg &= ~_cmd_mask; // SET(*_cmd_oreg,_cmd_mask);
#if defined(__AVR__)
  SREG = old_sreg;
#endif
}

inline void  PS2X::ATT_SET(void) {
#if defined(__AVR__)
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_att_oreg |= _att_mask ;
#if defined(__AVR__)
  SREG = old_sreg;
#endif
}

inline void PS2X::ATT_CLR(void) {
#if defined(__AVR__)
  register uint8_t old_sreg = SREG;
  cli();
#endif
  *_att_oreg &= ~_att_mask;
#if defined(__AVR__)
  SREG = old_sreg;
#endif

}

inline bool PS2X::DAT_CHK(void) {
  return (*_dat_ireg & _dat_mask) ? true : false;
}

#elif defined(HAVE_PORTREG_SC)
inline void  PS2X::CLK_SET(void) {
  *_clk_lport_set |= _clk_mask;
}

inline void  PS2X::CLK_CLR(void) {
  *_clk_lport_clr |= _clk_mask;
}

inline void  PS2X::CMD_SET(void) {
  *_cmd_lport_set |= _cmd_mask;
}

inline void  PS2X::CMD_CLR(void) {
  *_cmd_lport_clr |= _cmd_mask;
}

inline void  PS2X::ATT_SET(void) {
  *_att_lport_set |= _att_mask;
}

inline void PS2X::ATT_CLR(void) {
  *_att_lport_clr |= _att_mask;
}

inline bool PS2X::DAT_CHK(void) {
  return (*_dat_lport & _dat_mask) ? true : false;
}
#else
inline void  PS2X::CLK_SET(void) {
  digitalWrite(_clk_pin, HIGH);
}

inline void  PS2X::CLK_CLR(void) {
  digitalWrite(_clk_pin, LOW);
}

inline void  PS2X::CMD_SET(void) {
  digitalWrite(_cmd_pin, HIGH);
}

inline void  PS2X::CMD_CLR(void) {
  digitalWrite(_cmd_pin, LOW);
}

inline void  PS2X::ATT_SET(void) {
  digitalWrite(_att_pin, HIGH);
}

inline void PS2X::ATT_CLR(void) {
  digitalWrite(_att_pin, LOW);
}

inline bool PS2X::DAT_CHK(void) {
  return digitalRead(_dat_pin) ? true : false;
}

#endif

inline void PS2X::BEGIN_SPI_NOATT(void) {
  if(_spi != NULL) {
#if defined(SPI_HAS_TRANSACTION)
    _spi->beginTransaction(_spi_settings);
#else
    // _spi->begin();
    _spi->setBitOrder(LSBFIRST);
    _spi->setDataMode(SPI_MODE2);
#if defined(__AVR__)
    _spi->setClockDivider(CTRL_DIVIDER);
#elif defined(__SAM3X8E__)
    _spi->setClockDivider(F_CPU / CTRL_BITRATE);
#else
    #error Unsupported method of setting clock divider without transaction, please update this library to support this platform, update the platform code to support SPI transaction, or use software SPI.
#endif
#endif
  } else {
    CMD_CLR();
    CLK_SET();
  }
}

inline void PS2X::BEGIN_SPI(void) {
  BEGIN_SPI_NOATT();
  while(millis() - t_last_att < CTRL_PACKET_DELAY);
  ATT_CLR(); // low enable joystick
  delayMicroseconds(CTRL_BYTE_DELAY);
}

inline void PS2X::END_SPI_NOATT(void) {
  if(_spi != NULL) {
#if defined(SPI_HAS_TRANSACTION)
    _spi->endTransaction();
#else
    // _spi->end();
#endif
  } else {
    CMD_CLR();
    CLK_SET();
  }
}

inline void PS2X::END_SPI(void) {
  ATT_SET();
  END_SPI_NOATT();
  t_last_att = millis();
}

/****************************************************************************************/
byte PS2X::config_gamepad_arduino_spi(uint8_t att) {
  return config_gamepad_arduino_spi(att, false, false);
}

byte PS2X::config_gamepad_arduino_spi(uint8_t att, bool pressures, bool rumble) {
  return config_gamepad(&SPI, att, pressures, rumble);
}

#if defined(ESP32)
byte PS2X::config_gamepad_esp32_hspi(uint8_t att) {
  return config_gamepad_esp32_hspi(att, false, false);
}

byte PS2X::config_gamepad_esp32_hspi(uint8_t att, bool pressures, bool rumble) {
  return config_gamepad(new SPIClass(HSPI), att, pressures, rumble);
}

byte PS2X::config_gamepad_esp32_hspi(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {
  return config_gamepad_esp32_hspi(clk, cmd, att, dat, false, false);
}

byte PS2X::config_gamepad_esp32_hspi(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat, bool pressures, bool rumble) {
  SPIClass* spi_class = new SPIClass(HSPI);
  spi_class->begin(clk, dat, cmd, att);
  return config_gamepad(spi_class, att, pressures, rumble, false);
}

byte PS2X::config_gamepad_esp32_vspi(uint8_t att) {
  return config_gamepad_esp32_vspi(att, false, false);
}

byte PS2X::config_gamepad_esp32_vspi(uint8_t att, bool pressures, bool rumble) {
  return config_gamepad(new SPIClass(VSPI), att, pressures, rumble);
}

byte PS2X::config_gamepad_esp32_vspi(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {
  return config_gamepad_esp32_vspi(clk, cmd, att, dat, false, false);
}

byte PS2X::config_gamepad_esp32_vspi(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat, bool pressures, bool rumble) {
  SPIClass* spi_class = new SPIClass(VSPI);
  spi_class->begin(clk, dat, cmd, att);
  return config_gamepad(spi_class, att, pressures, rumble, false);
}
#endif

