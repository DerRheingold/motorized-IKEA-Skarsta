/*
  ----------------------------
   Arduino Power Desktop PLUS
  ----------------------------
  SUMMARY 
    Controls the direction and speed of the motor, supports auto-raising and auto-lowering by measuring the distance via sonar sensor
  
  DESCRIPTION 
    This code waits for a button to be pressed (UP or DOWN), and accordingly powers the motors in the desired direction and speed.
    One motor will turn clockwise, the other counterclockwise. The motors are intended to be placed facing each other, to duplicate the torque on the allen wrench
    On my particular setup (a heavy water cooled tower-PC and one 49" monitor totaling around 35-40 kg) I use 100% of the power speed and torque when raising the desk,
    however, when lowering the desk I need a bit less since gravity helps, to keep the speed up and down at a similar rate, this is configurable.
    You may want to tweak the variables PWM_SPEED_UP and PWM_SPEED_DOWN to adjust it to your desktop load, the allowed values
    are 0 (min) to 255 (max).

  BASIC USAGE
    - Press and hold BUTTON_UP to raise the desk. a small delay of 250ms has been introduced for smoothness
    - Press and hold BUTTON_DOWN to lower the desk. a small delay of 250ms has been introduced for smoothness
    - Press and hold the "Position 0" button to save the lower/sitting position
    - Press and hold the "Position 1" button to save the higher/standing position
    - Press the "Position 0" button shortly to automatically have the desk drive into this position. It stops as soon as the sonar sensor reads the distance from the ground is equal or lower than saved
    - Press the "Position 1" button shortly to automatically have the desk drive into this position. It stops as soon as the sonar sensor reads the distance from the ground is equal or higher than saved
    - The desk should also automatically stop as soon as there is an error in the sonar reading

  ERROR CODES
    Err0: When trying to save a sitting position that is HIGHER than a standing position "Err0" will be shown in the display
    Err1: When trying to save a standing position that is LOWER than a sitting position "Err1" will be shown in the display
    Err2: If there is an error in the sonar (be it while manually or automatically moving the desk) "Err2" is shown in the display. The automatic program will stop directly. Manual adjustment is still possible.


  You may use this code and all of the diagrams and documentations completely free. Enjoy!

  CREDITS
  - https://github.com/cesar-moya/arduino-power-desktop <-- most of the code
  - https://github.com/aenniw/ARDUINO/tree/master/skarsta <-- idea with the 7-segment display and 3d-print files
  
  Original Author:  Cesar Moya
             Date:  July 28th, 2020
              URL:  https://github.com/cesar-moya/arduino-power-desktop
  Updated by:       https://github.com/DerRheingold/motorized-IKEA-Skarsta
*/
#include <EEPROM.h>
#include <Arduino.h>
#include <TM1637Display.h>
#include <Ultrasonic.h>

/* TO DO
- 
*/

#define BUTTON_UP 2     
#define BUTTON_DOWN 3   
#define BUTTON_POS_0 4  
#define BUTTON_POS_1 5  
#define enA 6           
#define in1 7           
#define in2 8           
#define enB 10          
#define in3 11          
#define in4 12          
#define CLK 14          // 7 Segment
#define DIO 15          // 7 Segment
#define ECHO_PIN 16     // Arduino pin tied to echo pin on the ultrasonic sensor
#define TRIGGER_PIN 17  // Arduino pin tied to trigger pin on the ultrasonic sensor

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
TM1637Display display(CLK, DIO);

// Definitions for Platformio
void readFromEEPROM();
void handleButtonUp();
void handleButtonDown();
void position_0();
void position_1();
void checkHeight();
void goDown();
void goUp();
void stopMoving();

//Struct to store the various necessary variables to persist the autoRaise/autoLower programs to EEPROM
struct StoredProgram
{
  int pos0Height = 0; //height in cm above ground for the sitting position
  int pos1Height = 0; //height in cm above ground for the standing position
};

StoredProgram savedProgram;
int EEPROM_ADDRESS = 0;
int BUTTON_UP_STATE = LOW;
int BUTTON_DOWN_STATE = LOW;
int BUTTON_POS_0_STATE = LOW;
int BUTTON_POS_1_STATE = LOW;
long BUTTON_WAIT_TIME = 250; //the small delay before starting to go up/down for smoothness on any button

