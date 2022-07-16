/* 
Open Tag, a DIY laser tag game

Software V 0.99
Hardware V 0.99

All of the variables you can change to change how the game is played are shown below.
After that, there are all the variables needed to run the game.
*/

//include TimerOne
#include <TimerOne.h>
#include <SoftwareSerial.h>

//Device ID
//this is the id of the unit. It's who is sending the tag. it can be anything between 0 and 63
//id number 63 is reserved for the controller
#define DEVICE_ID 63

#define TX_TAG 7
#define RX_TAG 3

SoftwareSerial mySerial(RX_TAG, TX_TAG); // RX, TX

#define TAG_ARRAY_LEN 4         //tags have 4 bytes
byte SENT_ARRAY_TAG[] = {0, DEVICE_ID, 0, 0};

//new library
#include "ssd1306.h"
#include "nano_gfx.h"

//DEBUGGING
#define SERIAL_DEBUG 0      //1 turns on debugging. 0 turns it off
//so, this only works when Serial_debug_mode is set to 0. oh wait. now it always fails.
#define TEST_MODE 0         //1 turns on sending test tags

#define BONUS_DAMAGE 0      //no bonus damage

#define MIN_CHANGE_GAME_STATE_VALUE 16  //any value over 16 is changing the game stats and gets treated differently

//teams can't do friendly fire and can only heal or revive each other
//the exception is the white team, which is free for all (hits everyone)
byte TEAM = 0;
#define NUMBER_OF_TEAMS 8
// 1 == red ream
// 2 == blue team
// 3 == green team (if you are out from someone on green team, you come back on green team)
// 0 == white team (white team is free for all)
// 4-7 are also valid teams. Just saying.
// 4 (orange) 5 (magenta) 6 (yellow) 7 (purple)
// team 8 is only for runes

//class - this is the variable for what type of class you are
byte CHARACTER_CLASS = 1;           //start off as soldier
#define NUMBER_OF_CLASSES 11        //this is the number of classes there are

// Defining pins (don't change these unless changing hardware)
const byte button_pin = 12;
const byte ir_led_pin = 10;

#define TAG_PIN A0
#define TEAM_PIN A1

//***** protocol definitions: how long a tag is in microseconds
//don't change unless you know what you're doing
const int protocol_error = 300;
const int protocol_base = 1200;    //this is the minimum a tag is
const int TIMEOUT = 2550;         //after 2,550 microseconds, stop timing
const int PROTOCOL_BITS = 8;      //8 bits in the protocol
#define PROTOCOL_TEAM_BITS 3      //3 bits are dedicated to the team
const int PROTOCOL_START_CODE = 1800; //start code is 1800 ms
const int PROTOCOL_0 = 600;           //0 is 600 ms
const int PROTOCOL_1 = 1200;          //1 is 1200 ms
const int PROTOCOL_DELAY = 600;       //600 ms delay between pulses
const int PROTOCOL_START_ERROR = 400; //400 ms error with start code
const int PROTOCOL_ERROR = 250;       //250 ms error with all other codes
//tags can be within prococol_error of correct value and still count

//const int protocol_num_tags = 16;  //this is the number of defined tags
/* defined tags: (all defined)
1 - normal tag
2 - heal
3 - revive
4 - flame
5 - ice
6 - poison
7 - rock armor
8 - water
9 - lightning
10 - also lightning. Not sure why, but it sends an extra 500 microseconds some times. this is a
     hardware thing (IR receivers). Using interrupts on the software, we get the same result
11 = juggernaut
27 - extra life
28 - armor
29 - mana
30 - health
31 - damage
*/
int TAG = 1;

int OLD_TAG = 0;
byte OLD_TEAM = 0;



