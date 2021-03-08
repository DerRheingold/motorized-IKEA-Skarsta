/*
  ----------------------------
   Arduino Power Desktop PLUS
  ----------------------------
  SUMMARY 
    Controls the direction and speed of the motor, supports auto-raising and auto-lowering by pre-recording their elapsed times
  
  DESCRIPTION 
    This code waits for a button to be pressed (UP or DOWN), and accordingly powers the motors in the desired direction and speed.
    One motor will turn clockwise, the other counterclockwise. The motors are intended to be placed facing each other, to duplicate the torque on the allen wrench
    On my particular setup (a heavy desktop with 2 monitors) I use 100% of the power speed and torque when raising the desk,
    however, when lowering the desk I need a bit less since gravity helps, to keep the speed up and down at a similar rate, this is configurable.
    You may want to tweak the variables PWM_SPEED_UP and PWM_SPEED_DOWN to adjust it to your desktop load, the allowed values
    are 0 (min) to 255 (max).

  BASIC USAGE
    - Press and hold BUTTON_UP to raise the desk. a small delay of 500ms has been introduced for smoothness
    - Press and hold BUTTON_DOWN to lower the desk. a small delay of 500ms has been introduced for smoothness
  
  PROGRAM MODE (Recording time to raise/lower) - [optional]
    Use program mode to record the time it takes your motors to raise and lower the desk. once recorded, you can use the auto-raise and auto-lower functions,
    this is an optional functionality, it's ok if you decide simply not to use it
    - To enter Program Mode, press and hold BUTTON_UP and BUTTON_DOWN for 3 seconds
    - The LED on pin 9 will continuously blink rapidly, indicating that the system is in program mode waiting for the user to start recording (raise or lower)
    - While in program mode, Press and Hold BUTTON_UP to raise the desk and RECORD the time it takes to raise it, this value will be used on auto-raise later
    - While in program mode, Press and Hold BUTTON_DOWN to lower the desk and RECORD the time it takes to lower it, this value will be used on auto-lower later
    - After the button is released, the LED will perform a "BLINK_THINKING" followed by a "BLINK_SUCCESS" (see below) to indicate the program was recorded to EEPROM correctly,
      otherwise, a BLINK_ERROR will be performed

  AUTO RAISING THE DESK [optional]
    - IMPORTANT: This is a very cool but kind of risky feature, it's completely optional, don't use it if you don't fully understand the risks. Here is why: If you 
      activate auto-raise, and your desk was already at the maximum height, then - depending on your desk - on the IKEA SKARSTA it will hit a stopping point and the MOTORS WILL
      STALL for the amount of seconds that you recorded. In other words, if you recorded 30 seconds to raise, and your desk is already at the top position (or close), and 
      you still enable auto-raise, you risk damaging your motors as a full power will be sent to them but they will be blocked. When using auto-raise and auto-lower you must
      ALWAYS be present and watching the desk, ready to cancel the operation if the motors stall for any reason. To cancel, simply press any button, or turn the circuit off.
      I have built-in a somewhat-safety option feature (see below), but there is still a small risk. Needless to say, use at your own risk, I do not assume any responsibility if
      your motors or anything else breaks because of misuse or any other reason.
    - In order to automatically raise the desk, you must first record the time it takes to raise it (see PROGRAM MODE above), if you try to input the auto-raise sequence
      without the time to raise programmed it will simply stop raising and perform a BLINK_ERROR routine. Completely unharmful, you can continue to use the manual up/down buttons
    - To activate auto-raise (using BUTTON_UP only)
      - Press the button Twice in less than a second then immediately Press & Hold for 2 seconds (i.e. Press, Press, Press+Hold)
      - If the sequence was done correctly, the LED will blink continuously (AFTER the 2 seconds of holding) and you can then release the button, 
        it will continue raising the desk automatically and then stop when the recorded time has elapsed. The 2 seconds that it takes to activate program mode are substracted from
        the recorded time to keep the total raise time correct (approximately, of course).
      - Please note, the moment you hold the button (after the 2 presses) the desk will begin to raise, you must keep holding it while it raises for 2 seconds for the program
        to kick in. This was done on purpose as a safety measure so that if your desk was already at maximum height, you will immediately notice that the motor stalled and you 
        should release the button immediately. 
      - To cancel auto-raise at any time, simply press any button. But be CAREFUL if you want to activate it again, as the program simply re-plays the recorded values, and will not 
        know that you are already at a higher position. see IMPORTANT note above for details.

  AUTO LOWERING THE DESK [optional]
    - Follow the same exact steps as with AUTO RAISING, but using BUTTON_DOWN

  You may use this code and all of the diagrams and documentations completely free. Enjoy!
  
  Author: Cesar Moya
  Date:   July 28th, 2020
  URL:    https://github.com/cesar-moya/arduino-power-desktop
*/
#include <EEPROM.h>
#include <Arduino.h>
#include <TM1637Display.h>
#include <Ultrasonic.h>


