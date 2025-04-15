/* 
 * EE1301 Final Project
 * Author: Andrew Paetznick
 * Date: Nov 16, 2024
 */
#include "Particle.h"
#include "MyKeypad.h"
#include "neopixel.h"

SYSTEM_MODE(AUTOMATIC);

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

// These lines of code should appear AFTER the #include statements, and before
// the setup() function.
// IMPORTANT: Set pixel COUNT, PIN and TYPE
int PIXEL_COUNT = 1;
#define PIXEL_PIN SPI // Only use SPI or SPI1 on Photon 2 (SPI is MO or S0 pin; SPI1 is D2)
                      // NOTE: On the Photon 2, this must be a compiler constant!
int PIXEL_TYPE = WS2812;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

//declaring all functions used
bool checkPassword(String passwordIn);
bool checkPasswordCloud(String passwordIn);
int changePasswordFromCloud(String passwordIn);
void checkOpen();
void flashLED(int green, int red, int blue);
bool newPassCloud(String passwordIn);
int lock(String garbage);


//keypad location
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte colPins[ROWS] = {D5, D4, D3, D2}; //connect to the row pinouts of the keypad
byte rowPins[COLS] = {D0, D10, D7, D6}; //connect to the column pinouts of the keypad

MyKeypad keypad = MyKeypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//servo for lock
Servo myServo; 

//declaration for password variables
//string to hold input buttons
String onHold;

//string to hold current password to check against
String password;

//boolean if a new password is being input
boolean newPass;

//for changing password path in cloud
bool cloudChange = false;

//Data that is visible in the cloud interface
boolean isOpen;
String isOpenStr;

//long timer to refresh is_Open every few seconds
//unsigned long refresher = time(0);

void setup() {
  //servo starts at a closed point
  myServo.attach(A2);
  myServo.write(0);
  isOpen = false;
  isOpenStr = "Closed";
  
  //initializes the password to 1234
  password = "1234";

  //initializes the aRGB
  strip.begin();
  strip.show();

  //variable for the password to change online 
  Particle.variable("passwordCheck", password);
  Particle.variable("is_Open", isOpenStr);

  //function to check input password in html is correct
  Particle.function("cF_checkPass", checkPasswordCloud);
  Particle.function("cF_changePass", changePasswordFromCloud);
  Particle.function("cF_lock", lock);
  Particle.function("cF_correctPass", newPassCloud);


  //starts checking in terminal
  Serial.begin(9600);
}



void loop() {

  char key = keypad.getKey();
  if(key){
    //prints any input to the terminal 
    Serial.println(key);

    //if the create new password key is pressed '*'
    if(key == '*'){

      //sets the next ability to change the password
      if(checkPassword(onHold)){
        newPass = true;
        Serial.println("password is correct and ready to change");
        flashLED(100,100,0);
      } else{ //if the input password is wrong
        Serial.println("incorrect password to change");
        flashLED(0,100,0);
      }
      //always clears the input
      onHold = "";
    }

    //if the check password key is pressed '#'
    else if(key == '#'){
      //for opening the door
      if(isOpen){
        myServo.write(0);
        isOpen = !isOpen;
        isOpenStr = "Closed";
        Serial.println("closing");
        flashLED(100,0,100);
        checkOpen();
      }
      else if(!newPass){
        if(checkPassword(onHold)){
          myServo.write(180);
          isOpen = !isOpen;
          isOpenStr = "Open";
          Serial.println("opening");
          checkOpen();
          flashLED(200,0,0);
        }
        else{
        Serial.print(onHold);
        Serial.println(" incorrect password");
        flashLED(0,200,0);
        }
        onHold = "";
      }
      //for changing the password
      else if(newPass){
        Serial.println("password change successful");

        flashLED(200,200,200);
        password = onHold;
        newPass = false;
        onHold = "";
      }
      else{
        Serial.println("incorrect password");
        flashLED(0,200,0);
        onHold = "";
      }

    }
    //if a number key is pressed
    else{
      onHold += key;
    }
  }

  //owner combo
  if(onHold == "ABCD"){
    newPass = true;
    flashLED(100,255,100);
    onHold = "";
  }

  //end of keypad press code
}





//checks if the password input is the password
bool checkPassword(String passwordIn){
  if(passwordIn == password)
    return true;
  else
    return false;
}

bool checkPasswordCloud(String passwordIn){
  if(passwordIn == password){
    myServo.write(180);
    Serial.println("Opening from cloud");
    flashLED(100,0,0);
  }
  else{
    Serial.println("Incorrect password from cloud");
  }
  if(myServo.read() == 180){
    isOpen = TRUE;
    checkOpen();
    return true;
  }
  else  
    return false;
}

bool newPassCloud(String passwordIn){
  if(password == passwordIn){
    cloudChange = true;
    Serial.print("Password ready to be changed");
    return 1;
  }
  else{
    Serial.print("Incorrect Password");
    return 0;
  }
}

int changePasswordFromCloud(String passwordIn){
  onHold = "";
  if(cloudChange){
    password = passwordIn;
    //shows an optimal functioning change
    Serial.print(password);
    Serial.print("Password Successfully changed");
    cloudChange = false;
    flashLED(0,100,100);
    return 1;    
  }
  //fails the password change
  Serial.println("Input the correct password first");
  return -1;
}

//does not need the string, automatically locks the door
int lock(String garbage){
  myServo.write(0);
  isOpenStr = "Closed";
  isOpen = FALSE;
  Serial.println("closing from cloud");
  flashLED(100,0,100);
  checkOpen();
  if(!isOpen)
    return 1;
  else
    return -1;
}

void checkOpen(){
  if(isOpen){
    isOpenStr = "Open";
  }
  else{
    isOpenStr = "Closed";
  }
}

//1 for green, 2 for yellow, 3 for red, 4 for purple, 5 for 
void flashLED(int green, int red, int blue){
  int pixelColor = strip.Color(green, red, blue);
  for(int i = 0; i < 3; i++){
    strip.setPixelColor(0, pixelColor);
    strip.show();
    delay(100);
    strip.setPixelColor(0, strip.Color(0,0,0));
    strip.show();
  }
}