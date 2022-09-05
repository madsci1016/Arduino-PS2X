#include <PS2X_lib.h>  //for v1.6
#include <SPI.h> // for hardware SPI support

/******************************************************************
 * set pins connected to PS2 controller:
 *   - 1e column: original 
 *   - 2e colmun: Stef?
 *   - 3e column: itsmevjnk (for ESP32 HSPI)
 * replace pin numbers by the ones you use
 ******************************************************************/
#define PS2_DAT        13  //14  12
#define PS2_CMD        11  //15  13
#define PS2_SEL        10  //16  15
#define PS2_CLK        12  //17  14

/******************************************************************
 * select modes of PS2 controller:
 *   - pressures = analog reading of push-butttons 
 *   - rumble    = motor rumbling
 * uncomment 1 of the lines for each mode selection
 ******************************************************************/
//#define pressures   true
#define pressures   false
//#define rumble      true
#define rumble      false

PS2X ps2x; // create PS2 Controller Class

//right now, the library does NOT support hot pluggable controllers, meaning 
//you must always either restart your Arduino after you connect the controller, 
//or call config_gamepad(pins) again after connecting the controller.

int error = 0;
byte type = 0;
byte vibrate = 0;

void setup(){
 
  Serial.begin(57600);

  /* support more flexible waiting times for wireless PS2 module to start up */
  unsigned long t_start = millis();
  Serial.println("Initializing PS2 controller.");
  while(1) {
    /* 
     * There are multiple ways to initialize the gamepad, which can be categorized into three levels:
     * Level 1: Beginner
     *  The gamepad can be initialized using these ready-to-use functions, which make use of the platform's hardware SPI bus:
     *   error = ps2x.config_gamepad_arduino_spi(PS2_SEL); // please note that unless specified with the pressures and rumble arguments, these features will not be used
     *   error = ps2x.config_gamepad_arduino_spi(PS2_SEL, pressures, rumble);
     *  A few other functions are available exclusively for the ESP32 to make use of its HSPI and VSPI buses (the examples shown below are for HSPI, to use VSPI just change hspi to vspi):
     *   error = ps2x.config_gamepad_esp32_hspi(PS2_SEL);
     *   error = ps2x.config_gamepad_esp32_hspi(PS2_SEL, pressures, rumble);
     *   error = ps2x.config_gamepad_esp32_hspi(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT); // this and the below function use ESP32's SPI pin routing features so that connection is not just limited to the designated pins
     *   error = ps2x.config_gamepad_esp32_hspi(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
     *  If hardware SPI is not available, this library also provides a software (bitbanged) SPI solution. To use it, use one of these functions:
     *   error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT);
     *   error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble); // used below
     * Level 2: Intermediate
     *  Alternatively, if you choose to use a custom SPI class (e.g. software SPI library), or you want to tinker with pointers, these functions are available:
     *   error = ps2x.config_gamepad(&SPI, PS2_SEL);
     *   error = ps2x.config_gamepad(&SPI, PS2_SEL, pressures, rumble);
     *  If you're using something other than the default SPI bus, make sure that you replace &SPI with a pointer (SPIClass*) pointing to the SPI class of your choice.
     *  Please note that the above functions automatically initialize the SPI class using its begin() function. If you are looking for functions that don't do so, check out Level 3.
     *  NOTE: On ESP32, the default SPI class refers to the VSPI bus. If you're using HSPI, you have to change &SPI to `new SPIClass(HSPI)` (without the backticks).
     * Level 3: Advanced
     *  Some SPI class implementations (e.g. ESP32) allows for custom configuration in their begin() function (e.g. pin routing). If you wish to be in control of this configuration, use one of these instead:
     *   error = ps2x.config_gamepad(&SPI, PS2_SEL, false);
     *   error = ps2x.config_gamepad(&SPI, PS2_SEL, pressures, rumble, false);
     *  Note the `false` argument at the end.
     *  You MUST run the SPI class' begin() function (e.g. SPI.begin() in the above example) prior to running the functions above, otherwise initialization will fail.
     */
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble); // software SPI initialization
    
    if(error == 0){
      Serial.print("Found Controller, configured successful after ");
      Serial.print(millis() - t_start, DEC);
      Serial.print("ms (pressures = ");
    if (pressures)
      Serial.print("true");
    else
      Serial.print("false");
    Serial.print(", rumble = ");
    if (rumble)
      Serial.println("true)");
    else
      Serial.println("false");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Note: Go to www.billporter.info for updates and to report bugs.");
    break;
    }  
    else if(error == 1) {
      Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
      continue;
    }
    
    else if(error == 2) {
      Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
      continue;
    }

    else if(error == 3) {
      Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
      break; // non-fatal error
    }
  }

  type = ps2x.readType(); 
  switch(type) {
    case 0:
      Serial.print("Unknown Controller type found ");
      break;
    case 1:
      Serial.print("DualShock Controller found ");
      break;
    case 2:
      Serial.print("GuitarHero Controller found ");
      break;
	  case 3:
      Serial.print("Wireless Sony DualShock Controller found ");
      break;
  }
}