/* TO DO
- Sometimes loses value for Position 1 in EEPROM :(
- Catch loss of sonar signal during automatic table movement
*/

#define BUTTON_UP 2   //ATM-5
#define BUTTON_DOWN 3 //ATM-4
#define BUTTON_POS_0 4
#define BUTTON_POS_1 5
#define enA 6         //ATM-12
#define in1 7         //ATM-13
#define in2 8         //ATM-14
#define enB 10        //ATM-16
#define in3 11        //ATM-17
#define in4 12        //ATM-18
#define CLK 14 // 7 Segment
#define DIO 15 // 7 Segment
#define ECHO_PIN     16  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN  17  // Arduino pin tied to trigger pin on the ultrasonic sensor.

//NewPing doesn't work reliably with an external power supply, so switched to "Ultrasonic" library.
Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
TM1637Display display(CLK, DIO);

// Definitions for Platformio
void readFromEEPROM();
void validateEEPROM();
void checkButtonClicksExpired();
void handleButtonUp();
void handleButtonDown();
void position_0();
void position_1();
void checkHeight();
void goDown();
void goUp();
void autoRaiseDesk(long alreadyElapsed);
void stopMoving();
void autoLowerDesk(long alreadyElapsed);
void saveToEEPROM_TimeUp(long timeUp);
void saveToEEPROM_TimeDown(long timeDown);
bool handleProgramMode();

//Struct to store the various necessary variables to persist the autoRaise/autoLower programs to EEPROM
struct StoredProgram
{
  bool isUpSet = false; //whether the UP program has been recorded
  bool isDownSet = false; // whether the DOWN program has been recorded
  float timeUp = 0; //time in milliseconds recorded to RAISE the desk
  float timeDown = 0; // time in milliseconds recorded to LOWER the desk
};

StoredProgram savedProgram;
int EEPROM_ADDRESS = 0;
bool BUTTON_UP_STATE = LOW;
bool BUTTON_DOWN_STATE = LOW;
bool BUTTON_POS_0_STATE = LOW;
bool BUTTON_POS_1_STATE = LOW;
long BUTTON_WAIT_TIME = 250; //the small delay before starting to go up/down for smoothness on any button

//Program Mode SET/ACTIVATE variables
//The following three properties can be understood as follows: a user needs to do [PRG_ACTIVATE_PRECLICKS] clicks in less than [PRG_ACTIVATE_PRECLICK_THRESHOLD] ms, and then
//hold the list click for [PRG_ACTIVATE_HOLD_THRESHOLD] ms, to activate auto-raise / auto-lower mode
int  PRG_ACTIVATE_PRECLICKS = 3;//number of clicks before checking for hold to activate auto-raise/lower
long PRG_ACTIVATE_PRECLICK_THRESHOLD = 1000; //the threshold for the PRG_ACTIVATE_PRECLICKS before PRG_ACTIVATE_HOLD_THRESHOLD check kicks in
long PRG_ACTIVATE_HOLD_THRESHOLD = 2000;  //threshold that needs to elapse holding a button (up OR down) after preclicks has been satisfied to enter auto-raise / auto-lower

//the time to enter program mode, to record a new timeUp/timeDown value
long PRG_SET_THRESHOLD = 2000;
//the time when the user began pressing and holding both buttons, attempting to enter program mode
long holdButtonsStartTime = 0; 

//Button UP variables
int btnUpClicks = 0;
long btnUpFirstClickTime = 0;
bool autoRaiseActivated = false;

//Button DOWN variables
int btnDownClicks = 0;
long btnDownFirstClickTime = 0;
bool autoLowerActivated = false;

//Using custom values to ensure no more than 24v are delivered to the motors given my desk load.
//feel free to play with these numbers but make sure to stay within your motor's rated voltage.
const int PWM_SPEED_UP = 255; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 220; //0 - 255, controls motor speed when going DOWN

// Required for additional buttons
long pressedTime;
long releasedTime;
long TimePressed;
const int LONG_PRESS_TIME  = 2000; // The time button "0" or "1" need to be pressed to register as a "long" press and with that save the position to eeprom.


// Required for the ultrasonic sensor
int oldDistance;
int current_height = ultrasonic.read(); // get initial reading upon start
int pos0_height = EEPROM.read(0);
int pos1_height = EEPROM.read(1);

