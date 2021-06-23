#include <PS2X_lib.h> //for v1.6
#include <BleGamepad.h> //https://github.com/lemmingDev/ESP32-BLE-Gamepad

BleGamepad bleGamepad;

/******************************************************************
 * set pins connected to PS2 controller:
 * replace pin numbers by the ones you use
 ******************************************************************/
#define PS2_DAT 19
#define PS2_CMD 23
#define PS2_SEL 5
#define PS2_CLK 18

/******************************************************************
 * select modes of PS2 controller:
 *   - pressures = analog reading of push-butttons 
 *   - rumble    = motor rumbling
 * in this example rumble and pressures arent used.
 ******************************************************************/
//#define pressures   true
#define pressures false
//#define rumble      true
#define rumble false

PS2X ps2x; // create PS2 Controller Class

//right now, the PS2X library does NOT support hot pluggable controllers, meaning
//you must always either restart your Arduino after you connect the controller,
//or call config_gamepad(pins) again after connecting the controller.

int error = 0;
byte type = 0;

//Create unique deviceName
std::string getUniqueDeviceName()
{
    
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    char outputString[9];
    itoa(chipId, outputString, 16);
    String devName = "PSX Adapter " + String(outputString);
    return devName.c_str();
}
void setup()
{

    Serial.begin(9600);

    delay(300); //added delay to give wireless ps2 module some time to startup, before configuring it

    //CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************

    //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);

    if (error == 0)
    {
        Serial.print("Found Controller, configured successful ");
        Serial.print("pressures = ");
        if (pressures)
            Serial.println("true ");
        else
            Serial.println("false");
        Serial.print("rumble = ");
        if (rumble)
            Serial.println("true)");
        else
            Serial.println("false");
    }
    else if (error == 1)
        Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");

    else if (error == 2)
        Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

    else if (error == 3)
        Serial.println("Controller refusing to enter Pressures mode, may not support it. ");

    //  Serial.print(ps2x.Analog(1), HEX);

    type = ps2x.readType();
    switch (type)
    {
    case 0:
        Serial.print("Unknown Controller type found");
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

    bleGamepad.deviceName = getUniqueDeviceName().c_str();
    bleGamepad.begin();
}

void convertGamePad(uint32_t psx_btn, uint32_t ble_btn)
{
    if (ps2x.ButtonPressed(psx_btn))
    {
        bleGamepad.press(ble_btn);
    }
    if (ps2x.ButtonReleased(psx_btn))
    {
        bleGamepad.release(ble_btn);
    }
}

void loop()
{
    ps2x.read_gamepad();
    if (bleGamepad.isConnected())
    {
        convertGamePad(PSB_PAD_LEFT, BUTTON_1);
        convertGamePad(PSB_PAD_UP, BUTTON_2);
        convertGamePad(PSB_PAD_RIGHT, BUTTON_3);
        convertGamePad(PSB_PAD_DOWN, BUTTON_4);

        convertGamePad(PSB_SELECT, BUTTON_5);
        convertGamePad(PSB_START, BUTTON_6);

        convertGamePad(PSB_SQUARE, BUTTON_7);
        convertGamePad(PSB_TRIANGLE, BUTTON_8);
        convertGamePad(PSB_CIRCLE, BUTTON_9);
        convertGamePad(PSB_CROSS, BUTTON_10);

        convertGamePad(PSB_L1, BUTTON_11);
        convertGamePad(PSB_L2, BUTTON_12);

        convertGamePad(PSB_R1, BUTTON_13);
        convertGamePad(PSB_R2, BUTTON_14);

        convertGamePad(PSB_L3, BUTTON_15);
        convertGamePad(PSB_R3, BUTTON_16);

        int16_t lx = map(ps2x.Analog(PSS_LX), 0, 255, -32767, 32767);
        int16_t ly = map(ps2x.Analog(PSS_LY), 0, 255, -32767, 32767);

        int16_t rx = map(ps2x.Analog(PSS_RX), 0, 255, -32767, 32767);
        int16_t ry = map( ps2x.Analog(PSS_RY), 0, 255, -32767, 32767);

        /*
        //The BleGamepad Lib does not provide enough axes. We cant send every pressure button sadly. Will maybe change in the future.

        int16_t pres_pad_left = map(ps2x.Analog(PSAB_PAD_LEFT), 0, 255, 0, 32767);
        int16_t pres_pad_up = map(ps2x.Analog(PSAB_PAD_UP), 0, 255, 0, 32767);
        int16_t pres_pad_right = map(ps2x.Analog(PSAB_PAD_RIGHT), 0, 255, 0, 32767);
        int16_t pres_pad_down = map(ps2x.Analog(PSAB_PAD_DOWN), 0, 255, 0, 32767);

        int16_t pres_square = map(ps2x.Analog(PSAB_SQUARE), 0, 255, 0, 32767);
        int16_t pres_triangle = map(ps2x.Analog(PSAB_TRIANGLE), 0, 255, 0, 32767);
        int16_t pres_circle = map(ps2x.Analog(PSAB_CIRCLE), 0, 255, 0, 32767);
        int16_t pres_cross = map(ps2x.Analog(PSAB_CROSS), 0, 255, 0, 32767);

        int16_t pres_l1 = map(ps2x.Analog(PSAB_L1), 0, 255, 0, 32767);
        int16_t pres_l2 = map(ps2x.Analog(PSAB_L2), 0, 255, 0, 32767);

        int16_t pres_r1 = map(ps2x.Analog(PSAB_R1), 0, 255, 0, 32767);
        int16_t pres_r2 = map(ps2x.Analog(PSAB_R2), 0, 255, 0, 32767);
        */

        //Set analog axes

        bleGamepad.setAxes(lx, ly, rx, ry, 32767, 32767, 32767, 32767, DPAD_CENTERED);
    }
    delay(50);
}