//Using custom values to ensure no more than 24v are delivered to the motors given my desk load.
//feel free to play with these numbers but make sure to stay within your motor's rated voltage.
const int PWM_SPEED_UP = 255; //0 - 255, controls motor speed when going UP
const int PWM_SPEED_DOWN = 220; //0 - 255, controls motor speed when going DOWN

// Required for additional buttons
long pressedTime;
long releasedTime;
long TimePressed;
const int LONG_PRESS_TIME  = 2000; // The time button "0" or "1" need to be pressed to register as a "long" press to save the current position to eeprom.

// Required for the ultrasonic sensor
int old_Height;
int current_height = 0;
int pos0_height = 0;
int pos1_height = 0;

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
const uint8_t E[] = {
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, // E
};
const uint8_t R[] = {
  SEG_E | SEG_G, // r
  SEG_E | SEG_G, // r
};
const uint8_t Minus[] = {
  SEG_G  // -
};
const uint8_t smallO [] = {
  SEG_C | SEG_D | SEG_E | SEG_G // o
};
const uint8_t circle [] = {
  SEG_A | SEG_B | SEG_F | SEG_G  // ° 
};
const uint8_t empty[] = {0x0}; //blank segment for 7-Segment display
        

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

void animation (const uint8_t* symbol, int delayTime){
  int digitPosition = 0;
  while (digitPosition <  4 ){  
    delay (delayTime);
    display.setSegments (symbol,1,digitPosition);
    digitPosition++;
    if (digitPosition >= 4){
      break;
    }
  };
}