// Some digits/figures for the display
const uint8_t P[] = {
  SEG_A | SEG_B | SEG_E | SEG_F | SEG_G, // P
};
const uint8_t Zero[] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // 0
};
const uint8_t One[] = {
  SEG_B | SEG_C, // 1
};
const uint8_t Two[] = {
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D, // 2
};
const uint8_t Err[] = {
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, // E
  SEG_E | SEG_G, // r
  SEG_E | SEG_G, // r
};
uint8_t Minus[] = {
  SEG_G  // -
};
uint8_t smallO [] = {
  SEG_C | SEG_D | SEG_E | SEG_G // o
};
uint8_t circle [] = {
  SEG_A | SEG_B | SEG_F | SEG_G  // ° 
};
uint8_t empty[] = {0x0}; //blank segement for 7-Segment display
        

//This function debounces the initial button reads to prevent flickering
bool debounceRead(int buttonPin, bool state)
{
  bool stateNow = digitalRead(buttonPin);
  if (state != stateNow)
  {
    delay(10);
    stateNow = digitalRead(buttonPin);
  }
  return stateNow;
}



void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_POS_0, INPUT);
  pinMode(BUTTON_POS_1, INPUT);
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  readFromEEPROM();
  validateEEPROM();
  current_height = ultrasonic.read();
  int digitPosition = 0;
  display.setBrightness(1);
  display.clear();
  //Some start-up-animation on Display
  while (digitPosition <  4 ){  
    delay (100);
    display.setSegments (smallO,1,digitPosition);
    digitPosition++;
    if (digitPosition >= 4){
      digitPosition=0;
      break;
    }
  };
  while (digitPosition < 4){
    delay (100);
    display.setSegments (circle,1,digitPosition);
    digitPosition++;
    if (digitPosition >= 4){
      digitPosition=0;
      break;
    }
  };
  while (digitPosition < 4){
    delay (100);
    display.setSegments (Zero,1,digitPosition);
    digitPosition++;
    if (digitPosition >= 4){
      digitPosition=0;
      break;
    }
  };
  while (digitPosition < 4){
    delay (100);
    display.setSegments (empty,1,digitPosition);
    digitPosition++;
    if (digitPosition >= 4){
      digitPosition=0;
      break;
    }
  };
  display.clear();
  delay (400);
  // Display the saved Position 0 height on the display upon startup
  display.setSegments (P,1,0);  
  display.setSegments (empty,1,1);
  display.setSegments (Zero,1,2);
  display.setSegments (empty,1,3);
  delay (1000);
  display.showNumberDec(pos0_height, false);
  delay (1500);
  display.clear();
  delay (400);
  // Display the saved Position 1 height on the display upon startup
  display.setSegments (P,1,0); 
  display.setSegments (empty,1,1);
  display.setSegments (One,1,2);
  display.setSegments (empty,1,3);
  delay (1000);
  display.showNumberDec(pos1_height, false);
  delay (1500);
  display.clear();
  delay (400);
  // Display the current height on the display upon startup
  checkHeight(); 
  delay (1500);  
  display.clear();
}

void loop() {
  //If both buttons are pressed and held, check for entering program mode, 
  //if this func returns true we want to skip anything else in the main loop
  if (handleProgramMode()){  
    return;
  }
  //Check if the user didn't click at least twice(default config) in less than a second, reset counters if necessary
  checkButtonClicksExpired();

  //Handle press and hold of buttons to raise/lower, and check if enter auto-raise and auto-lower
  handleButtonUp();
  handleButtonDown();

  //Handle press and hold of buttons to trigger a saved position or save the current position
  position_0();
  position_1();
}