void setup() {
  if(SERIAL_DEBUG){
    Serial.begin(9600);
    Serial.println(F("Hello World!"));
  }

  mySerial.begin(2400);       //turn on serial for receiving IR commands
  
  //new OLED
  ssd1306_128x64_i2c_init();
  
  //initialize TimerOne to pulse 38,000 times a second
  Timer1.initialize(26);

  pinMode( ir_led_pin, OUTPUT);
  pinMode( button_pin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  TAG = map(analogRead(TAG_PIN), 0, 1023, 1, 32);
  //I'm getting some jitter on 31, so I upped the map to 32, and set it to 31 for a wider range
  if(TAG == 32) TAG = 31;
  if(SERIAL_DEBUG){
        Serial.print(F("Tag pin voltage: "));
        Serial.print(analogRead(TAG_PIN));
        Serial.print(F(", Tag to send: "));
        Serial.println(TAG);
  }
  TEAM = map(analogRead(TEAM_PIN), 0, 1023, 0, 8);
  //I'm getting some jitter on 7, so I added another one
  if(TEAM == 8) TEAM = 7;
  if(SERIAL_DEBUG){
        Serial.print(F("Team pin voltage: "));
        Serial.print(analogRead(TEAM_PIN));
        Serial.print(F(", Team to send: "));
        Serial.println(TEAM);
        delay(750);
  }
  if(OLD_TAG != TAG || OLD_TEAM != TEAM){
    //if tag or team has changed, update the display
    ssd1306_normalMode();
    ssd1306_clearScreen( );
    ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
    ssd1306_printFixed(0,  16, String(TAG).c_str(), STYLE_BOLD);
    if(TAG == 1) ssd1306_printFixed(0,  0, "Soldier", STYLE_BOLD);
    if(TAG == 2) ssd1306_printFixed(0,  0, "Medic", STYLE_BOLD);
    if(TAG == 3) ssd1306_printFixed(0,  0, "Base", STYLE_BOLD);
    if(TAG == 4) ssd1306_printFixed(0,  0, "Pyro", STYLE_BOLD);
    if(TAG == 5) ssd1306_printFixed(0,  0, "Ice", STYLE_BOLD);
    if(TAG == 6) ssd1306_printFixed(0,  0, "Poison", STYLE_BOLD);
    if(TAG == 7) ssd1306_printFixed(0,  0, "Rock", STYLE_BOLD);
    if(TAG == 8) ssd1306_printFixed(0,  0, "Water", STYLE_BOLD);
    if(TAG == 9) ssd1306_printFixed(0,  0, "Lightning", STYLE_BOLD);
    if(TAG == 10) ssd1306_printFixed(0,  0, "Necrmancer", STYLE_BOLD);
    if(TAG == 11) ssd1306_printFixed(0,  0, "Juggernaut", STYLE_BOLD);
    if(TAG == 12) ssd1306_printFixed(0,  0, "Wind", STYLE_BOLD);
    if(TAG == 17) ssd1306_printFixed(0,  0, "Auto Revive On/Off", STYLE_BOLD);
    if(TAG == 18) ssd1306_printFixed(0,  0, "Survival On/Off", STYLE_BOLD);
    if(TAG == 19) ssd1306_printFixed(0,  0, "Aim Assist On/Off", STYLE_BOLD);
    if(TAG == 20) ssd1306_printFixed(0,  0, "Ammo On/Off", STYLE_BOLD);
    if(TAG == 21) ssd1306_printFixed(0,  0, "Aim detect", STYLE_BOLD);
    if(TAG == 22) ssd1306_printFixed(0,  0, "Aim detect", STYLE_BOLD);
    if(TAG == 23) ssd1306_printFixed(0,  0, "King O' Hill", STYLE_BOLD);
    if(TAG == 24) ssd1306_printFixed(0,  0, "Change to Jug", STYLE_BOLD);
    if(TAG == 25) ssd1306_printFixed(0,  0, "Nothing", STYLE_BOLD);
    if(TAG == 26) ssd1306_printFixed(0,  0, "Ammo Refill", STYLE_BOLD);
    if(TAG == 27) ssd1306_printFixed(0,  0, "Extra Life", STYLE_BOLD);
    if(TAG == 28) ssd1306_printFixed(0,  0, "Double Armor", STYLE_BOLD);
    if(TAG == 29) ssd1306_printFixed(0,  0, "Arcane Rune", STYLE_BOLD);
    if(TAG == 30) ssd1306_printFixed(0,  0, "Double health", STYLE_BOLD);
    if(TAG == 31) ssd1306_printFixed(0,  0, "Double Damage", STYLE_BOLD);
    if(TEAM == 0) ssd1306_printFixed(0,  32, "White Team", STYLE_BOLD);
    if(TEAM == 1) ssd1306_printFixed(0,  32, "Red Team", STYLE_BOLD);
    if(TEAM == 2) ssd1306_printFixed(0,  32, "Blue Team", STYLE_BOLD);
    if(TEAM == 3) ssd1306_printFixed(0,  32, "Green Team", STYLE_BOLD);
    if(TEAM == 4) ssd1306_printFixed(0,  32, "Orange Team", STYLE_BOLD);
    if(TEAM == 5) ssd1306_printFixed(0,  32, "Magenta Team", STYLE_BOLD);
    if(TEAM == 6) ssd1306_printFixed(0,  32, "Yellow Team", STYLE_BOLD);
    if(TEAM == 7) ssd1306_printFixed(0,  32, "Purple Team", STYLE_BOLD);
    ssd1306_printFixed(0,  48, String(TEAM).c_str(), STYLE_BOLD);
  }
  
  if( button_changed(digitalRead(button_pin), button_pin) == 1){
    //if the button was just pressed, send tag
    send_main_tag();
    //say you sent the tag
    ssd1306_normalMode();
    ssd1306_clearScreen( );
    ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
    ssd1306_printFixed(0,  0, "Sent Tag", STYLE_BOLD);
    ssd1306_printFixed(0,  16, String(TAG).c_str(), STYLE_BOLD);
    delay(1250);
    TAG = 42;    //set tag to something different, so the screen will update
  }
  OLD_TAG = TAG;
  OLD_TEAM = TEAM;
  //delay(1000);
}

/****
send_main_tag f(x)
the send main tag function uses the class that you are and
sends out a different tag based on what your class is
inputs: void (uses global variables based on characer class)
outputs: void (sends out tag via IR LED)
*/

void send_main_tag( void ){
  //first, turn on PWM
  Timer1.pwm(ir_led_pin, 512);       //send pulses

  //calculate tag to send

  if(TAG >= MIN_CHANGE_GAME_STATE_VALUE){
    //if you are changing game stats, bonus damage is 0 and team is 0
    setup_tag_array( TAG, 0, 0, SENT_ARRAY_TAG);       //you have to send 0 bonus damage. Otherwise it thinks it's an error
  }
  else{
    setup_tag_array( TAG, TEAM, 0, SENT_ARRAY_TAG);       //you have to send 0 bonus damage. Otherwise it thinks it's an error
  }
  
  
  mySerial.write(SENT_ARRAY_TAG, TAG_ARRAY_LEN);               //send a tag!

  //turn pwm off
  Timer1.disablePwm(ir_led_pin);
  
}

/*
button_changed
checks to see if the button was just pressed. 
If the button was just pressed, it outputs a 1
If the button wasn't just pressed, it outputs a 0
input: button_state (int), button to check
output: 1 or 0 (int)
*/

int button_changed( int current_state, int button_checked ){
  //I remember the state from last time
  static int tag_button_last_state;
  int last_state;               //last state button was in  
  
  last_state = tag_button_last_state;

  
  //tell arduino to remember last state the button was in
  
  int result = 0;               //default to not sending a tag
  
  /*
  if(Serial_debug_mode){
    Serial.println("Checking button states");
    Serial.print("last state: ");
    Serial.println(last_state);
    Serial.print("current state: ");
    Serial.println(current_state);
  }
  */
  
  if (current_state == HIGH){  //if the button is pressed
    if (last_state == LOW){    //and it wasn't pressed last time
      result = 1;              //send a tag!
      /*
      if(Serial_debug_mode){
       Serial.println("");
       Serial.println("send tag");
       Serial.println("");
      }
      */
    }
    else{                      //if it was pressed last time
      result = 0;              //don't send a tag
    }
  }
  else{                        //if the button isn't pressed
    result = 0;                //don't send a tag
  }
  
  //tell the arduino to remember what state the button is in now
  //the next time that it calls this function
  
  tag_button_last_state = current_state;
  
  return result;                //return result
}

/******
setup_tag_array f(x)
This function sets up the tag array that you input into it
inputs: Tag, Team, bonus Damage
outputs: updates the array with the correct information
 */
 void setup_tag_array( byte tag_to_send, byte team, byte bonus_damage, byte tag_array[]){
  tag_array[0] = (80 + team);   //add 01010000 to the team
  tag_array[1] = ((bonus_damage << 3) + (tag_to_send >> 2));
  tag_array[2] = ((tag_to_send & 0x03) << 6) + DEVICE_ID; 
  //after setting up the 3 bytes in the array, then you can calculate the checksum.
  tag_array[3] = calculate_checksum(tag_array);
 }

/******
calculate_checksum f(x)
This function calculates the checksum of a data packet of 2 bytes using one's complement
addition
inputs: array of two bytes
outputs: checksum value, with bonus damage on it
 */

 byte calculate_checksum( byte tag_array[] ){
  int i = tag_array[0] + tag_array[1] + tag_array[2]; //add together the bytes
  i = (i & 0xFF) + (i >> 8);             //add together any carryover
  return byte(i & 0xFF);                 //output the resulting byte
 }
