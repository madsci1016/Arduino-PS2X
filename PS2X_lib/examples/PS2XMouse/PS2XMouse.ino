#include <PS2X_lib.h>

PS2X ps2x; // create PS2 Controller Class

int error = 0; 
byte type = 0;
const int ledPin = 11;         // Mouse control LED (11 on Teensy 2.0, 13 on Arduino Leonardo)

// parameters for reading the joystick:
int range = 12;               // output range of X or Y movement
int responseDelay = 5;        // response delay of the mouse, in ms
int threshold = range/4;      // resting threshold
int center = range/2;         // resting position value

boolean mouseIsActive = false;    // whether or not to control the mouse
int lastSwitchState = LOW;        // previous switch state

void setup(){
 Serial.begin(57600);
  
 error = ps2x.config_gamepad(15,14,13,12, true, true);   //setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
 
 if(error == 0){
   Serial.println("Found Controller, configured successful");
 }
   
  else if(error == 1)
   Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
   
  else if(error == 2)
   Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
   
  else if(error == 3)
   Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
   
   type = ps2x.readType(); 
     switch(type) {
       case 0:
        Serial.println("Unknown Controller type");
       break;
       case 1:
        Serial.println("DualShock Controller Found");
       break;
       case 2:
         Serial.println("GuitarHero Controller Found");
       break;
     }
  
 // take control of the mouse:
  Mouse.begin();
  Keyboard.begin();
}

void loop()
{
   
 if(error == 1) //skip loop if no controller found
  return; 
  
  ps2x.read_gamepad(false, 0);          //read controller and set large motor to spin at 'vibrate' speed
    
  // read the switch:
  int switchState = ps2x.ButtonPressed(PSB_RED);
  // if it's changed and it's high, toggle the mouse state:
  if (switchState != lastSwitchState) {
    if (switchState == HIGH) {
      mouseIsActive = !mouseIsActive;
      // turn on LED to indicate mouse state:
      digitalWrite(ledPin, mouseIsActive);
    } 
  }
  // save switch state for next comparison:
  lastSwitchState = switchState;

  // read and scale the two axes:
  int xReading = readAxis(PSS_LX);
  int yReading = readAxis(PSS_LY);

  // if the mouse control state is active, move the mouse:
  if (mouseIsActive) {
    Mouse.move(xReading, yReading, 0);
  }  

  // read the mouse button and click or not click:
  // if the mouse button is pressed:
  if (ps2x.ButtonPressed(PSB_BLUE)) {
    // if the mouse is not pressed, press it:
    if (!Mouse.isPressed(MOUSE_LEFT)) {
      Mouse.press(MOUSE_LEFT); 
    }
  } 
  // else the mouse button is not pressed:
  else {
    // if the mouse is pressed, release it:
    if (Mouse.isPressed(MOUSE_LEFT)) {
      Mouse.release(MOUSE_LEFT); 
    }
  }
  
  if (ps2x.Button(PSB_PAD_UP)) {
    Keyboard.press(KEY_UP_ARROW); 
  } else {
    Keyboard.release(KEY_UP_ARROW); 
  }
  
  if (ps2x.Button(PSB_PAD_DOWN)) {
    Keyboard.press(KEY_DOWN_ARROW); 
  } else {
    Keyboard.release(KEY_DOWN_ARROW); 
  }
  
  if (ps2x.Button(PSB_PAD_RIGHT)) {
    Keyboard.press(KEY_RIGHT_ARROW); 
  } else {
    Keyboard.release(KEY_RIGHT_ARROW); 
  }
  
  if (ps2x.Button(PSB_PAD_LEFT)) {
    Keyboard.press(KEY_LEFT_ARROW); 
  } else {
    Keyboard.release(KEY_LEFT_ARROW); 
  }
 
 delay(5);
     
}

/*
  reads an axis (0 or 1 for x or y) and scales the 
 analog input range to a range from 0 to <range>
 */

int readAxis(int thisAxis) { 
  // read the analog input:
  int reading = ps2x.Analog(thisAxis);

  // map the reading from the analog input range to the output range:
  reading = map(reading, 0, 255, 0, range);

  // if the output reading is outside from the
  // rest position threshold,  use it:
  int distance = reading - center;

  if (abs(distance) < threshold) {
    distance = 0;
  } 

  // return the distance for this axis:
  return distance;
}