/***********************************************
  Pressing the Button for Position 0 (sitting)
  Upon a short press, the table will drive into the lower (sitting) position
  When long-pressed there is a small animation in the display and afterwards (upon release of the button) the current height is saved to eeprom and shown in the display
***********************************************/
void position_0 (){
   bool btnUpState = digitalRead(BUTTON_UP);
   bool btnDownState = digitalRead(BUTTON_DOWN);
   bool btnPos0State = digitalRead(BUTTON_POS_0);
   current_height = ultrasonic.read();
   if (!BUTTON_POS_0_STATE && debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){  //define what to do when the button is pressed 
       BUTTON_POS_0_STATE=HIGH;
       Serial.println("POS 0 pressed");
       pressedTime = millis(); 
       int digitPosition = 0;
       while (btnPos0State && debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){  //small animation on Display while button is held down longer than 500 ms
         delay (500);
         display.setSegments (smallO,1,digitPosition);
         digitPosition++;
         if (digitPosition >= 4){
          delay (500);
          display.setSegments (Zero,1,0);
          display.setSegments (Zero,1,1);
          display.setSegments (Zero,1,2);
          display.setSegments (Zero,1,3);
          break;
         }
       }
   }
   else if (BUTTON_POS_0_STATE && !debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){ //releasing the button checks how long it was pressed and then decides what to do
       BUTTON_POS_0_STATE=LOW;
       releasedTime = millis();   
       TimePressed = releasedTime-pressedTime;

       //If "Position 0 button" is long-pressed, save current height to Position 0 and display "P 0" and the height in cm in the display
       if (TimePressed >= LONG_PRESS_TIME){  
        int pos0SaveHeight = ultrasonic.read();
        if (pos0SaveHeight >= pos1_height) {  //Check if Position 0 is lower than Position 1. If not, display "Err0"
          Serial.print ("must be lower than "); 
          Serial.println (pos1_height);
          display.setSegments (Err,3,0);
          display.setSegments (Zero,1,3);
          delay (1000);
          display.clear();
        }
        else { // Save height and give output to user
          EEPROM.put(0, pos0SaveHeight);
          Serial.print("Saved Position 0: ");
          Serial.println(pos0SaveHeight);
          display.setSegments (P,1,0);
          display.setSegments (empty,1,1);
          display.setSegments (Zero,1,2);
          display.setSegments (empty,1,3);
          delay (1000);
          display.showNumberDec(pos0SaveHeight, false);
          delay (1000);
          display.clear();
        };          
       }
         
       if (TimePressed < LONG_PRESS_TIME){  //If "Position 0 button" is short-pressed, check height and if possible drive to desired height
        
        display.setSegments (P,1,0);
        display.setSegments (empty,1,1);
        display.setSegments (Zero,1,2);
        display.setSegments (empty,1,3);
        
        delay (100);
        int desired_height = pos0_height;
        //if (current_height == 0){ //Catch Sonar-Error before starting program
          //Serial.println("Down Er 1");
        //};
        while (current_height > desired_height){
          checkHeight();
          Serial.print ("current: "); Serial.println(current_height);
          Serial.print ("desired: "); Serial.println(desired_height);
          goDown();
          if (current_height <= desired_height){ //stop automatically if the desired height is reached
            stopMoving();
            Serial.println("Desk too low");
            display.setSegments (P,1,0);
            display.setSegments (empty,1,1);
            display.setSegments (Zero,1,2);
            display.setSegments (empty,1,3);
            delay (1000);
            checkHeight();
            break;
          }
          else if (current_height == 0){  //Catch Sonar-Error while table is moving
            Serial.println("Sonar Error in automated program");
            checkHeight();
            break;
          }
          if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){    //Cancel if up- or down-button is pressed during automatic procedure
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON UP");
            int digitPosition = 0;
            while (digitPosition <  4 ){  //small cancel-animation on Display
              delay (50);
              display.setSegments (Minus,1,digitPosition);
              digitPosition++;
              if (digitPosition >= 4){
                break;
              }
            };
            break;
          }
          else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
            btnUpState = LOW;
          }
      
          if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON DOWN");
            int digitPosition = 0;
            while (digitPosition <  4 ){  //small cancel-animation on Display
              delay (50);
              display.setSegments (Minus,1,digitPosition);
              digitPosition++;
              if (digitPosition >= 4){
                break;
              }
            };
            break;
          }
          else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
            btnDownState = LOW;
          }
        };
        delay (500);
        checkHeight();
        delay (1500);
        display.clear();
       };
   };
};


