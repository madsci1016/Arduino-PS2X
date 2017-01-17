#include <PS2X_lib.h>  //for v1.6

/*
  right now, the library does NOT support hot pluggable controllers, meaning 
  you must always either restart your Arduino after you conect the controller, 
  or call config_gamepad(pins) again after connecting the controller.
*/

// create PS2 Controller Class
PS2X ps2x;
int error = 0; 
byte type = 0;
byte vibrate = 0;

void setup()
{
  Serial.begin(57600);
  // CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************
  // setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(22,23,24,25, true, true);

  if(error == 0)
  {
    Serial.println("Found Controller, configured successful");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Go to www.billporter.info for updates and to report bugs.");
  }
  else if(error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
  else if(error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
  else if(error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  
  // Serial.print(ps2x.Analog(1), HEX);
  type = ps2x.readType();
  switch(type)
  {
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
}

void loop()
{
  /*
    You must Read Gamepad to get new values
    Read GamePad and set vibration values
    ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
    if you don't enable the rumble, use ps2x.read_gamepad(); with no values
    you should call this at least once a second
  */
  // skip loop if no controller found
  if(error == 1)
    return; 
  
  if(type == 2)
  {
    // Guitar Hero Controller
    // read controller
    ps2x.read_gamepad();
    
    if(ps2x.ButtonPressed(GREEN_FRET))
      Serial.println("Green Fret Pressed");
    if(ps2x.ButtonPressed(RED_FRET))
      Serial.println("Red Fret Pressed");
    if(ps2x.ButtonPressed(YELLOW_FRET))
      Serial.println("Yellow Fret Pressed");
    if(ps2x.ButtonPressed(BLUE_FRET))
      Serial.println("Blue Fret Pressed");
    if(ps2x.ButtonPressed(ORANGE_FRET))
      Serial.println("Orange Fret Pressed");
    
    if(ps2x.ButtonPressed(STAR_POWER))
      Serial.println("Star Power Command");
    
    // will be TRUE as long as button is pressed
    if(ps2x.Button(UP_STRUM))
      Serial.println("Up Strum");
    if(ps2x.Button(DOWN_STRUM))
      Serial.println("DOWN Strum");
    
    // will be TRUE as long as button is pressed
    if(ps2x.Button(PSB_START))
      Serial.println("Start is being held");
    if(ps2x.Button(PSB_SELECT))
      Serial.println("Select is being held");
    
    // print stick value IF TRUE
    if(ps2x.Button(ORANGE_FRET))
    {
      Serial.print("Wammy Bar Position:");
      Serial.println(ps2x.Analog(WHAMMY_BAR), DEC); 
    } 
  }
  else
  {
    // DualShock Controller
    // read controller and set large motor to spin at 'vibrate' speed
    ps2x.read_gamepad(false, vibrate);
    
    // will be TRUE as long as button is pressed
    if(ps2x.Button(PSB_START))
      Serial.println("Start is being held");
    if(ps2x.Button(PSB_SELECT))
      Serial.println("Select is being held");
    
    // will be TRUE as long as button is pressed
    if(ps2x.Button(PSB_PAD_UP))
    {
      Serial.print("Up held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_UP), DEC);
    }
    if(ps2x.Button(PSB_PAD_RIGHT))
    {
      Serial.print("Right held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_RIGHT), DEC);
    }
    if(ps2x.Button(PSB_PAD_LEFT))
    {
      Serial.print("LEFT held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_LEFT), DEC);
    }
    if(ps2x.Button(PSB_PAD_DOWN))
    {
      Serial.print("DOWN held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_DOWN), DEC);
    }
    
    // this will set the large motor vibrate speed based on how hard you press the blue (X) button
    vibrate = ps2x.Analog(PSAB_BLUE);
    
    // will be TRUE if any button changes state (on to off, or off to on)
    if (ps2x.NewButtonState())
    {
      if(ps2x.Button(PSB_L3))
        Serial.println("L3 pressed");
      if(ps2x.Button(PSB_R3))
        Serial.println("R3 pressed");
      if(ps2x.Button(PSB_L2))
        Serial.println("L2 pressed");
      if(ps2x.Button(PSB_R2))
        Serial.println("R2 pressed");
      if(ps2x.Button(PSB_GREEN))
        Serial.println("Triangle pressed");
    }
    
    // will be TRUE if button was JUST pressed
    if(ps2x.ButtonPressed(PSB_RED))
    Serial.println("Circle just pressed");
    
    // will be TRUE if button was JUST released
    if(ps2x.ButtonReleased(PSB_PINK))
      Serial.println("Square just released");     
    
    // will be TRUE if button was JUST pressed OR released
    if(ps2x.NewButtonState(PSB_BLUE))
      Serial.println("X just changed");    
    
    // print stick values if either is TRUE
    if(ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1))
    {
      Serial.print("Stick Values:");
      // Left stick, Y axis. Other options: LX, RY, RX  
      Serial.print(ps2x.Analog(PSS_LY), DEC);
      Serial.print(",");
      Serial.print(ps2x.Analog(PSS_LX), DEC); 
      Serial.print(",");
      Serial.print(ps2x.Analog(PSS_RY), DEC); 
      Serial.print(",");
      Serial.println(ps2x.Analog(PSS_RX), DEC); 
    }
  }
  delay(50);
}