void showOnDisplay (const uint8_t* firstChar, const uint8_t* secondChar, const uint8_t* thirdChar, const uint8_t* fourthChar){
  display.setSegments (firstChar,1,0);  
  display.setSegments (secondChar,1,1);
  display.setSegments (thirdChar,1,2);
  display.setSegments (fourthChar,1,3);
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
  //current_height = ultrasonic.read();
  display.setBrightness(1);
  display.clear();
  //Some start-up-animation on Display
  animation (smallO, 100);
  animation (circle, 100);
  animation (Zero, 100);
  animation (empty, 100);
  display.clear();
  delay (400);
  // Display the saved Position 0 height on the display upon startup
  showOnDisplay (P, empty, Zero, empty);
  delay (1000);
  display.showNumberDec(pos0_height, false);
  delay (1500);
  display.clear();
  delay (400);
  // Display the saved Position 1 height on the display upon startup
  showOnDisplay (P, empty, One, empty);
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
  //Handle press and hold of buttons to raise/lower, and check if enter auto-raise and auto-lower
  handleButtonUp();
  handleButtonDown();

  //Handle press and hold of buttons to drive into a saved position (short press) or to save the current position (long press)
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
   if (BUTTON_POS_0_STATE==LOW && debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){  //define what to do when the button is pressed 
       BUTTON_POS_0_STATE=HIGH;
       Serial.println("BUTTON Position 0 Pressed");
       pressedTime = millis(); 
       int digitPosition = 0;
       while (btnPos0State && debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){  //small animation on Display while button is held down longer than 500 ms
         delay (400);
         display.setSegments (smallO,1,digitPosition);
         digitPosition++;
         if (digitPosition >= 4){
          delay (400);
          showOnDisplay (Zero, Zero, Zero, Zero);
          break;
          }
        }
      }
   else if (BUTTON_POS_0_STATE==HIGH && !debounceRead(BUTTON_POS_0, BUTTON_POS_0_STATE)){ //releasing the button checks how long it was pressed and then decides what to do
       BUTTON_POS_0_STATE=LOW;
       releasedTime = millis();   
       TimePressed = releasedTime-pressedTime;

       //If "Position 0 button" is long-pressed, save current height to Position 0, display "P 0" and the height in cm in the display
       if (TimePressed >= LONG_PRESS_TIME){  
        int pos0SaveHeight = ultrasonic.read();
        pos1_height = savedProgram.pos1Height;
        if (pos0SaveHeight >= pos1_height) {  //Check if Position 0 is lower than Position 1. If not, display "Err0"
          Serial.print ("must be lower than "); 
          Serial.println (pos1_height);
          showOnDisplay (E, R, R, Zero);
          delay (1000);
          display.clear();
        }
        else { // Save height and give output to user
          savedProgram.pos0Height = pos0SaveHeight;
          EEPROM.put(EEPROM_ADDRESS, savedProgram);
          Serial.print("Saved Position 0: ");
          Serial.println(pos0SaveHeight);
          showOnDisplay (P, empty, One, empty);
          delay (1000);
          display.showNumberDec(pos0SaveHeight, false);
          delay (1000);
          display.clear();
        };          
       }
         
       if (TimePressed < LONG_PRESS_TIME){  //If "Position 0 button" is short-pressed, check height and if possible drive to desired height
        showOnDisplay (P, empty, Zero, empty);
        delay (100);
        int desired_height = pos0_height;
        if (current_height == 0){ //Catch Sonar-Error before starting program
          Serial.println("Won't start Down-Program because of Sonar Error.");
        };
        while (current_height > desired_height){
          checkHeight();
          Serial.print ("desired: "); Serial.println(desired_height);
          goDown();
          if (current_height <= desired_height && current_height != 0){ //stop automatically if the desired height is reached
            delay (500); //a short delay to compensate for sensor inaccuracy.
            stopMoving();
            Serial.println("Sitting position reached");
            showOnDisplay (P, empty, Zero, empty);
            delay (1000);
            checkHeight();
            break;
          }
          else if (current_height == 0){  //Catch Sonar-Error while table is moving and abort program
            Serial.println("Sonar Error in automated down-program");
            checkHeight();
            break;
          }
          if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){    //Cancel if up- or down-button is pressed during automatic procedure
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON UP");
            animation (Minus, 50);
            break;
          }
          else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
            btnUpState = LOW;
          }
      
          if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON DOWN");
            animation (Minus, 50);
            break;
          }
          else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
            btnDownState = LOW;
          }
        };
        Serial.println("End Down-Program");
        stopMoving();
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
       Serial.println("BUTTON Position 1 Pressed");
       pressedTime = millis(); 
       int digitPosition = 0;
       while (btnPos1State && debounceRead(BUTTON_POS_1, BUTTON_POS_1_STATE)){  //small animation on Display while button is pressed
         delay (400);
         display.setSegments (smallO,1,digitPosition);
         digitPosition++;
         if (digitPosition >= 4){
          delay (400);
          showOnDisplay (Zero, Zero, Zero, Zero);
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
        pos0_height = savedProgram.pos0Height;
        if (pos1SaveHeight <= pos0_height) {  //Check if Position 1 is higher than Position 0. If not, display "Err1"
          Serial.print ("must be higher than "); 
          Serial.println (pos0_height);
          showOnDisplay (E, R, R, One);
          delay (1000);
          display.clear();
        }
        else { // Save height and give output to user
          savedProgram.pos1Height = pos1SaveHeight;
          EEPROM.put(EEPROM_ADDRESS, savedProgram);
          Serial.print("Saved Position 1: ");
          Serial.println(pos1SaveHeight);
          showOnDisplay (P, empty, One, empty);
          delay (1000);
          display.showNumberDec(pos1SaveHeight, false); 
          delay (1000);
          display.clear();
        }        
       }
       
       if (TimePressed < LONG_PRESS_TIME){ //If "Position 1 button" is short-pressed, check height and if possible drive to desired height
        showOnDisplay (P, empty, One, empty);
        delay (100);
        int desired_height = pos1_height;
        if (current_height == 0){ //Catch Sonar-Error before starting program
          Serial.println("Won't start Up-Program because of Sonar Error.");
        };
        while (current_height < desired_height && current_height != 0){
          checkHeight();
          Serial.print ("desired: "); Serial.println(desired_height);
          goUp();
          if (current_height >= desired_height){ //stop automatically if the desired height is reached
            delay (500); //a short delay to compensate for sensor inaccuracy.
            stopMoving();
            Serial.println("Standing position reached");
            showOnDisplay (P, empty, One, empty);
            delay (1000);
            checkHeight();
            break;
          }
          else if (current_height == 0){  //Catch Sonar-Error while table is moving and abort program
            Serial.println("Sonar Error in automated up-program");
            checkHeight();
            break;
          }
          if(!btnUpState && debounceRead(BUTTON_UP, btnUpState)){    //Cancel if up- or down-button is pressed during automatic procedure
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON UP");
            animation (Minus, 50);
            break;
          }
          else if(btnUpState && !debounceRead(BUTTON_UP, btnUpState)){
            btnUpState = LOW;
          }
      
          if (!btnDownState && debounceRead(BUTTON_DOWN, btnDownState)){
            stopMoving();
            Serial.println("Program Cancelled by user, BUTTON DOWN");
            animation (Minus, 50);
            break;
          }
          else if (btnDownState && !debounceRead(BUTTON_DOWN, btnDownState)){
            btnDownState = LOW;
          }
        };
        Serial.println("End Up-Program");
        stopMoving();
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
  if (current_height != old_Height && current_height != 0) {  //avoid flickering of 7-segment as it now only refreshes if the value has changed
    Serial.print("current height: "); Serial.println(current_height);
    display.showNumberDec(current_height, false);
    old_Height = current_height;
  }
  if (current_height == 0) { //display "Err2" if the sonar sensor has an error"
    showOnDisplay (E, R, R, Two);
    Serial.println("Sonar Sensor Error");
    };
  delay(100); //polling frequency of the sonar sensor. Not recommended to go below 30-50 ms
}


/****************************************
  MAIN CONTROL FUNCTIONS
****************************************/
//This function takes care of the events related to pressing BUTTON_UP, and only BUTTON_UP. It raises the desk when holding it
void handleButtonUp()
{
  //If button is held the desk raises
  if (!BUTTON_UP_STATE && debounceRead(BUTTON_UP, BUTTON_UP_STATE))
  {
    Serial.println("BUTTON UP Pressed");
    BUTTON_UP_STATE = HIGH;
    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_UP))
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON UP | Holding | elapsed: ");
      Serial.println(elapsed);
      checkHeight();

      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME)
      {
        goUp();
      }

      //If you press down while holding UP, you indicate desire to enter program mode, break the loop to stop going
      //UP, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_DOWN, LOW))
      {
        Serial.println("BUTTON UP | Button DOWN pressed, breaking loop");
        break;
      }
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