/**********************************************
  Pressing the Button for Position 1 (standing)
  Upon a short press, the table will drive into the higher (standing) position
  When long-pressed there is a small animation in the display and afterwards (upon release of the button) the current height is saved to eeprom and shown in the display
***********************************************/
void position_1 (){
   bool btnUpState = digitalRead(BUTTON_UP);
   bool btnDownState = digitalRead(BUTTON_DOWN);
   bool btnPos1State = digitalRead(BUTTON_POS_1);
   current_height = ultrasonic.read();
    if (!BUTTON_POS_1_STATE && debounceRead(BUTTON_POS_1, BUTTON_POS_1_STATE)){ //define what to do when the button is pressed 
       BUTTON_POS_1_STATE=HIGH;
       pressedTime = millis(); 
       int digitPosition = 0;
       while (btnPos1State && debounceRead(BUTTON_POS_1, BUTTON_POS_1_STATE)){  //small animation on Display while button is pressed
         delay (500);
         display.setSegments (smallO,1,digitPosition);
         digitPosition++;
         if (digitPosition >= 4){
          delay (500);
          display.setSegments (Zero,1,0);
          display.setSegments (Zero,1,1);
          display.setSegments (Zero,1,2);
          display.setSegments (Zero,1,3);
          break;
         }
       }
   }
   else if (BUTTON_POS_1_STATE && !debounceRead(BUTTON_POS_1, BUTTON_POS_1_STATE)){ //releasing the button checks how long it was pressed and then decides what to do
       BUTTON_POS_1_STATE=LOW;
       releasedTime = millis();   
       TimePressed = releasedTime-pressedTime;
       //If "Position 1 button" is long-pressed, save current height to Position 1 and display "P 1" and the height in cm in the display
       if (TimePressed >= LONG_PRESS_TIME){ 
        int pos1SaveHeight = ultrasonic.read();
        if (pos1SaveHeight <= pos0_height) {  //Check if Position 1 is higher than Position 0. If not, display "Err1"
          Serial.print ("must be higher than "); 
          Serial.println (pos0_height);
          display.setSegments (Err,3,0);
          display.setSegments (One,1,3);
          delay (1000);
          display.clear();
        }
        else { // Save height and give output to user
          EEPROM.put(1, pos1SaveHeight);
          Serial.print("Saved Position 1: ");
          Serial.println(pos1SaveHeight);
          display.setSegments (P,1,0);
          display.setSegments (empty,1,1);
          display.setSegments (One,1,2);
          display.setSegments (empty,1,3);
          delay (1000);
          display.showNumberDec(pos1SaveHeight, false); 
          delay (1000);
          display.clear();
        }        
       }
       
       if (TimePressed < LONG_PRESS_TIME){ //If "Position 1 button" is short-pressed, check height and if possible drive to desired height
        display.setSegments (P,1,0);
        display.setSegments (empty,1,1);
        display.setSegments (One,1,2);
        display.setSegments (empty,1,3);
        delay (100);
        int desired_height = pos1_height;
        while (current_height < desired_height){
          checkHeight();
          Serial.print ("current: "); Serial.println(current_height);
          Serial.print ("desired: "); Serial.println(desired_height);
          goUp();
          if (current_height >= desired_height){ //stop automatically if the desired height is reached
            stopMoving();
            Serial.println("too high");
            display.setSegments (P,1,0);
            display.setSegments (empty,1,1);
            display.setSegments (One,1,2);
            display.setSegments (empty,1,3);
            delay (1000);
            checkHeight();
            break;
          }
          else if (current_height == 0){  //Catch Sonar-Error while table is moving
            Serial.println("Sonar Error in automated program");
            checkHeight();
            break;
          }
          if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){    //Cancel if up- or down-button is pressed during automatic procedure
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON UP");
            int digitPosition = 0;
            while (digitPosition <  4 ){  //small cancel-animation on Display
              delay (50);
              display.setSegments (Minus,1,digitPosition);
              digitPosition++;
              if (digitPosition >= 4){
                break;
              }
            };
            break;
          }
          else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
            btnUpState = LOW;
          }
      
          if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON DOWN");
            int digitPosition = 0;
            while (digitPosition <  4 ){  //small cancel-animation on Display
              delay (50);
              display.setSegments (Minus,1,digitPosition);
              digitPosition++;
              if (digitPosition >= 4){
                break;
              }
            };
            break;
          }
          else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
            btnDownState = LOW;
          }
        };
        Serial.println("End");
        delay (500);
        checkHeight();
        delay (1500);
        display.clear();
       };
   };
};


void checkHeight() {    // Get Sensor Reading and display on 7-Segment
  display.setBrightness(1);
  current_height = ultrasonic.read();
  Serial.print("current height: "); Serial.println(current_height);
  if (current_height != oldDistance) {  //avoid flickering of 7-segment as it now only refreshes if the value has changed
    display.showNumberDec(current_height, false);
    oldDistance = current_height;
  }
  if (current_height == 0) { //display "Err2" if the sonar sensor has an error"
    display.setSegments (Err,3,0);
    display.setSegments (Two,1,3);
    Serial.println("Sonar Sensor Error");
    };
  delay(100); //polling frequency of the sonar sensor. Not recommended to go below 30-50 ms
}


/****************************************
  MAIN CONTROL FUNCTIONS
****************************************/
//If you didn't enter auto-raise or auto-lower by doing the magic combination (2 clicks in under a second + click and hold 2 secs)
//then this function resets the clicks back to zero and the timer. handles both UP and DOWN buttons
void checkButtonClicksExpired(){
  //Check threshold for button UP
  if (btnUpFirstClickTime > 0 && (millis() - btnUpFirstClickTime) >= PRG_ACTIVATE_PRECLICK_THRESHOLD)
  {
    Serial.println("BUTTON UP | PRECLICK_THRESHOLD expired, clicks = 0");
    btnUpClicks = 0;
    btnUpFirstClickTime = 0;
  }

  //Check threshold for button DOWN
  if (btnDownFirstClickTime > 0 && (millis() - btnDownFirstClickTime) >= PRG_ACTIVATE_PRECLICK_THRESHOLD)
  {
    Serial.println("BUTTON DOWN | PRECLICK_THRESHOLD expired, clicks = 0");
    btnDownClicks = 0;
    btnDownFirstClickTime = 0;
  }
}