void loop() {
  /* You must Read Gamepad to get new values and set vibration values
     ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
     if you don't enable the rumble, use ps2x.read_gamepad(); with no values
     You should call this at least once a second
   */  
  if(error == 1) //skip loop if no controller found
    return; 
  
  if(type == 2){ //Guitar Hero Controller
    ps2x.read_gamepad();          //read controller 
   
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
    
    if(ps2x.Button(UP_STRUM))          //will be TRUE as long as button is pressed
      Serial.println("Up Strum");
    if(ps2x.Button(DOWN_STRUM))
      Serial.println("DOWN Strum");
 
    if(ps2x.Button(PSB_START))         //will be TRUE as long as button is pressed
      Serial.println("Start is being held");
    if(ps2x.Button(PSB_SELECT))
      Serial.println("Select is being held");
    
    if(ps2x.Button(ORANGE_FRET)) {     // print stick value IF TRUE
      Serial.print("Wammy Bar Position:");
      Serial.println(ps2x.Analog(WHAMMY_BAR), DEC); 
    } 
  }
  else { //DualShock Controller
    ps2x.read_gamepad(false, vibrate); //read controller and set large motor to spin at 'vibrate' speed
    
    if(ps2x.Button(PSB_START))         //will be TRUE as long as button is pressed
      Serial.println("Start is being held");
    if(ps2x.Button(PSB_SELECT))
      Serial.println("Select is being held");      

    if(ps2x.Button(PSB_PAD_UP)) {      //will be TRUE as long as button is pressed
      Serial.print("Up held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_UP), DEC);
    }
    if(ps2x.Button(PSB_PAD_RIGHT)){
      Serial.print("Right held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_RIGHT), DEC);
    }
    if(ps2x.Button(PSB_PAD_LEFT)){
      Serial.print("LEFT held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_LEFT), DEC);
    }
    if(ps2x.Button(PSB_PAD_DOWN)){
      Serial.print("DOWN held this hard: ");
      Serial.println(ps2x.Analog(PSAB_PAD_DOWN), DEC);
    }   

    vibrate = ps2x.Analog(PSAB_CROSS);  //this will set the large motor vibrate speed based on how hard you press the blue (X) button
    if (ps2x.NewButtonState()) {        //will be TRUE if any button changes state (on to off, or off to on)
      if(ps2x.Button(PSB_L3))
        Serial.println("L3 pressed");
      if(ps2x.Button(PSB_R3))
        Serial.println("R3 pressed");
      if(ps2x.Button(PSB_L2))
        Serial.println("L2 pressed");
      if(ps2x.Button(PSB_R2))
        Serial.println("R2 pressed");
      if(ps2x.Button(PSB_TRIANGLE))
        Serial.println("Triangle pressed");        
    }

    if(ps2x.ButtonPressed(PSB_CIRCLE))               //will be TRUE if button was JUST pressed
      Serial.println("Circle just pressed");
    if(ps2x.NewButtonState(PSB_CROSS))               //will be TRUE if button was JUST pressed OR released
      Serial.println("X just changed");
    if(ps2x.ButtonReleased(PSB_SQUARE))              //will be TRUE if button was JUST released
      Serial.println("Square just released");     

    if(ps2x.Button(PSB_L1) || ps2x.Button(PSB_R1)) { //print stick values if either is TRUE
      Serial.print("Stick Values:");
      Serial.print(ps2x.Analog(PSS_LY), DEC); //Left stick, Y axis. Other options: LX, RY, RX  
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

