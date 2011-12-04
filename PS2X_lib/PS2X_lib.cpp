#include "PS2X_lib.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>
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

boolean PS2X::NewButtonState() {
   return ((last_buttons ^ buttons) > 0);

}

boolean PS2X::NewButtonState(unsigned int button) {
  return (((last_buttons ^ buttons) & button) > 0);
}

boolean PS2X::ButtonPressed(unsigned int button) {
  return(NewButtonState(button) & Button(button));
}

boolean PS2X::ButtonReleased(unsigned int button) {
  return((NewButtonState(button)) & ((~last_buttons & button) > 0));
}
  
boolean PS2X::Button(uint16_t button) {
   return ((~buttons & button) > 0);
}

unsigned int PS2X::ButtonDataByte() {
   return (~buttons);
}

byte PS2X::Analog(byte button) {
  return PS2data[button];
}
unsigned char PS2X::_gamepad_shiftinout (char byte) {
   uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts
  

   unsigned char tmp = 0;
   cli();                          // *** KJE *** disable for now
   for(i=0;i<8;i++) {

	  if(CHK(byte,i)) SET(*_cmd_oreg,_cmd_mask);
	  else  CLR(*_cmd_oreg,_cmd_mask);
	  CLR(*_clk_oreg,_clk_mask);

      SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	  delayMicroseconds(CTRL_CLK);
	  cli();	// *** KJE ***

	  if(CHK(*_dat_ireg,_dat_mask)) SET(tmp,i);
	  SET(*_clk_oreg,_clk_mask);
   }
   SET(*_cmd_oreg,_cmd_mask);
   SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
   delayMicroseconds(CTRL_BYTE_DELAY);
   return tmp;
}

void PS2X::read_gamepad() {
    read_gamepad(false, 0x00);
}


void PS2X::read_gamepad(boolean motor1, byte motor2) {
  double temp = millis() - last_read;
  uint8_t old_sreg = SREG;        // *** KJE **** save away the current state of interrupts - *** *** KJE *** ***
  
  if (temp > 1500) //waited to long
    reconfig_gamepad();
    
  if(temp < read_delay)  //waited too short
    delay(read_delay - temp);
    
    
  last_buttons = buttons; //store the previous buttons states

  if(motor2 != 0x00)
    motor2 = map(motor2,0,255,0x40,0xFF); //noting below 40 will make it spin
  
  cli();	//*** KJE ***  
  SET(*_cmd_oreg,_cmd_mask);
  SET(*_clk_oreg,_clk_mask);
  CLR(*_att_oreg,_att_mask); // low enable joystick
  SREG = old_sreg;  // *** KJE *** - Interrupts may be enabled again
  
  delayMicroseconds(CTRL_BYTE_DELAY);
  //Send the command to send button and joystick data;
  char dword[9] = {0x01,0x42,0,motor1,motor2,0,0,0,0};
  byte dword2[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

  for (int i = 0; i<9; i++) {
	  PS2data[i] = _gamepad_shiftinout(dword[i]);
  }
  if(PS2data[1] == 0x79) {  //if controller is in full data return mode, get the rest of data
       for (int i = 0; i<12; i++) {
			PS2data[i+9] = _gamepad_shiftinout(dword2[i]);
       }
  }
    
  cli();
  SET(*_att_oreg,_att_mask); // HI disable joystick
  SREG = old_sreg;  // Interrupts may be enabled again    
	
	#ifdef PS2X_COM_DEBUG
    Serial.println("OUT:IN");
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
	
   buttons = *(uint16_t*)(PS2data+3);   //store as one value for multiple functions
   last_read = millis();
}

byte PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat) {
	return config_gamepad(clk, cmd, att, dat, false, false);
}