//This function takes care of the events related to pressing BUTTON_UP, and only BUTTON_UP. It raises the desk when holding it, and if you do the 
//magic combination (click, click, click+hold 2 secs) it will trigger auto-raise if recorded
void handleButtonUp(){
  //If button has just been pressed, we count the presses, and we also handle if it's kept hold to raise desk / enter program
  if (!BUTTON_UP_STATE && debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.print("BUTTON UP | Pressed [");
    Serial.print(btnUpClicks);
    Serial.println("] times");    
    BUTTON_UP_STATE = HIGH;
    if (btnUpClicks == 0)
    {
      btnUpFirstClickTime = millis();
    }
    btnUpClicks++;
    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_UP) && !autoRaiseActivated)
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON UP | Holding | elapsed: ");
      Serial.print(elapsed);
      Serial.print(" | Clicks: ");
      Serial.println(btnUpClicks);
      checkHeight();

      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME)
      {
        //if 2 clicks + hold for 2 secs, enter auto-raise
        if (btnUpClicks >= PRG_ACTIVATE_PRECLICKS && elapsed >= PRG_ACTIVATE_HOLD_THRESHOLD) 
        {
          autoRaiseActivated = true;
          break;
        }
        else
        {
          goUp();
        }
      }

      //If you press down while holding UP, you indicate desire to enter program mode, break the loop to stop going
      //UP, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_DOWN, LOW))
      {
        Serial.println("BUTTON UP | Button DOWN pressed, breaking loop");
        break;
      }
    }

    if (autoRaiseActivated)
    {
      Serial.print("BUTTON UP | Going up automatically | Elapsed to activate:");
      Serial.println(elapsed);
      autoRaiseDesk(elapsed);
      autoRaiseActivated = false;
      Serial.println("BUTTON UP | PrgUp Deactivated");
    }

    stopMoving();
  }
  else if (BUTTON_UP_STATE && !debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.println("BUTTON UP | Released");
    BUTTON_UP_STATE = LOW;
    checkHeight();
    delay(1500);
    display.clear();
  }
}

//This function takes care of the events related to pressing BUTTON_DOWN, and only BUTTON_DOWN. It lowers the desk when holding it, and if you do the
//magic combination (click, click, click+hold 2 secs) it will trigger auto-lower if recorded
void handleButtonDown()
{
  //If button has just been pressed, we count the presses, and we also handle if it's kept hold to lower desk / enter program
  if (!BUTTON_DOWN_STATE && debounceRead(BUTTON_DOWN, BUTTON_DOWN_STATE))
  {
    Serial.print("BUTTON DOWN | Pressed [");
    Serial.print(btnDownClicks);
    Serial.println("] times");
    BUTTON_DOWN_STATE = HIGH;
    if (btnDownClicks == 0)
    {
      btnDownFirstClickTime = millis();
    }
    btnDownClicks++;

    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_DOWN) && !autoLowerActivated)
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON DOWN | Holding | elapsed: ");
      Serial.print(elapsed);
      Serial.print(" | Clicks: ");
      Serial.println(btnDownClicks);
      checkHeight();
      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME) 
      {
        //if 2 clicks + hold for 2 secs, enter auto-lower
        if (btnDownClicks >= PRG_ACTIVATE_PRECLICKS && elapsed >= PRG_ACTIVATE_HOLD_THRESHOLD) 
        {
          autoLowerActivated = true;
          break;
        }
        else
        {
          //checkHeight();  //for testing ultrasonic sensor
          goDown();
        }
      }

      //If you press UP while holding DOWN, you indicate desire to enter program mode, break the loop to stop going
      //DOWN, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_UP, LOW))
      {
        Serial.println("BUTTON DOWN | Button UP pressed, breaking loop");
        break;
      }
    }

    if (autoLowerActivated)
    {
      Serial.print("BUTTON DOWN | Going DOWN automatically | Elapsed to activate:");
      Serial.println(elapsed);
      autoLowerDesk(elapsed);
      autoLowerActivated = false;
      Serial.println("BUTTON DOWN | PrgDown Deactivated");
    }

    stopMoving();
  }
  else if (BUTTON_DOWN_STATE && !debounceRead(BUTTON_DOWN, BUTTON_DOWN_STATE))
  {
    Serial.println("BUTTON DOWN | Released");
    BUTTON_DOWN_STATE = LOW;
    checkHeight();
    delay(1500);
    display.clear();
  }
}