//This function takes care of the events related to pressing BUTTON_DOWN, and only BUTTON_DOWN. It lowers the desk when holding it
void handleButtonDown()
{
  //If button is held the desk lowers
  if (!BUTTON_DOWN_STATE && debounceRead(BUTTON_DOWN, BUTTON_DOWN_STATE))
  {
    Serial.println("BUTTON DOWN | Pressed");
    BUTTON_DOWN_STATE = HIGH;
    long pressTime = millis();
    long elapsed = 0;
    while (digitalRead(BUTTON_DOWN))
    {
      elapsed = millis() - pressTime;
      Serial.print("BUTTON DOWN | Holding | elapsed: ");
      Serial.println(elapsed);
      checkHeight();

      //small delay before starting to work for smoothness
      if (elapsed >= BUTTON_WAIT_TIME) 
      {
        goDown();
      }

      //If you press UP while holding DOWN, you indicate desire to enter program mode, break the loop to stop going
      //DOWN, the code will catch the 2 buttons being held separately
      if (debounceRead(BUTTON_UP, LOW))
      {
        Serial.println("BUTTON DOWN | Button UP pressed, breaking loop");
        break;
      }
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
void readFromEEPROM()
{
  Serial.println("Reading from EEPROM");
  EEPROM.get(EEPROM_ADDRESS, savedProgram);
  Serial.print("Position 0: ");
  Serial.print(savedProgram.pos0Height);
  pos0_height = savedProgram.pos0Height;
  Serial.print("cm | Position 1: ");
  Serial.print(savedProgram.pos1Height);
  Serial.println("cm");
  pos1_height = savedProgram.pos1Height;
}

void clearEEPROM(){
  int eeprom_length = EEPROM.length();
  for (int i = 0; i < eeprom_length; i++) {
    EEPROM.write(i, 0);
  }
}