byte PS2X::config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat, bool pressures, bool rumble) {

   uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts
   byte temp[sizeof(type_read)];
  
 _clk_mask = maskToBitNum(digitalPinToBitMask(clk));
 _clk_oreg = portOutputRegister(digitalPinToPort(clk));
 _cmd_mask = maskToBitNum(digitalPinToBitMask(cmd));
 _cmd_oreg = portOutputRegister(digitalPinToPort(cmd));
 _att_mask = maskToBitNum(digitalPinToBitMask(att));
 _att_oreg = portOutputRegister(digitalPinToPort(att));
 _dat_mask = maskToBitNum(digitalPinToBitMask(dat));
 _dat_ireg = portInputRegister(digitalPinToPort(dat));
  

  pinMode(clk, OUTPUT); //configure ports
  pinMode(att, OUTPUT);
  pinMode(cmd, OUTPUT);
  pinMode(dat, INPUT);

  digitalWrite(dat, HIGH); //enable pull-up 
    
   cli();                          // *** KJE *** disable for now
   SET(*_cmd_oreg,_cmd_mask); // SET(*_cmd_oreg,_cmd_mask);
   SET(*_clk_oreg,_clk_mask);
   SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
   
   //new error checking. First, read gamepad a few times to see if it's talking
   read_gamepad();
   read_gamepad();
   
   //see if it talked
   if(PS2data[1] != 0x41 && PS2data[1] != 0x73 && PS2data[1] != 0x79){ //see if mode came back. If still anything but 41, 73 or 79, then it's not talking
      #ifdef PS2X_DEBUG
		Serial.println("Controller mode not matched or no controller found");
		Serial.print("Expected 0x41 or 0x73, got ");
		Serial.println(PS2data[1], HEX);
	  #endif
	 
	 return 1; //return error code 1
	}
  
  //try setting mode, increasing delays if need be. 
  read_delay = 1;
  
  for(int y = 0; y <= 10; y++)
  {
   sendCommandString(enter_config, sizeof(enter_config)); //start config run
   
   //read type
   	delayMicroseconds(CTRL_BYTE_DELAY);

    cli();                          // *** KJE *** disable for now
	SET(*_cmd_oreg,_cmd_mask);
    SET(*_clk_oreg,_clk_mask);
    CLR(*_att_oreg,_att_mask); // low enable joystick
    SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	
    delayMicroseconds(CTRL_BYTE_DELAY);

    for (int i = 0; i<9; i++) {
	  temp[i] = _gamepad_shiftinout(type_read[i]);
    }

    cli();                          // *** KJE *** disable for now
	SET(*_att_oreg,_att_mask); // HI disable joystick
    SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	
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
		Serial.print("mode stil set at");
		Serial.println(PS2data[1], HEX);
		#endif
      return 2; //exit function with error
	  }
    
    read_delay += 1; //add 1ms to read_delay
  }
   
 return 0; //no error if here
}



void PS2X::sendCommandString(byte string[], byte len) {
  
   uint8_t old_sreg = SREG;        // *** KJE *** save away the current state of interrupts

  #ifdef PS2X_COM_DEBUG
  byte temp[len];
  cli();                          // *** KJE *** disable for now
  CLR(*_att_oreg,_att_mask); // low enable joystick
  SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
	
  for (int y=0; y < len; y++)
    temp[y] = _gamepad_shiftinout(string[y]);
    
  cli();                          // *** KJE *** disable for now
  SET(*_att_oreg,_att_mask); //high disable joystick  
  SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
   delay(read_delay);                  //wait a few
  
  Serial.println("OUT:IN Configure");
  for(int i=0; i<len; i++){
			Serial.print(string[i], HEX);
			Serial.print(":");
			Serial.print(temp[i], HEX);
			Serial.print(" ");
		}
   Serial.println("");
  
  #else
  cli();                          // *** KJE *** disable for now
  CLR(*_att_oreg,_att_mask); // low enable joystick
  SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
  for (int y=0; y < len; y++)
    _gamepad_shiftinout(string[y]);
    
   cli();                          // *** KJE *** disable for now
   SET(*_att_oreg,_att_mask); //high disable joystick  
   SREG = old_sreg;  // *** *** KJE *** *** Interrupts may be enabled again 
   delay(read_delay);                  //wait a few
   #endif
}

 
uint8_t PS2X::maskToBitNum(uint8_t mask) {
    for (int y = 0; y < 8; y++)
    {
      if(CHK(mask,y))
        return y;
    }
    return 0;
}


byte PS2X::readType() {
/*
	byte temp[sizeof(type_read)];
	
	sendCommandString(enter_config, sizeof(enter_config));
	
	delayMicroseconds(CTRL_BYTE_DELAY);

	SET(*_cmd_oreg,_cmd_mask);
    SET(*_clk_oreg,_clk_mask);
    CLR(*_att_oreg,_att_mask); // low enable joystick
	
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
	
	if(controller_type == 0x03)
		return 1;
	else if(controller_type == 0x01)
		return 2;
	
	return 0;
	
}

void PS2X::enableRumble() {
  
     sendCommandString(enter_config, sizeof(enter_config));
     sendCommandString(enable_rumble, sizeof(enable_rumble));
     sendCommandString(exit_config, sizeof(exit_config));
     en_Rumble = true;
  
}

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

void PS2X::reconfig_gamepad(){
  
   sendCommandString(enter_config, sizeof(enter_config));
   sendCommandString(set_mode, sizeof(set_mode));
   if (en_Rumble)
      sendCommandString(enable_rumble, sizeof(enable_rumble));
   if (en_Pressures)
      sendCommandString(set_bytes_large, sizeof(set_bytes_large));
   sendCommandString(exit_config, sizeof(exit_config));
   
}