/****************************************
  LOWER / RAISE DESK FUNCTIONS
****************************************/
// Tries to raise the desk automatically using the previously programmed values
// This methode doesn't utilize the sonar sensor
void autoRaiseDesk(long alreadyElapsed)
{
  Serial.print("IsUpSet: ");
  Serial.println(savedProgram.isUpSet);
  if (savedProgram.isUpSet)
  {
    //we subtract the amount of ms that already elapsed to enter auto-mode (about 2 seconds in default configs)
    long timeToRaise = savedProgram.timeUp - alreadyElapsed;
    Serial.print("AUTORAISE | PRG_TimeUp: "); Serial.print(savedProgram.timeUp);
    Serial.print(" | TimeToRaise: "); Serial.println(timeToRaise);
    long startTime = millis();
    long elapsed = (millis() - startTime);
    bool btnUpState = digitalRead(BUTTON_UP);
    bool btnDownState = digitalRead(BUTTON_DOWN);

    while(elapsed < timeToRaise){
      goUp();

      //Cancel if any button is pressed during autoRaise
      if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){
        Serial.println("Program Cancelled by user, BUTTON UP");
        break;
      }else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
        btnUpState = LOW;
      }

      if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
        Serial.println("Program Cancelled by user, BUTTON DOWN");
        break;
      }
      else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
        btnDownState = LOW;
      }

      elapsed = (millis() - startTime);
    }
    stopMoving();
    long stopTime = millis();
    long totalElapsed = stopTime - startTime;
    Serial.print("AUTORAISE | Auto-TotalElapsed:"); Serial.println(totalElapsed);
  }
  else
  {
    Serial.println("Error: Up Program not set");
  }
}

// Tries to lower the desk automatically using the previously programmed values
// This methode doesn't utilize the sonar sensor
void autoLowerDesk(long alreadyElapsed)
{
  Serial.print("IsDownSet: ");
  Serial.println(savedProgram.isDownSet);
  if (savedProgram.isDownSet)
  {
    long timeToLower = savedProgram.timeDown - alreadyElapsed;
    Serial.print("AUTOLOWER | PRG_TimeDown: "); Serial.print(savedProgram.timeDown);
    Serial.print(" | TimeToLower: "); Serial.println(timeToLower);
    long startTime = millis();
    long elapsed = (millis() - startTime);
    bool btnUpState = digitalRead(BUTTON_UP);
    bool btnDownState = digitalRead(BUTTON_DOWN);
    
    while (elapsed < timeToLower)
    {
      goDown();

      //Cancel if any button is pressed during auto-lower
      if (!btnUpState && debounceRead(BUTTON_UP, btnUpState)){
        Serial.println("Program Cancelled by user, BUTTON UP");
        break;
      }
      else if (btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
        btnUpState = LOW;
      }

      if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
        Serial.println("Program Cancelled by user, BUTTON DOWN");
        break;
      }
      else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
        btnDownState = LOW;
      }

      elapsed = (millis() - startTime);
    }
    stopMoving();
    long stopTime = millis();
    long totalElapsed = stopTime - startTime;
    Serial.print("AUTOLOWER | Auto-TotalElapsed:"); Serial.println(totalElapsed);
  }
  else
  {
    Serial.println("Warning: Can't lower desk on current conditions");
  }
}

//Send PWM signal to L298N enX pin (sets motor speed)
void goUp()
{
  Serial.print("UP:"); Serial.println(PWM_SPEED_UP);
  digitalWrite(LED_BUILTIN, HIGH);

  //Motor A: Turns in (LH) direction
  analogWrite(enA, PWM_SPEED_UP);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  //Motor B: Turns in OPPOSITE (HL) direction
  analogWrite(enB, PWM_SPEED_UP);
  digitalWrite(in4, HIGH);
  digitalWrite(in3, LOW);
}

//Send PWM signal to L298N enX pin (sets motor speed)
void goDown()
{
  Serial.print("DOWN:");Serial.println(PWM_SPEED_DOWN);
  digitalWrite(LED_BUILTIN, HIGH);
  
  //Motor A: Turns in (HL) Direction
  analogWrite(enA, PWM_SPEED_DOWN);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  //Motor B: Turns in OPPOSITE (LH) direction
  analogWrite(enB, PWM_SPEED_DOWN);
  digitalWrite(in4, LOW);
  digitalWrite(in3, HIGH);
}

void stopMoving()
{
  analogWrite(enA, 0);
  analogWrite(enB, 0);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Idle...");
}

/****************************************
  EEPROM FUNCTIONS
****************************************/
//Saves the Time it took to raise the desk (timeUP) and sets the desk as Raised
void saveToEEPROM_TimeUp(long timeUp)
{
  Serial.println("Saving UP to EEPROM");
  savedProgram.timeUp = (float)timeUp;
  savedProgram.isUpSet = true;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
}

//Saves the Time it took to raise the desk (timeUP) and sets the desk as Lowered (!Raised)
void saveToEEPROM_TimeDown(long timeDown)
{
  Serial.println("Saving DOWN to EEPROM");
  savedProgram.timeDown = (float)timeDown;
  savedProgram.isDownSet = true;
  EEPROM.put(EEPROM_ADDRESS, savedProgram);
}

void readFromEEPROM()
{
  Serial.println("Reading from EEPROM");
  Serial.print("Position 0 is: ");
  Serial.print(pos0_height);
  Serial.print("cm | Position 1 is: ");
  Serial.print(pos1_height);
  Serial.println("cm");
  EEPROM.get(EEPROM_ADDRESS, savedProgram);
  long timeUpInt = (long)savedProgram.timeUp;
  Serial.print("TimeUp: ");
  Serial.print(timeUpInt);
  long timeDownInt = (long)savedProgram.timeDown;
  Serial.print(" | TimeDown: ");
  Serial.print(timeDownInt);
  Serial.print(" | IsUPSet: ");
  Serial.print(savedProgram.isUpSet);
  Serial.print(" | IsDownSet: ");
  Serial.println(savedProgram.isDownSet);
}

void validateEEPROM(){
  if(isnan(savedProgram.timeUp) || isnan(savedProgram.timeDown)){
    Serial.println("Warning: timeUp/Down are NAN, Re-initializing");
    savedProgram.timeUp = 0;
    savedProgram.timeDown = 0;
    savedProgram.isUpSet = false;
    savedProgram.isDownSet = false;
    EEPROM.put(EEPROM_ADDRESS, savedProgram);
  }
}

void clearEEPROM(){
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

//Checks if the user is requesting to enter program mode (by pressing and holding both buttons), if so, returns true, otherwise, false
bool handleProgramMode(){
  if (debounceRead(BUTTON_DOWN, LOW) && debounceRead(BUTTON_UP, LOW))
  {
    Serial.println("BOTH BUTTONS PRESSED");
    if (holdButtonsStartTime == 0)
      holdButtonsStartTime = millis();

    //Enter program mode
    if ((millis() - holdButtonsStartTime) >= PRG_SET_THRESHOLD)
    {
      Serial.println("PROGRAM MODE | Entered Program Mode");
      //long enterTime = millis();
      long recStart = 0;
      bool recUp = false;
      bool recDown = false;
      bool btnUpPressed = digitalRead(BUTTON_UP);
      bool btnDownPressed = digitalRead(BUTTON_DOWN);

      while (true)
      {
        //long elapsed = (millis() - enterTime);
        if (!recUp && !recDown)
          
        //HANDLE PROGRAM - RAISE
        //when entering this loop. button UP WAS already pressed, we need to wait until it's released, then start recording
        //if button wasn't pressed and now IS pressed, start recording
        if(!recDown){
          if (!btnUpPressed && debounceRead(BUTTON_UP, btnUpPressed))
          {
            btnUpPressed = true;
            recUp = true;
            recStart = millis();
          } //if button was pressed and now is released
          else if (btnUpPressed && !debounceRead(BUTTON_UP, btnUpPressed))
          {
            btnUpPressed = false;
            if (recUp)
            {
              recUp = false;
              stopMoving();
              long timeUp = millis() - recStart;
              saveToEEPROM_TimeUp(timeUp);
              Serial.print("Recorded Program UP: ");
              Serial.println(timeUp);
              break;
            }
          }

          if (recUp)
          {
            Serial.print("PROGRAM MODE | Recording | elapsed: ");
            Serial.println(millis() - recStart);
            goUp();
          }
        }
        
        //HANDLE PROGRAM - LOWER
        //when entering this loop. button DOWN WAS already pressed, we need to wait until it's released, then start recording
        //if button wasn't pressed and now IS pressed, start recording
        if(!recUp){
          if (!btnDownPressed && debounceRead(BUTTON_DOWN, btnDownPressed))
          {
            btnDownPressed = true;
            recDown = true;
            recStart = millis();
          } //if button was pressed and now is released
          else if (btnDownPressed && !debounceRead(BUTTON_DOWN, btnDownPressed))
          {
            btnDownPressed = false;
            if (recDown)
            {
              recDown = false;
              stopMoving();
              long timeDown = millis() - recStart;
              saveToEEPROM_TimeDown(timeDown);
              Serial.print("Recorded Program DOWN: ");
              Serial.println(timeDown);
              break;
            }
          }

          if (recDown)
          {
            Serial.print("PROGRAM MODE | Recording Down| elapsed: ");
            Serial.println(millis() - recStart);
            goDown();
          }
        }

      } //end of while / program mode

      Serial.println("PROGRAM MODE | Exited");
    }

    return true;
  }
  else
  {
    holdButtonsStartTime = 0;
    return false;
  }
}