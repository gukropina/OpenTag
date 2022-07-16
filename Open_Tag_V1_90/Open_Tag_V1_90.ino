/* 
Open Tag, a DIY laser tag game

Software V 0.99_2 - added aim assist
Software V 0.99_3 - changed screen on death to stay on main screen and moved some strings to flash memory
                    Test that changing the global variable changes the game to display your class when you aren't hit.
                    Document how you changed telegraph_hit and check_passives to not display your class and show your status
Software V0.99_4 - changed to UART protocol for sending IR commands
Hardware V 0.99 - switched to button for changing class
Hardware V 0.99_4 - added PNP to pin 2 for UART and "I'm out" color change

Hardware V 1.50_1 - ATmega4809
Software V 1.50_1 - Changed libraries and code to allow the game to run on the ATmega4809

Software V 1.50_2 - Changed libraries and code to allow the game to run on the ATmega4809
Hardware V 1.50_2 - added new sound module, and the MP6050

Software V 1.75 - changed protocol to add bonus damage to tags
Hardware V 1.75 - changed ATmega4809 to include overclocks and equipment

Software V 1.76 - changed tag and ability button on ATmega4809 to use interrupts (can't on ATmega328p. Only pins 2 and 3. Bummer)
                - changed wireless payload for node.js communication
Hardware V 1.76 - changed ATmega4809 to 20 MHz

All of the variables you can change to change how the game is played are shown below.
After that, there are all the variables needed to run the game.
*/

/******DEBUGGING********/
#define SERIAL_DEBUG 0      //1 turns on serial debugging. 0 turns it off
//so, this only works when Serial_debug_mode is set to 0. oh wait. now it always fails.
#define TEST_MODE 0         //1 turns on sending test tags

/********STARTING DEVICE INFORMATION******/
//Device ID - 7 bits (0-127). 
//this is the id of the unit. It's who is sending the tag. it can be anything between 0 and 19
//to change this above 19, you need to change the "MIN_BASE_ID". Also, this can't be higher than "MAX_USED_DEVICE_ID"
//and MAX_RADIOS_USED, or the radios used to communicate could send at the same time and interfere with one another)
//id number 64 is reserved for setting other devices. Do not set DEVICE_ID to 64.
#define DEVICE_ID 23

#define CONTROLLER_DEVICE_ID 63       //do not change this unless you are drastically changing the code
                                      //this is the id of the controller, which controls the rest of the devices
#define MIN_BASE_ID 30                //lowest id used for a base. Any device id that is this number or higher
                                      //is assumed to be a base, and it functions differently than a player
#define MAX_USED_DEVICE_ID 40         //if you receive a tag from over this id, ignore it, as it isn't valid

//pick your processor
#define ATMEGA4809_TRUE 1       //set to 1 if using the ATmega4809 and 0 if using ATmega328p
#if ATMEGA4809_TRUE
  #include <Wire.h> 
  #include <SPI.h>
  #include <RF24.h>
  #include <RF24Network.h>

  //OLED
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  
  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  #define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  //RF24 radion stuff
  RF24 radio(PIN_PC2, PIN_PA7); // using pin 10 for the CE pin, and pin 7 for the CSN pin for ATmega4809
  RF24Network network(radio);          // Network uses that radio
  
  struct payload_t {                   // Structure of our payload
    byte command;
    byte who_was_hit;
    byte hit_by;
    long tag_received;
  };

  payload_t payload = { 0, 0, 0, 0 };

  byte NRF_DEBUG_MODE = 1;         //1 turns on sending anything seen by the IR sensor to the PC

  //create an instance of the DY Player for sounds
  #include <Arduino.h>
  #include "DYPlayerArduino.h"
  DY::Player player(&Serial2);

  //variables for MPU6050
  #define ACCEL_ARRAY_LEN 10     //10 items in the array for logging accellerometer and gyro data
  
  //this measures the the difference between the last reading and this reading
  #define RUN_ACCEL_THRESHOLD 8000      //decrease to increase sensitivity (determined empirically)
  #define WALK_ACCEL_THRESHOLD 5000     //decrease to increase sensitivity (determined empirically)
  #define MIN_WALKING_COUNT 3           //decrease to make it more sensitive to walking as juggernaut over time
  #define MIN_RUNNING_COUNT 5           //decrease to make it more sensitive to detecting running over time

  //generic constants
  #define MPU_ADDR 0x68                      // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
  #define MPU6050_TIME_BETWEEN_DATA   100   //100 ms between pulling data from the MPU

  //variables needed
  int X_ACCEL_ARRAY[ACCEL_ARRAY_LEN];          //create array with 10 items, indexed 0 to 9
  int Y_ACCEL_ARRAY[ACCEL_ARRAY_LEN];          //create array with 10 items, indexed 0 to 9
  int Z_ACCEL_ARRAY[ACCEL_ARRAY_LEN];          //create array with 10 items, indexed 0 to 9
  int X_GYRO_ARRAY[ACCEL_ARRAY_LEN];           //create array with 10 items, indexed 0 to 9
  int Y_GYRO_ARRAY[ACCEL_ARRAY_LEN];           //create array with 10 items, indexed 0 to 9
  int Z_GYRO_ARRAY[ACCEL_ARRAY_LEN];           //create array with 10 items, indexed 0 to 9
  byte ACCEL_ARRAY_INDEX = 0;  //start with array index at 0
  unsigned long MPU_DelayTimer;
  int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
  int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
  int16_t temperature; // variables for temperature data
  char tmp_str[7]; // temporary variable used in convert function
  
  char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
    sprintf(tmp_str, "%6d", i);
    return tmp_str;
  }

#else  //include libraries for ATmega328p
  #include <TimerOne.h>
  #include <SoftwareSerial.h>
  //OLED library
  #include "ssd1306.h"
  #include "nano_gfx.h"
  byte NRF_DEBUG_MODE = 0;         //1 turns on sending anything seen by the IR sensor to the PC
#endif

//set which team you are on
byte TEAM = 0;
//teams can't hurt each other, except white team (0), which is free for all
// 0 == white (free for all)
// 1 == red (can play capture the flag)
// 2 == blue 
// 3 == green (by default, if you are out from someone on green team, are now on green team)
// 4 == orange
// 5 == magenta
// 6 == yellow (by default, if you are out from someone on yellow team, are now on yellow team)
// 7 == purple (by default, if you are out from someone on purple team, are now on purple team)
#define NUMBER_OF_TEAMS 7    //Note: this is the max team number. To raise this, add more team bits to the sent tag

//class - this is the variable for what type of class you are
byte CHARACTER_CLASS = 2;           //start off as healer
/*
If you are using resistors to set the class, the values are shown below:
1 is normal - 5 V - read 1023
2 is healer - GND - read 0
3 is base - 2.5 V (10k and 10k)- read 551
4 is pyro - 3.33 V (5k and 10k), read 681
5 is ice - 1.667 V (10k and 5k) 341
6 is poison - 4.55V (1 k and 10k) read 930
7 is rock -  0.45 V(10k and 1k) read 93
8 is water - 4.58 V (2k and 10k) read 854
9 is lightning - 0.83 V (10k and 2k) read 170
10 is necromancer - 1.15 V (10k and 3k) read 236
11 is juggernaut - 3.85 V (3k and 10k)read 787
Note: as written, first resistor is connected to 5V, 2nd resistor is connected to GND
*/

#define NORMAL_TAG_NUM 1
#define HEAL_TAG_NUM 2
#define BASE_TAG_NUM 3
#define FIRE_TAG_NUM 4
#define ICE_TAG_NUM 5
#define POISON_TAG_NUM 6
#define ROCK_TAG_NUM 7
#define WATER_TAG_NUM 8
#define LIGHTNING_TAG_NUM 9

#define NUMBER_OF_CLASSES 11        //this is the number of classes there are

#define MIN_CHANGE_GAME_STATE_VALUE 16    //this is the minimum tag used to change state of the game
#define MIN_RUNE_VALUE 24                 //this is the minimum tag value that is considered a rune

/*******GAME MODE VARIABLES*******/
//can change classes when you respawn with one of your lives
#define LIVES_CAN_CHANGE_CLASS 0      //1 lets you change classes when you revive due to having lives. 0 doesn't

//are you playing in survival mode? if yes, anyone who is out spawns as a pyro on purple team. Pyro has different stats
byte SURVIVAL_MODE = 0;             //1 is to play in survival mode. 0 is to not play in survival mode
#define SURVIVAL_TEAM 7             //the team that everyone in survival mode changes to
#define AUTO_REVIVE_SURVIVAL 0      //1 is that when you play survival mode, you automatically turn on auto-reviving (see below)

//do you want to automatically respawn a short time after being tagged out?
byte AUTO_REVIVE = 0;                   //1 is to play that you automatically revive after a short time
#define AUTO_REVIVE_TIME_OUT 20000      //the team that everyone in survival mode changes to
unsigned long AUTO_REVIVE_TIME = 0;     //timer to track when you can come back to life


//do you want to have everyone take damage every minute? If so, how much?
#define DAMAGE_EVERY_MINUTE 0            //1 is to set damage every minute. 0 is off.
#define DMG_EVRY_MINUTE_DELAY 60000      //time between damage instances
#define DMG_EVRY_MINUTE_TAG 1            //the tag you are hit with every minute

//the change class rune changes your team. Set this to be the team you want them to be on
#define CHANGE_CLASS_RUNE_TEAM 0     //set this to the team you want someone who gets the change class rune
                                     //to change their team to. By default, they go to free for all

//this makes you change class to juggernaut when you kill the juggernaut
#define CHANGE_TO_JUG_WHEN_KILL_JUG 1     //set this to 1 to make you change to juggernaut when you kill them. 0 to not

//You can specify a team that is the "zombie" team. If someone from the "zombie" team tags you out,
//you switch sides to that team (note, there can be up to four "zombie" teams
//set the team to 8 to not use it, since there is no team 8)
#define ZOMBIE_TEAM_1 3             //zombie team 1, by default, is green (set to 3)
#define ZOMBIE_TEAM_2 6             //zombie team 2, by default, is yellow (set to 6)
#define ZOMBIE_TEAM_3 7             //zombie team 3, by default, is purple (set to 7)
#define ZOMBIE_TEAM_4 8             //zombie team 4, by default, is not used (set to 8)
byte ZOMBIE_GOT_ME = 0;             //0 if a Zombie tagged me out. otherwise, it's the zombie's team that got me out

//should someone who is out from the zombie team come back at full health (1), or need to be revived (0) 
//when they are tagged out?
#define REVIVE_ZOMBIE_WHEN_OUT 0   //set auto-reviving when out zombie team (1 is on, 0 is off)

//can zombies, if they revive when they are out, change their class?
#define ZOMBIE_REVIVES_CHANGE_CLASS 1  //1 is on

//do you want healers to heal automatically every few seconds or not?
#define HEALER_PASSIVE_HEALING 1     //set to 1 to have healers heal passively, 0 for them not to

//do you want pyro to get extra health every time they are tagged out (for a minute or so) or not?
#define PYRO_PASSIVE_EXTRA_HEALTH 1  //set to 1 to have pyro get extra health after they are out. 0 for nope

//do you want rock to have get more armor every few seconds or not?
#define ROCK_PASSIVE_ARMOR 1         //set to 1 to have rock class gain armor passively. 0 for them not to

//do you want to be able to "tag" a base that sends runes to keep it from sending for 2 seconds?
#define CONTEST_RUNES 0              //set to 1 to make bases that send runes to not send for a few seconds if tagged

//do you want to use "aim assist"?
byte ENABLE_AIM_ASSIST = 0;         //set to 1 to use aim assist, 0 to not use it
#define AUTOMATIC_AIM_ASSIST 0      //set to 1 to have aim assist always be on. Set to 0 to have to hold the tag button for aim assist

//King of the hill: if you are a base, you are capturable and you count up to victory
byte KING_OF_THE_HILL = 0;           //sets if you are playing king of the hill. 1 to play, 0 to not play
int RED_TEAM_VICTORY_POINTS = 0;     //count points for red
int BLUE_TEAM_VICTORY_POINTS = 0;    //count points for blue
int ORANGE_TEAM_VICTORY_POINTS = 0;  //count points for orange
int MAGENTA_TEAM_VICTORY_POINTS = 0; //count points for blue
#define POINTS_TO_WIN 180            //number of points to win (max is 32,000)
#define POINT_GAIN_DELAY 1000        //get 1 point every second

//do you want to display your class continuously?
#define SHOW_CLASS_CONSTANTLY 0      //set to 1 to display class constantly

//bonus damage
byte BONUS_DAMAGE = 0;              //amount of bonus damage (tracked for poison)
#define MAX_BONUS_DAMAGE 30         //max amount of bonus damage (5 bit number)

/*******VARIABLES for selectting classes*******/
//class voltages are shown below in what analog read will read
//NOTE: class voltages are used for both teams and classes
//NOTE: Every volt is 1023/5 or 204.6 units in analog read
//int class_voltages[] = {1023, 0, 511, 681, 341, 930, 93, 854, 170, 236, 787};

//allowable voltage error on analog read
#define voltage_error 10

//tag delay is: received_tag_sound (total) - tag_sound (total) + 10
#define tag_delay 100     //delay between tags for soldier
#define extra_delay 10    //a little extra delay between sounds

/**********PINS*************/
// Defining pins (don't change these unless changing hardware)
#define ZERO_POINT_NINE_EIGHT 1     //if the hardware version is above 0.98 (1 for yes, 0 for no)
//if you are using hardware version 0.98 or above, use new pinout 

#if ZERO_POINT_NINE_EIGHT

#if ATMEGA4809_TRUE                  //pins for ATmega4809
  /*
  #define hit_led_pin 14
  #define tag_button_pin PIN_PD3
  #define ability_button_pin PIN_PD4
  #define ir_led_pin PIN_PF4              //TCB0 timer output
  #define RGB_Red 11
  #define DISTANCE_SELECT 21     //for IR on device
  #define IR_SELECT PIN_PC3      //select between short beam angle IR LED on device and the wide angle ones
  #define piezo_pin PIN_PE3
  #define RGB_Blue 12
  #define RGB_Green 13
  #define SET_PIN 10                        //programming pin for radios
  #define CLASS_PIN 19
  #define TEAM_PIN A6
  #define HIT_ENEMY_DEVICE_PIN 16
  #define HIT_ENEMY_ARMOR_PIN 15
  */

  #define hit_led_pin PIN_PC5
  #define tag_button_pin PIN_PD2
  #define ability_button_pin PIN_PD3
  #define ir_led_pin PIN_PF4              //TCB0 timer output
  #define RGB_Red PIN_PE0
  #define DISTANCE_SELECT PIN_PC4     //for IR on device
  #define IR_SELECT PIN_PC3      //select between short beam angle IR LED on device and the wide angle ones
  #define piezo_pin PIN_PD4
  #define RGB_Blue PIN_PE3
  #define RGB_Green PIN_PE1
  #define SET_PIN 10                        //programming pin for radios
  #define CLASS_PIN PIN_PD5
  #define TEAM_PIN PIN_PD6
  #define HIT_ENEMY_DEVICE_PIN PIN_PD1
  #define HIT_ENEMY_ARMOR_PIN PIN_PD2

  #define mySerial Serial1                 //IR signals are sent through serial 1

#else //ATmega328p pins
  #define hit_led_pin 13
  #define tag_button_pin 12
  #define ability_button_pin 11
  #define ir_led_pin 10
  #define RGB_Red 9
  #define DISTANCE_SELECT 8
  #define piezo_pin 7
  #define RGB_Blue 6
  #define RGB_Green 5
  #define SET_PIN 4                        //programming pin for radios
  #define TX_TAG 2
  #define RX_TAG 3
  //pins 0 and 1 are used for serial communication
  #define CLASS_PIN A0
  #define TEAM_PIN A1
  #define HIT_ENEMY_DEVICE_PIN A2
  #define HIT_ENEMY_ARMOR_PIN A3
  //pins A4 and A5 are used for I2C communication
  
  //define software serial pins
  SoftwareSerial mySerial(RX_TAG, TX_TAG); // RX, TX

#endif

#define TAG_ARRAY_LEN 4         //tags have 4 bytes
#define TAG 1                   //this is the basic tag, sending a 1
#define AIM_ASSIST_TAG 23       //aim assist tag is 23
#define AIM_ASSIST_AUTOMATIC 24 //aim assist automatically working is 24
//define tag sent
byte SENT_ARRAY_TAG[] = {0, 0, 0, 0};
//define ability sent
byte SENT_ARRAY_ABILITY[] = {0, 0, 0, 0};
byte SENT_ARRAY_AIM_ASSIST[] = {0, 0, 0, 0};

#else
#define hit_led_pin 13
const byte tag_button_pin = 12;
const byte ability_button_pin = 11;
const byte ir_led_pin = 10;
const byte ir_receiver_pin_1 = 9;     //this is RGB Red in V0.98
//int ir_receiver_pin_2 = 8;          //this is distance select in V0.98
const byte ir_receiver_pin_3 = 7;     //this is piezo in V0.98

const byte RGB_Red = 6;               //this is blue in V0.98
const byte RGB_Blue = 5;              //this is green in V0.98
#define SET_PIN 4                     //programming pin for radios
const byte RGB_Green = 3;             //this is for receiving serial input for IR communication in V0.98

const byte piezo_pin = 2;             //this is sending IR serial in V0.98
//pins 1 and 0 are used for serial


#define CLASS_PIN A0
#define TEAM_PIN A1
#define HIT_ENEMY_DEVICE_PIN A2
#define HIT_ENEMY_ARMOR_PIN A3
//pins A4 and A5 are used for I2C
#endif

#if ATMEGA4809_TRUE                  //interrupts for ATmega4809
byte TAG_BUTTON_PRESSED = 0;         //tag button pressed
byte ABILTY_BUTTON_PRESSED = 0;      //ability button pressed

#endif

#define TELEGRAPH_ENEMY_HIT 750        //ms to telegraph that you tagged another person
#define TELEGRAPH_AIM_ASSIST_HIT 750   //ms to telegraph that you are aiming at someone

//Default health and mana regeneration (note, this is just defining the global variables. To change
//this for individual classes, see the defined variables above)
int HEALTH = 120;                        //if anything can go negative, make it an int, not a byte, or it will roll over
int MAX_HEALTH = 150;                    //max health of 150
int MANA = 100;                          //mana can go negative, so keep it as an int
int MAX_MANA = 100;                      //max Mana of 100
unsigned long MANA_TIME = 0;             //when to add mana
int MANA_DELAY = 1000;                   //how long between adding mana in ms (don't go above 32,000 (for int))
#define DEFAULT_MANA_DELAY 1000          //normally respawn mana every second
#define NECRO_MANA_DELAY 5000            //necromancer gets 1 mana every 5 seconds
#define JUG_HIT_MANA_DELAY 6000          //if you are the juggernaut, do not regenerate mana after being tagged
byte MANA_REGEN = 1;                     //regenerate 1 mana per regen period
byte MANA_COST = 1;                      //default mana cost is 1
int AMMO = 200;                          //how much ammunition you have
byte USE_AMMO = 0;                       //if this is set to 0, don't worry about ammo. If over 0, us ammo
int MAX_AMMO = 200;                      //start off with 200 max ammo
#define AMMO_MANA_MULTIPLIER 4           //if you are using ammo, start off with x3 normal mana (and have max mana of x3 normal)
unsigned long PASSIVE_ABILITY_TIME = 0;  //when to do passive ability
long PASSIVE_ABILITY_COOLDOWN = 5000;    //default passive ability cooldown
unsigned long AIM_ASSIST_TIME = 6000;    //when to send an aim assist tag (don't send in the first 6 seconds)
#define AIM_ASSIST_COOLDOWN 500          //default aim assist cooldown
#define AIM_ASSIST_TAG_COOLDOWN 3000     //turn off aim assist for 3000 ms after sending a tag
#define RESPAWN_TIME 3000                //it takes 3 seconds to respawn (note: startup_sound also plays for 5 seconds
                                         //if you change this, change the startup_sound_length array so the total adds to
                                         //this (in milliseconds)
#define RESPAWN_CHANGE_CLASS_TIME 8000   //milliseconds to change your class after spawning
unsigned long CHANGE_CLASS_TIME = 10000; //how long you have to change classes, which should be about the same as
                                         //RESPAWN_TIME above (getting back into the game)
unsigned long DAMAGE_EVERY_MINUTE_TIME = DMG_EVRY_MINUTE_DELAY; //to deal damage
#define CHANGE_CLASS_MINIMUM_TIME 2000    //if you are changing classes, you can keep changing classes for this number of
                                          //milliseconds after hitting the change class button
#define OUT_COLOR_CHANGE_TIME 150         //flash each color for this amount of time when you are tagged out


//define damage for all abilities (note, damage is used for health and against armor)
#define NORMAL_DAMAGE 10               //damage of a regular tag is 10
#define DOUBLE_DAMAGE 20               //double damage is 20 (damage dealt if you have double damage rune)
#define TRIPLE_DAMAGE 30               //damage for the soldier "special" tag is 30
#define HEALING_DAMAGE 12              //healing heals 12 health per hit
#define REVIVE_DAMAGE 20               //revive does 20 damage to someone on the wrong team
#define REVIVE_POISON_DAMAGE 40        //if you're poisoned, a revive does 40 damage (if hitting the wrong team)
#define FIRE_DAMAGE 8                  //fire damage (doubled to health, does this to armor)
#define ICE_DAMAGE 7                   //ice damage (after it reaching maximum stacks or to armor)
#define ICE_WATER_DAMAGE 15            //ice damage (after reaching maximum stacks while target is wet)
#define ICE_EXPLOSION_MULTIPLIER 20    //extra damage per stack of ice when hit with fire (this is doubled again by the fire)
#define POISON_DAMAGE 13               //poison damage per stack. (does damage x stacks per hit to armor)
#define POISON_PER_HIT 2               //poison stacks per hit
#define LIGHTNING_DAMAGE 15            //lightning does 15 damage at max stacks
#define LIGHTNING_WATER_DAMAGE 20      //lightning does 20 damage if they are wet (and automatically goes to full stacks)
#define LIGHTNING_ARMOR_DAMAGE 15      //lightning does 15 damage to armor
#define ROCK_ARMOR_DAMAGE 10           //rock gives 10 armor per hit
#define WATER_ARMOR_DAMAGE 25          //water does 25 damage to rock armor
#define JUG_MANA_REGEN 10              //Juggernaut regains mana per second with ability
#define JUG_ARMOR_REGEN 5              //Juggernaut regains armor per second with ability
#define NORMAL_COOLDOWN 333            //you can normally send tags every 333 ms
#define JUG_ABILITY_COOLDOWN 600       //how long, in ms, juggernaut takes between gaining mana & armor
#define MEDIC_PASSIVE_COOLDOWN 12000   //how long, in ms, healer takes to heal itself
#define WIND_DAMAGE 20                 //damage per stack of wind
#define WIND__ARMOR_DAMAGE 10          //damage to armor from wind
#define WIND_MOVE_TIME 12000           //time in ms you have to move before wind does damage to you
#define WIND_MOVE_THRESHOLD 7          //the amount of movement you need to do before clearing wind
#define WIND_RUNNING_MOVEMENT 2        //running counts for twice as much as walking for the wind threshold
#define WIND_WALKING_MOVEMENT 1        //running counts for twice as much as walking for the wind threshold
#define MIN_RESPAWN_HEALTH_PERCENT 20  //at minimum, you respawn with at least 20% health (necromancers without bonuses spawn at minimum)
                                       //bases and controller respawns at maximum. 1 = 1%. 5=5%, etc. Can't go over 100. 
//ability variables. These are all the variables that impact how abilities work
#define SOLDIER_MAX_HP_DAMAGE_BONUS 10 //10=100% damage bonus for soldier at max hp (1 = 10% increase. 10 = 100%) can go up to 10
#define ICE_DELAY 5000                 //ice stack lasts for 5 seconds each
#define MAX_ICE_STACKS 3               //max ice stacks
#define MAX_ICE_WATER_STACKS 5         //max ice stacks if someone has water on them
#define WATER_DELAY 12000              //water lasts for 12 seconds
#define POISON_DELAY 1500              //poison loses 1 stack per 1.5 seconds
#define MAX_POISON_STACKS 25           //poison can have maximum of 25 stacks
#define MAX_LIGHTNING_STACKS 5         //max of 5 lightning stacks
#define LIGHTNING_DELAY 3000           //lightning clears in 3 seconds per stack
#define LIGHTNING_MULTIPLIER 1         //lightning removes 1 mana per stack
#define ROCK_MANA 2                    //getting hit with rock adds 2 mana
#define MAX_ROCK_EXTRA_MANA 10         //you can have 10 extra max mana from rock armor
#define ROCK_DISPLAY_TIME 2000         //show rock if you are hit by rock for 2 seconds
#define NORMAL_MAX_ROCK_STACKS 50      //you normally start with a max of 50 armor
#define HIGHER_MAX_ROCK_STACKS 100     //certain classes (rock and juggernaut) have more max armor
#define DOUBLE_ARMOR_MAX_ROCK_STACKS 300  //you get 300 armor if you get the double armor rune
#define SOLDIER_MAX_HEALTH 250
#define SOLDIER_SART_HEALTH 200
#define NORMAL_MAX_HEALTH 200
#define NORMAL_START_HEALTH 150
#define FRAGILE_MAX_HEALTH 150
#define FRAGILE_START_HEALTH 100
#define JUG_MAX_HEALTH 100
#define JUG_START_HEALTH 50
#define FIRE_MANA_COST 10              //fire ability mana cost
#define FIRE_STARTING_HEALTH 100       //fire starting health
#define FIRE_MAX_HEALTH 120            //fire starting max health
#define FIRE_HEALTH_PER_STACK 20       //how much health fire gets each time they are out
#define FIRE_MAX_HEALTH_PER_STACK 30   //how much max health fire gets each time they are out
#define FIRE_DELAY 10000               //how long fire lasts on someone
#define MAX_FIRE_DEATH_STACKS 5        //max of 5 death stacks
#define FIRE_DEATH_DELAY 45000         //how long death stacks last

//define all of the other variables for the fire class
//survival mode pyro stats
#define SURVIVE_FIRE_STARTING_HEALTH 10        //fire starting health
#define SURVIVE_FIRE_MAX_HEALTH 30             //fire starting max health
#define SURVIVE_FIRE_HEALTH_PER_STACK 20       //how much health fire gets each time they are out
#define SURVIVE_FIRE_MAX_HEALTH_PER_STACK 30   //how much max health fire gets each time they are out
#define SURVIVE_MAX_FIRE_DEATH_STACKS 15       //max of 15 death stacks
#define SURVIVE_FIRE_DEATH_DELAY 120000        //how long death stacks last




//Passives. these are all of the flags and timers on what passive you have
byte FIRE = 0;
unsigned long FIRE_TIME = 0;
byte FIRE_DEATH_STACKS = 0;            //number of times fire class has been tagged out within X time
byte ICE_STACKS = 0;
unsigned long ICE_TIME = 0;
byte WATER = 0;
unsigned long WATER_TIME = 0; 
byte POISON_STACKS = 0;
byte POISON_DEVICE = 0;                //the device that hit you with poison
unsigned long POISON_TIME = 0;
byte LIGHTNING_STACKS = 0;
unsigned long LIGHTNING_TIME = 0;
int ROCK_STACKS = 0;                   //to ensure it doesn't go negative when subtracting
byte MAX_ROCK_STACKS = 50;             //can have 50 max armor, but this can change
unsigned long WIND_TIME = 0;           //tracking how long you have to move
byte WIND_TOTAL_MOVEMENT = 0;          //tracking the total amount of movement you've had while hit with wind
byte WIND = 0;                         //whether or not you are hit with wind

//runes are power ups that you can get from bases
byte DOUBLE_DAMAGE_RUNE = 0;      //tracks how many tags you can do double damage with
byte RUNE_SENT = 0;               //what rune to send. Set to 0 to not send runes
byte CAN_SEND_CHANGE_CLASS = 1;   //how many times this device can send the change class rune
byte DOUBLE_HEALTH_RUNE = 0;      //whether or not you have the double health rune (if not 0)
byte ARCANE_RUNE = 0;             //whether or not you have the double mana rune (if not 0)
byte DOUBLE_ARMOR_RUNE= 0;        //whether or not you have the double armor rune (if not 0)

#define DOUBLE_DAMAGE_RUNE_ATTACKS 50       //when you get a double damage rune, you get 50 attacks at double damage
#define DOUBLE_DAMAGE_DAMAGE_BONUS 10       //100% damage bonus for double mana rune 
#define ARCANE_RUNE_ATTACKS_HIGH 90         //when you get a double damage rune, you get 90 attacks (lightning)
#define ARCANE_RUNE_ATTACKS_NORMAL 60       //when you get a double damage rune, you get 60 attacks (all others)
#define ARCANE_RUNE_ATTACKS_LOW 15          //when you get a double damage rune, you get 15 attacks (water and necromancer)
#define ARCANE_RUNE_DAMAGE_BONUS 5          //50% damage bonus for double mana rune 

//if you have lives left, you respawn automatically when you are out and lose 1 life
byte LIVES = 0;

//rune information
#define RUNE_COOLDOWN 25000            //runes come back every 25 seconds
#define MAX_RUNE_ID 32                 //maximum tag value that is a rune
#define MIN_RUNE_ID 25                 //minimum tag value that is a rune
#define CHANGE_CLASS_RUNE 25           //rune used to change class
#define CLASS_CHANGED_TO 11            //when hit with the change class rune, you change to this class

//should a base that sends a run switch or stay the same?
#define RUNE_CYCLES_NORMALLY 0         //rune cycles in a pattern through all runes (1 to activate, 0 to deactivate)
#define RUNE_CYCLES_RANDOMLY 0         //rune cycles randomly through all runes (1 to activate, 0 to deactivate)
#define RUNE_CYCLES_THROUGH_ARRAY 0    //rune cycles randomly through an array of runes (1 to activate, 0 to deactivate)
#if RUNE_CYCLES_THROUGH_ARRAY
byte RUNE_ARRAY[] = {27, 28, 29, 30, 31};   //array of runes to pick randomly from, if you cycle through array
#endif
//defined runes: 25 - undefined (can be rune later) 26 - ammo refill
//               27 - extra life, 28 - double armor, 29 - double mana
//               30 - double health, 31 - double damage, 32 - change class

//should you automatically get mana back?
byte AUTO_RESPAWN_MANA = 1;            //0 is to not regain mana automatically. anything greater than 0 respawns mana

//tag cooldown variables (note: these are just defined globally here. To change it for each class
//change the respawne function)
//NOTE: If you make the cooldown less than 250, the other device doesn't have enough time to
//get tagged, update the display, and be ready to receive the next tag, so keep this above 250.
//above 350 to be safe. If you ever want this to be below 250, you will need to use interrupts
//to monitor the IR Receiver pins, so you can interrupd updating the display
int tag_cooldown = 350;
long ability_cooldown = 350;
unsigned long cooldown_time = 0;

//time in ms that your status LED's blink when you are tagged
//make this less than tag cooldown, so you can see it blinking
//if you are tagging someone
#define HIT_OFF_TIME 80
#define HIT_DISPLAY_TIME 220
#define HIT_BASE_COOLDOWN_TIME 2000         //when tagged, bases can't tag for 2 seconds

int TELEGRAPHING_HIT = -1;   //start out without being hit

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
const int PROTOCOL_START_ERROR = 350; //350 ms error with start code
const int PROTOCOL_ERROR = 250;       //250 ms error with all other codes
//tags can be within prococol_error of correct value and still count

/* defined tags: (all defined)
0 - double damage
1 - normal tag
2 - heal
3 - revive
4 - fire
5 - ice
6 - poison
7 - rock armor
8 - water
9 - lightning
10 - used to take damage for poison
11 - triple damage
25 - turn on king of the hill game mode (if you are a base)
26 - turn on change class rune mode (if you are a base)
27 - extra life rune
28 - double armor rune
29 - double mana rune
30 - double health rune
31 - double damage rune
32 - never sent, but used as the change class rune internally in the code
*/
#define NORMAL_TAG_NUM 1
#define HEAL_TAG_NUM 2
#define BASE_TAG_NUM 3
#define FIRE_TAG_NUM 4
#define ICE_TAG_NUM 5
#define POISON_TAG_NUM 6
#define ROCK_TAG_NUM 7
#define WATER_TAG_NUM 8
#define LIGHTNING_TAG_NUM 9

/*********RADIO VARIABLES**********/
#define RADIO_TIME_PER_DEVICE 4             //each device has a 4 ms window to transmit
#define RADIO_SILENCE_TIME 1                //each device has 1 ms of silence between transmit times
#define WIRELESS_SEND_BUFFER 25             //can store 24 bytes (6 messages) for sending out the radio
#define WIRELESS_MESSAGE_LENGTH 5           //number of bytes in radio messages
#define MAX_RADIOS_USED 38                  //maximum number of radios used. allows device_id to be 0-37,
                                            //and each id will have a different "time" to send radio commands)
//time in microseconds for all radios to send their instructions, before they loop and can send again
//RADIO_PERIOD = MAX_RADIOS_USED*(RADIO_TIME_PER_DEVICE + RADIO_SILENCE_TIME);

//definition of radio bytes sent:
//first byte: who the transmission is sent to
//second byte: what command is sent
//third byte: value - what value to set things to
byte RADIO_IN_SYNC_MODE = 0;                //used to synchronize radio time
#define RADIO_TIMEOUT_DELAY 6000            //leave sync mode if you haven't received anything (ms)
#define RADIO_SYNC_DELAY 2000               //send second sync signal after this time (ms)
unsigned long RADIO_TIMEOUT_TIME = 0;       //when to leave syncing radios
byte SEND_RADIO_SYNC_ONCE = 0;              //send sync transmission once (used by base)

#if ATMEGA4809_TRUE
  char AMBIENT_ELECTRICITY_S[] = "/ae01.MP3";
  char AMBIENT_FIRE_S[] = "/af01.MP3";
  char AMBIENT_POISON_S[] = "/ap01.MP3";
  char AMBIENT_WATER_S[] = "/aw01.MP3";
  char HIT_ELECTRICITY_S[] = "/he01.MP3";
  char HIT_FIRE_S[] = "/tf01.MP3";        //I like the tag fire sound better, so am using that here
  char HIT_HEAL_S[] = "/hh01.MP3";
  char HIT_ICE_S[] = "/hi01.MP3";
  char HIT_WATER_S[] = "/hw01.MP3";
  char HIT_ROCK_S[] = "/oo02.MP3";        //not sure how much I like this one, but fine for now
  char HIT_NORMAL_S[] = "/oa05.MP3";
  char CAN_NOT_TAG_S[] = "/oa03.MP3";
  char CAN_NOT_TAG_GOOSE_S[] = "/oa08.MP3";
  char OUT_S[] = "/oa09.MP3";
  char HIT_ENEMY_S[] = "/og01.MP3";
  char EXTRA_LIFE_S[] = "/og02.MP3";
  char GOT_RUNE_S[] = "/og04.MP3";
  char FULL_HEALTH_S[] = "/og07.MP3";
  char NO_SHIELD_S[] = "/oj02.MP3";
  char SHIELD_RECHARGE_S[] = "/oj01.MP3";
  char JUGGERNAUT_APPROACH_S[] = "/oj03.MP3";
  char JUGGERNAUT_HERE_S[] = "/oj04.MP3";
  char YOU_SHOULD_BE_DEAD_S[] = "/oo03.MP3";
  char TAG_ELECTRICITY_S[] = "/te01.MP3";
  char TAG_FIRE_S[] = "/tf01.MP3";
  char TAG_ICE_S[] = "/ti01.MP3";
  char TAG_NORMAL_S[] = "/tn03.MP3";
  char TAG_DOUBLE_DAMAGE_S[] = "/tn09.MP3";
  char TAG_HEAL_S[] = "/hh01.MP3";          //i like the hit sound for heal. using it for tags too
  char TAG_ROCK_S[] = "/hh01.MP3";
  char TAG_NECROMANCER_S[] = "/hh01.MP3";
  char TAG_POISON_S[] = "/tn08.MP3";
  char TAG_WATER_S[] = "/tn06.MP3";

  char NEXT_SOUND_ARRAY[10];              //global array for playing a sound

  #define GOOSE_SOUND_NUM 33
  #define SHIELD_RECHARGE_SOUND_NUM 34
  #define SHIELD_OUT_SOUND_NUM 35
  #define OUT_SOUND_NUM 36
  #define HIT_SOMEONE_SOUND_NUM 37
  #define JUGGERNAUT_HERE_SOUND_NUM 38
  
  #define HIT_EVENT 1
  #define TAG_EVENT 2
  #define OTHER_EVENT 3
  
#else  //include libraries for ATmega328p

#endif

//sound arrays
int current_sound_length = 8;
//current sound lenght MUST be at least as long as the longest sound you have
int current_sound[8] = {};
int current_sound_delay[8] = {};

int tag_sound[] = {4000, 4500, 5000, 4000, 3000, 2000};
int tag_sound_delay[] = {10, 10, 10, 10, 10, 10};
int tag_sound_length = 6;

int cant_tag_sound[] = {220, 110};
int cant_tag_sound_delay[] = {25, 75};
int cant_tag_sound_length = 2;

int received_tag_sound[] = {1175, 300, 1175, 300, 1175, 300};
int received_tag_sound_delay[] = {25, 25, 25, 25, 25, 25};
int received_tag_sound_length = 6;

int out_sound[] = {5000, 4000, 3000, 2000, 1000, 100};
int out_sound_delay[] = {500, 500, 500, 500, 500, 500};
int out_sound_length = 6;

int ability_sound[] = {5000, 3500, 2000, 3500, 5000, 4000, 3000, 2000};
int ability_sound_delay[] = {10, 10, 20, 10, 10, 10, 10, 10};
int ability_sound_length = 8;

int received_ability_sound[] = {2000, 3000, 4000};
int received_ability_delay[] = {25, 25, 25};
int received_ability_length = 3;

int heal_ability_sound[] = {5000, 3500, 2000, 3500, 5000, 4000, 3000, 2000};
int heal_ability_sound_delay[] = {10, 10, 10, 10, 10, 10, 10, 10};
int heal_ability_sound_length = 8;

int fire_ability_sound[] = {4250, 4500, 4750, 4250, 4500, 4750};
int fire_ability_sound_delay[] = {50, 50, 50, 50, 50, 50};
int fire_ability_sound_length = 6;

int startup_sound[] = {100, 1000, 2000, 3000, 4000, 5000};
int startup_sound_delay[] = {500, 500, 500, 500, 500, 500, 500};
int startup_sound_length = 6;

/*******IMAGES FOR SCREEN**********/
#define IMAGE_NUM_PIXELS_X 26        //there are 26 pixels in the X direction for all images
#if ATMEGA4809_TRUE
//using a different library, with different images for ATmega4809
/*
 * These icons are bmp files. first, find a black and white png that you want to use.
 * then convert it to the right pixel size. I use 26x36 for icons that are on/off
 * and I use 26x24 for icons with numbers under them (which stack)
 * you can convert a png to size here: https://onlinepngtools.com/resize-png
 * then, change the image into a bite array (cpp) for an OLED display
 * I use https://diyusthad.com/image2cpp
 */
// 'fire', 26x32px
const unsigned char fire_image [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 
  0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 
  0x00, 0x70, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x78, 0x30, 0x00, 0x00, 0x7c, 0x30, 0x00, 
  0x00, 0x7c, 0x70, 0x00, 0x03, 0x7e, 0x70, 0x00, 0x03, 0x3f, 0x70, 0x00, 0x03, 0x3f, 0xf0, 0x00, 
  0x03, 0x3f, 0xf0, 0x00, 0x03, 0xff, 0xf8, 0x00, 0x07, 0xfb, 0xf8, 0x00, 0x07, 0xf9, 0xfc, 0x00, 
  0x0f, 0xf9, 0xfc, 0x00, 0x0f, 0xf9, 0xfc, 0x00, 0x0f, 0x91, 0xfc, 0x00, 0x0f, 0x82, 0xfc, 0x00, 
  0x0f, 0x82, 0xfc, 0x00, 0x0f, 0x80, 0x7c, 0x00, 0x0f, 0x00, 0x78, 0x00, 0x07, 0x00, 0x38, 0x00, 
  0x03, 0x80, 0x30, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'snowflake', 26x24px
const unsigned char snowflake_image [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4c, 0x80, 0x00, 
  0x00, 0x7f, 0x80, 0x00, 0x02, 0x1e, 0x10, 0x00, 0x12, 0x0c, 0x32, 0x00, 0x3f, 0x0c, 0x3e, 0x00, 
  0x0f, 0x0c, 0x3c, 0x00, 0x07, 0xcc, 0xf8, 0x00, 0x18, 0xed, 0xc6, 0x00, 0x00, 0x3f, 0x00, 0x00, 
  0x00, 0x1e, 0x00, 0x00, 0x18, 0xff, 0x86, 0x00, 0x0f, 0xcc, 0xfc, 0x00, 0x07, 0x0c, 0x38, 0x00, 
  0x1f, 0x0c, 0x3e, 0x00, 0x12, 0x0c, 0x32, 0x00, 0x02, 0x1e, 0x10, 0x00, 0x00, 0x7f, 0x00, 0x00, 
  0x00, 0x4c, 0x80, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'waterdrop', 26x32px
const unsigned char water_image [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 
  0x00, 0x0e, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 
  0x00, 0x7f, 0x80, 0x00, 0x00, 0x7f, 0xc0, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x01, 0xff, 0xe0, 0x00, 
  0x01, 0xff, 0xf0, 0x00, 0x03, 0xff, 0xf0, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 
  0x0f, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xfe, 0x00, 
  0x1f, 0xff, 0xfe, 0x00, 0x1f, 0xff, 0xfe, 0x00, 0x1d, 0xff, 0xfe, 0x00, 0x0c, 0xff, 0xfe, 0x00, 
  0x0c, 0xff, 0xfc, 0x00, 0x0e, 0x7f, 0xfc, 0x00, 0x06, 0x3f, 0xf8, 0x00, 0x07, 0x87, 0xf8, 0x00, 
  0x03, 0xff, 0xf0, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'poison', 26x24px
const unsigned char poison_image [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 
  0x00, 0x7f, 0x80, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0xff, 0xc0, 0x00, 
  0x00, 0xff, 0xe0, 0x00, 0x0c, 0xff, 0xcc, 0x00, 0x0e, 0x8c, 0x5c, 0x00, 0x1e, 0x8c, 0x5e, 0x00, 
  0x3e, 0x9e, 0x5f, 0x00, 0x3f, 0xf3, 0xff, 0x00, 0x18, 0xff, 0xc2, 0x00, 0x00, 0x7f, 0x80, 0x00, 
  0x00, 0xff, 0xc0, 0x00, 0x1b, 0xc0, 0xf6, 0x00, 0x3f, 0xc0, 0xff, 0x00, 0x1f, 0x00, 0x3e, 0x00, 
  0x0e, 0x00, 0x1c, 0x00, 0x0e, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'lightning', 26x24px
const unsigned char lightning_image [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x07, 0x80, 0x00, 
  0x00, 0x0f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x01, 0xfe, 0x00, 0x00, 
  0x00, 0xff, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x1f, 0xe0, 0x00, 0x00, 0x1f, 0xc0, 0x00, 
  0x00, 0x1f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 
  0x00, 0xc0, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'rockaromr', 26x24px
const unsigned char rock_image [] PROGMEM = {
  0x00, 0x0c, 0x00, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0xff, 0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 
  0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0, 
  0x7f, 0xff, 0xff, 0x80, 0x7f, 0xff, 0xff, 0x80, 0x3f, 0xff, 0xff, 0x00, 0x3f, 0xff, 0xff, 0x00, 
  0x1f, 0xff, 0xfe, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x0f, 0xff, 0xfc, 0x00, 0x07, 0xff, 0xf0, 0x00, 
  0x01, 0xff, 0xe0, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00 };

 // 'star', 26x24px
  const unsigned char double_damage_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x01, 0xcc, 0x40, 0x00, 
  0x00, 0xff, 0xc0, 0x00, 0x00, 0x5e, 0x80, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x07, 0xff, 0xf8, 0x00, 
  0x07, 0xff, 0xf8, 0x00, 0x00, 0x7f, 0x80, 0x00, 0x00, 0x5e, 0x80, 0x00, 0x00, 0xff, 0xc0, 0x00, 
  0x01, 0x8c, 0x60, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
    };

    // 'wind', 26x24px
  const unsigned char wind_image [] PROGMEM = {
    0x00, 0xc0, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x01, 0x20, 0x00, 0x00, 0x63, 0x30, 0x00, 0x00, 
    0xf2, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 
    0xc1, 0xff, 0xff, 0xc0, 0x7c, 0xff, 0xff, 0xc0, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 
    0x03, 0xff, 0xff, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x09, 0x1f, 0xfe, 0x00, 
    0x0b, 0x9f, 0xfc, 0x00, 0x0a, 0xf0, 0x00, 0x00, 0x18, 0x50, 0x00, 0x00, 0x08, 0x58, 0x00, 0x00, 
    0x08, 0x40, 0x00, 0x00, 0x0c, 0xc0, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 
    };

    // 'lives', 26x24px (looks like a heart)
  const unsigned char lives_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0xf0, 0x00, 0x07, 0xe1, 0xf8, 0x00, 
    0x07, 0xf3, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 
    0x07, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xf8, 0x00, 0x03, 0xff, 0xf0, 0x00, 
    0x01, 0xff, 0xe0, 0x00, 0x01, 0xff, 0xe0, 0x00, 0x00, 0xff, 0xc0, 0x00, 0x00, 0x7f, 0x80, 0x00, 
    0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 
    0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
    };

    
    // 'mana', 26x24px
  const unsigned char mana_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 
    0x00, 0x5d, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 
    0x00, 0x7f, 0xc0, 0x00, 0x01, 0xf7, 0xe0, 0x00, 0x02, 0x36, 0x30, 0x00, 0x04, 0x1e, 0x18, 0x00, 
    0x07, 0x9c, 0x68, 0x00, 0x07, 0x9c, 0x74, 0x00, 0x13, 0x3e, 0x70, 0x00, 0x21, 0xe1, 0xe2, 0x00, 
    0x3f, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
    };

  // 'health', 26x24px
  const unsigned char health_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x7e, 0x00, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 
    0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x0f, 0xff, 0xf0, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
    };

  // 'ammo', 26x24px
    const unsigned char ammo_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 
  0x00, 0xff, 0xc0, 0x00, 0x01, 0xcc, 0xe0, 0x00, 0x03, 0x0c, 0x30, 0x00, 0x06, 0x0c, 0x18, 0x00, 
  0x04, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x7f, 0x80, 0x3f, 0x80, 
  0x7f, 0x80, 0x7f, 0x80, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x0c, 0x00, 
  0x06, 0x0c, 0x18, 0x00, 0x03, 0x0c, 0x30, 0x00, 0x01, 0xcc, 0xe0, 0x00, 0x00, 0x7f, 0xc0, 0x00, 
  0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

  // 'arcane rune', 26x24px
    const unsigned char arcane_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 
  0x00, 0x36, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 
  0x00, 0xe1, 0xc0, 0x00, 0x01, 0xc1, 0xe0, 0x00, 0x02, 0x41, 0x20, 0x00, 0x02, 0x61, 0x30, 0x00, 
  0x04, 0x7f, 0x90, 0x00, 0x07, 0xe3, 0xf0, 0x00, 0x07, 0x22, 0x30, 0x00, 0x0c, 0x36, 0x18, 0x00, 
  0x0c, 0x1c, 0x1c, 0x00, 0x12, 0x1c, 0x34, 0x00, 0x13, 0x3e, 0x26, 0x00, 0x33, 0xe3, 0xe2, 0x00, 
  0x3f, 0xe1, 0xfe, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

  /*
   * bullet symbol
  const unsigned char ammo_image [] PROGMEM = {
    0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x0c, 0x07, 0xc0, 
    0x00, 0x1c, 0x07, 0xc0, 0x00, 0x78, 0x07, 0xc0, 0x00, 0xf0, 0x07, 0xc0, 0x01, 0xf2, 0x07, 0xc0, 
    0x03, 0xee, 0x07, 0xc0, 0x07, 0xfc, 0x07, 0xc0, 0x07, 0xf8, 0x07, 0xc0, 0x07, 0xf8, 0x07, 0xc0, 
    0x01, 0xf3, 0x07, 0xc0, 0x01, 0xe7, 0x07, 0xc0, 0x03, 0xfe, 0x07, 0xc0, 0x07, 0xfc, 0x07, 0xc0, 
    0x03, 0xfc, 0x07, 0xc0, 0x00, 0xf8, 0x07, 0xc0, 0x01, 0xf0, 0x07, 0xc0, 0x03, 0xe0, 0x07, 0xc0, 
    0x03, 0xc0, 0x07, 0xc0, 0x01, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x07, 0xc0
    };

    */

    

#else 
  /*
   * thes icons are xbmp files. 
   */
  // 'fire', 26x32px
  const unsigned char fire_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 
    0x00, 0x18, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 
    0x00, 0x1E, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 
    0x00, 0x1E, 0x08, 0x00, 0x00, 0x3E, 0x08, 0x00, 0x00, 0x3E, 0x0C, 0x00, 
    0x40, 0x7E, 0x0E, 0x00, 0xC0, 0x7E, 0x0E, 0x00, 0xC0, 0xFE, 0x0E, 0x00, 
    0xC0, 0xFD, 0x0F, 0x00, 0xC0, 0xFD, 0x1F, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 
    0xE0, 0xFF, 0x3F, 0x00, 0xF0, 0x9F, 0x3F, 0x00, 0xF0, 0x9F, 0x3F, 0x00, 
    0xF0, 0x9F, 0x3F, 0x00, 0xF0, 0x8B, 0x3F, 0x00, 0xF8, 0x43, 0x7F, 0x00, 
    0xF0, 0x81, 0x3E, 0x00, 0xF0, 0x01, 0x3E, 0x00, 0xF0, 0x00, 0x3E, 0x00, 
    0xE0, 0x01, 0x1C, 0x00, 0xC0, 0x01, 0x0C, 0x00, 0x00, 0x03, 0x02, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
  // 'snowflake', 26x24px
  const unsigned char snowflake_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 
    0x00, 0x32, 0x01, 0x00, 0x00, 0xFE, 0x01, 0x00, 0xC0, 0x78, 0x0C, 0x00, 
    0x48, 0x30, 0x4C, 0x00, 0xFC, 0x30, 0xFC, 0x00, 0xF0, 0x30, 0x3C, 0x00, 
    0xF0, 0x33, 0x3F, 0x00, 0x18, 0xFF, 0x63, 0x00, 0x00, 0x7C, 0x00, 0x00, 
    0x00, 0xF8, 0x00, 0x00, 0x18, 0xFF, 0x63, 0x00, 0xF8, 0x33, 0x3F, 0x00, 
    0xF0, 0x30, 0x1E, 0x00, 0xFC, 0x30, 0x7C, 0x00, 0xC8, 0x30, 0x4C, 0x00, 
    0x40, 0x78, 0x0C, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x32, 0x01, 0x00, 
    0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };
  // 'waterdrop', 26x32px
  const unsigned char water_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 
    0x00, 0x30, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 
    0x00, 0xFC, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0xFE, 0x01, 0x00, 
    0x00, 0xFE, 0x01, 0x00, 0x00, 0xFF, 0x03, 0x00, 0x00, 0xFF, 0x03, 0x00, 
    0x80, 0xFF, 0x07, 0x00, 0x80, 0xFF, 0x07, 0x00, 0xC0, 0xFF, 0x0F, 0x00, 
    0xC0, 0xFF, 0x0F, 0x00, 0xC0, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 
    0xE0, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 
    0xF0, 0xFF, 0x3F, 0x00, 0x30, 0xFF, 0x3F, 0x00, 0x20, 0xFF, 0x1F, 0x00, 
    0x60, 0xFF, 0x1F, 0x00, 0x60, 0xFE, 0x1F, 0x00, 0xE0, 0xFC, 0x1F, 0x00, 
    0xC0, 0xF0, 0x0F, 0x00, 0x80, 0xE3, 0x07, 0x00, 0x80, 0xFF, 0x07, 0x00, 
    0x00, 0xFE, 0x01, 0x00, 0x00, 0x78, 0x00, 0x00, };
    
  // 'poison', 26x24px
  const unsigned char poison_image [] PROGMEM = {
    0x00, 0xFC, 0x00, 0x00, 0x00, 0xFF, 0x03, 0x00, 0x80, 0xFF, 0x07, 0x00, 
    0x80, 0xFF, 0x07, 0x00, 0x80, 0xFF, 0x07, 0x00, 0xC0, 0xFF, 0x0F, 0x00, 
    0x80, 0xFF, 0x0F, 0x00, 0xDC, 0xFF, 0xE7, 0x00, 0x9E, 0x70, 0xEC, 0x00, 
    0x9C, 0x30, 0xE4, 0x00, 0xBE, 0x30, 0xF4, 0x01, 0xFF, 0x5A, 0xFC, 0x03, 
    0xDF, 0xCF, 0xEF, 0x03, 0x86, 0xEF, 0x87, 0x01, 0x00, 0xFF, 0x03, 0x00, 
    0x00, 0xFE, 0x01, 0x00, 0x80, 0xFD, 0x06, 0x00, 0xE7, 0x01, 0x9E, 0x03, 
    0xFF, 0x03, 0xFF, 0x03, 0xFF, 0x00, 0xFC, 0x03, 0x3E, 0x00, 0xF0, 0x01, 
    0x1C, 0x00, 0xE0, 0x00, 0x1C, 0x00, 0xE0, 0x00, 0x0C, 0x00, 0xC0, 0x00, 
    };
    
  // 'lightning', 26x24px
  const unsigned char lightning_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 
    0x00, 0xC0, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 
    0x00, 0x7F, 0x00, 0x00, 0x80, 0x7F, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 
    0x00, 0xFE, 0x03, 0x00, 0x00, 0xF0, 0x07, 0x00, 0x00, 0xF8, 0x03, 0x00, 
    0x00, 0xFC, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 
    0x00, 0x03, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };

  // 'star', 26x32px
  const unsigned char double_damage_image [] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 
    0x80, 0x30, 0x04, 0x00, 0x00, 0x33, 0x03, 0x00, 0x00, 0xFF, 0x03, 0x00, 
    0x00, 0x7A, 0x01, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0xFF, 0x03, 0x00, 
    0xE0, 0xFF, 0x1F, 0x00, 0xE0, 0xFF, 0x1F, 0x00, 0x00, 0xFF, 0x03, 0x00, 
    0x00, 0xFC, 0x00, 0x00, 0x00, 0x7A, 0x01, 0x00, 0x00, 0xFF, 0x03, 0x00, 
    0x00, 0x33, 0x03, 0x00, 0x80, 0x30, 0x04, 0x00, 0x00, 0x30, 0x00, 0x00, 
    0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };

    // 'wind', 26x24px
  const unsigned char wind_image [] PROGMEM = {
    0x00, 0x03, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 0x80, 0x07, 0x00, 0x00, 
    0x80, 0x04, 0x00, 0x00, 0xC6, 0x0C, 0x00, 0x00, 0x4E, 0x0C, 0x00, 0x00, 
    0x4F, 0x00, 0x00, 0x00, 0xC9, 0x00, 0x00, 0x00, 0xC9, 0x00, 0x00, 0x00, 
    0x81, 0x00, 0x00, 0x00, 0x81, 0x01, 0x00, 0x00, 0x83, 0xFF, 0xFF, 0x03, 
    0x3E, 0xFF, 0xFF, 0x03, 0x3E, 0xF8, 0xA9, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x80, 0xFF, 0xFF, 0x00, 0xC0, 0xFF, 0xFF, 0x01, 0xE0, 0xFF, 0xFF, 0x00, 
    0x20, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x10, 0xF8, 0x7F, 0x00, 
    0xD0, 0xF9, 0x7F, 0x00, 0xD0, 0x0D, 0x00, 0x00, 0x50, 0x0F, 0x00, 0x00, 
    0x18, 0x0A, 0x00, 0x00, 0x10, 0x1A, 0x00, 0x00, 0x10, 0x1A, 0x00, 0x00, 
    0x10, 0x02, 0x00, 0x00, 0x30, 0x03, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00, 
    0xE0, 0x01, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 
    };

#endif
  

//Strings for OLED

const char string_0[] PROGMEM = "White Team"; // "String 0" etc are strings to store - change to suit.
const char string_1[] PROGMEM = "Red Team";
const char string_2[] PROGMEM = "Blue Team";
const char string_3[] PROGMEM = "Green Team";
const char string_4[] PROGMEM = "Orange Team";
const char string_5[] PROGMEM = "Magenta Team";
const char string_6[] PROGMEM = "Yellow Team";
const char string_7[] PROGMEM = "Purple Team";
const char string_8[] PROGMEM = "Soldier";
const char string_9[] PROGMEM = "Medic";
const char string_10[] PROGMEM = "Base";
const char string_11[] PROGMEM = "Pyro";
const char string_12[] PROGMEM = "Ice";
const char string_13[] PROGMEM = "Poison";
const char string_14[] PROGMEM = "Rock";
const char string_15[] PROGMEM = "Water";
const char string_16[] PROGMEM = "Lightning";
const char string_17[] PROGMEM = "Necromancer";
const char string_18[] PROGMEM = "Juggernaut";

// Then set up a table to refer to your strings.

const char *const string_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5, string_6, string_7,
                                            string_8, string_9, string_10, string_11, string_12, string_13, string_14, string_15,
                                            string_16, string_17, string_18};

char BUFFER[12];  // make sure this is large enough for the largest string it must hold (see strings above)


//displaying statuses using the RGB LED
byte DISPLAY_CHECK = 0;                  //check if you need to display status
unsigned long DISPLAY_STATUS_TIME = 0;   //how long between displaying statuses
const int DISPLAY_CLASS_DELAY = 2500;    //how long you display your class
const int DISPLAY_STATUS_DELAY = 750;    //how long you display statuses

//Mj csy vieppc aerxih xs, csy gsyph xyvr xli evhymrs mrxs e gpsgo.
//Nywx wecmrk.

void setup() {
  Serial.begin(9600);             //turn on Serial port for radio
  
  /*if(SERIAL_DEBUG){
    //if we are debugging, say hello
    Serial.println(F("Hello World!"));
  }
  */
  
  mySerial.begin(2400);       //turn on serial for receiving IR commands
  
  //now, set up the radio
  #if ATMEGA4809_TRUE
    radio.begin();
    radio.setChannel(0);
    network.begin( node_lookup(DEVICE_ID));   //begin the network with this device's id
  #else  //include libraries for ATmega328p
    pinMode(SET_PIN, OUTPUT);       //set radio set pin to output
    digitalWrite(SET_PIN, LOW);     //start programming
    delay(50);                      //delay for HC-12 to go into programming mode
    Serial.print(F("AT+DEFAULT"));  //set HC-12 to default mode
    delay(50);
    digitalWrite(SET_PIN, HIGH);     //end programming mode
    delay(200);
    if(Serial.available()) {     //The HC-12 has response data (the AT Command response)
        if(SERIAL_DEBUG){
          //if we are debugging, write what you see
          Serial.println(Serial.read());
        }
        else{
          Serial.read();               //Read the data to clear it
        }
      }
    wireless_transceiver(0, -2, 0, 0);  //set the time for sending radio commands
    
  #endif

  wireless_transceiver(0, 5, DEVICE_ID, 0);  //send message that you started the game
  
  //new OLED code vs old one
  #if ATMEGA4809_TRUE
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      //note: most people use 8 bit addresses, while arduino uses 7 bits. to convert, drop the least significant bit
      //so, 0x7A becomes 0x3C, and 0x78 becomes 0x3D. 
      if(SERIAL_DEBUG)Serial.println(F("OLED failed"));
    for(;;); // Don't proceed, loop forever
    }
    else{
      if(SERIAL_DEBUG)Serial.println(F("OLED succeeded"));
    }

    //initialize with all images 
    display.clearDisplay();
    display.setTextSize(1);             // 2 to 1 pixel scale
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
    display.drawBitmap(0, 0, health_image, 26, 24, 1);
    display.setCursor(3, 24);
    display.println(HEALTH);
    display.drawBitmap(26, 0, mana_image, 26, 24, 1);
    display.setCursor(30, 24);
    display.println(MANA);
    display.drawBitmap(52, 0, ammo_image, 26, 24, 1);
    display.setCursor(57, 24);
    display.println(AMMO);
    display.drawBitmap(0, 32, fire_image, 26, 32, 1);
    display.drawBitmap(26, 32, snowflake_image, 26, 24, 1);
    display.drawBitmap(52, 32, water_image, 26, 32, 1);
    display.drawBitmap(78, 32, poison_image, 26, 24, 1);
    display.drawBitmap(104, 32, lightning_image, 26, 24, 1);
    display.drawBitmap(104, 0, rock_image, 26, 24, 1);
    display.setCursor(104,24);
    display.print(50);                                   //armor
    display.display();
    delay(500);
  #else  //include libraries for ATmega328p
    ssd1306_128x64_i2c_init();
  #endif
  
  //initialize TimerOne to pulse 38,000 times a second

  #if ATMEGA4809_TRUE
    // For the ATmega4809, we are setting up the timer registers manually
    // this code is adapted from: https://tomalmy.com/infrared-barrier-modulating-a-38khz-carrier-at-1khz-with-an-arduino-nano-every/
    // We use TCBO to generate the output. Note that you can change the duty cycle
    // by varying TCB0.CCMPH. It is just the proportion of the period, 
    // set in TCB0.CCMPL.
    TCB0.CTRLA = 0; // Turn off channel for configuring
    TCB0.CTRLB = 0x17; // output enabled, PWM mode
    TCB0.CCMPL = 8000000L/38000L - 1; // Set period as close to 
                                       // giving 38kHz as we can achieve (but, still using clock speed of 16 HHz divided by 2?)
    TCB0.CCMPH = (8000000L/38000L)/2; // 50% duty cycle 
    TCB0.CTRLA = 3; // Enable and use system clock/2 (now at 20 MHz, but doesn't need to change)
    //code to play sounds
    player.begin();
    player.setVolume(20); // 60% Volume
    player.playSpecifiedDevicePath(DY::Device::Sd, CAN_NOT_TAG_GOOSE_S);
  #else  //include libraries for ATmega328p
    Timer1.initialize(26);
  #endif

  //add in an axel / gyro (MPU) if we are on the new hardware
  #if ATMEGA4809_TRUE
    Wire.begin();
    Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
    Wire.write(0x6B); // PWR_MGMT_1 register
    Wire.write(0); // set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);
  #endif
    
  //tell the arduino if a pin is input/output
  pinMode(HIT_ENEMY_DEVICE_PIN, OUTPUT); 
  pinMode(HIT_ENEMY_ARMOR_PIN, OUTPUT); 
  pinMode(hit_led_pin, OUTPUT);
  #if ZERO_POINT_NINE_EIGHT                //if the hardware version is above 0.98
  pinMode(tag_button_pin, INPUT_PULLUP);
  pinMode(ability_button_pin, INPUT_PULLUP); 
  pinMode(DISTANCE_SELECT, OUTPUT);
  pinMode(CLASS_PIN, INPUT_PULLUP);
  #else
  pinMode(tag_button_pin, INPUT);
  pinMode(ability_button_pin, INPUT); 
  #endif
  pinMode(ir_led_pin, OUTPUT);
  
  #if ATMEGA4809_TRUE                  //add interrupts for button presses on ATmega4809
  pinMode(IR_SELECT, OUTPUT);
  //if the tag or ability button is pressed, run the interrupt for it (Falling, because it's active low)
  attachInterrupt(tag_button_pin, tag_pin_ISR, FALLING);
  attachInterrupt(ability_button_pin, ability_pin_ISR, FALLING); 
  #endif
  
  //add pinMode for RGB LED
  pinMode(RGB_Red, OUTPUT);
  pinMode(RGB_Green, OUTPUT);
  pinMode(RGB_Blue, OUTPUT);
  pinMode(piezo_pin, OUTPUT);



  //add pullup resistor to team pin if you need it
  //pinMode(TEAM_PIN, INPUT_PULLUP);
  
  //common cathode RGB LED
  //start with RGB LED off
  analogWrite(RGB_Red, 0);       //0 is off
  analogWrite(RGB_Green, 0);   //255 is on
  analogWrite(RGB_Blue, 0); 
  
  
  //initialize sound function
  play_sound( cant_tag_sound, cant_tag_sound_delay, 1);
  delay(10);
  play_sound( cant_tag_sound, cant_tag_sound_delay, 0);

  //calculate character class
  respawn(0, 0, 100, 0);                   //respawn using current class and team and 100% health

  
}

void loop() {

  //check if you received wireless information
    receive_wireless();
  
  #if ATMEGA4809_TRUE
    //wireless_transceiver sends data immediately with the radio network
  #else  //include functions for ATmega328p
    //check if you can send wireless information for hc-12 network
    wireless_transceiver(0, -1, 0, 0);
  #endif
  

  if(RADIO_IN_SYNC_MODE == 0){
    //if you are not trying to sync your radio, check other things
    
    //check if we are sending a tag
    check_if_tagging( tag_button_pin );
    check_if_tagging( ability_button_pin );
    
    //check if we are getting tagged
    check_if_tagged();
    //check_if_tagged(ir_receiver_pin_2);
    
    //check if you should stop telegraphing a hit
    telegraph_hit(-1);
    
    //check what sound to play
    play_sound( current_sound, current_sound_delay, 0);
    #if ATMEGA4809_TRUE
      //if you are using that sound module, check those sounds too
      play_mp3_sound(0, 0);
    #endif
    
    //check passives
    check_passives(0, 0);
  
    //check if you should stop indicating you tagged someone
    telegraph_tags(0);

    //check if you can change your class
    change_class(0);  
  }
  else{
    //if you are in radio sync mode, check to see if you've timed out
    if(RADIO_TIMEOUT_TIME < millis()){
      //if you've timed out, exit radio sync mode
      RADIO_IN_SYNC_MODE = 0;
      if(SEND_RADIO_SYNC_ONCE > 0){
        //if you are supposed to send the next sync, send it!
        SEND_RADIO_SYNC_ONCE = 0;
        //send a global message to everyone to sync their radios
        wireless_transceiver(CONTROLLER_DEVICE_ID, 6, DEVICE_ID, 0);
        if(SERIAL_DEBUG){
          Serial.println(F("Radio Sync"));
        }
      }
      else{
        if(SERIAL_DEBUG){
          Serial.println(F("Timeout Radio sync"));
        }
      }
      
    }
  }

  //check for accelerometer data
  #if ATMEGA4809_TRUE
    //if you are not hit with wind, or aren't juggernaut, ignore this and move on
    if(WIND != 0 || CHARACTER_CLASS == 11){
      //if we are using the ATmega4809, check for data from the MPU every 100 ms. This runs the PrintAllValues function (defined at top of everything)
      if ((millis() - MPU_DelayTimer) >= (MPU6050_TIME_BETWEEN_DATA)) {     // delay between pulling data
        read_mpu();                                                         // call function to read data
      }
    }
  #endif
  
  /*
  if(SERIAL_DEBUG){
    Serial.println("Loop Da Loop!");
    delay(500);
  }
  */
  
}

/*
check_if_tagging f(x)
Takes in a pint that a button is attached to.
If the button is pressed, check if you can send a tag
if you can send a tag, send one and reset cooldown.
Input: button_pin (int)
Output: nothing (void)
*/

void check_if_tagging( int button_to_check ){
  byte had_double_damage = 0;
  if(DOUBLE_DAMAGE_RUNE > 0) had_double_damage = 1;
  
  //read button and see if it was just pressed
  int button_state = digitalRead( button_to_check );
  //see if button is pressed
  int button_pressed = button_changed( button_state, button_to_check );

  #if ATMEGA4809_TRUE                  //include interrupts for button presses on ATmega4809
  if( button_to_check == tag_button_pin && TAG_BUTTON_PRESSED == 1){
    button_pressed = 1;                //send a tag!
    TAG_BUTTON_PRESSED = 0;            //reset interrupt flag
  }
  else if( button_to_check == ability_button_pin && ABILTY_BUTTON_PRESSED == 1){
    button_pressed = 1;                //send a tag!
    ABILTY_BUTTON_PRESSED = 0;         //reset interrupt flag
  }
  
  #endif
  
  //debugging
  /*
  Serial.print(F("checking button: "));
  Serial.print(button_to_check);
  Serial.print(F("Button State: "));
  Serial.println(button_state);
  */
  #if ZERO_POINT_NINE_EIGHT                //if the hardware version is above 0.98
  //if pyro or lightning or juggernaut, can hold ability button down
  if((CHARACTER_CLASS == 4 || CHARACTER_CLASS == 9 || CHARACTER_CLASS == 11) && button_state == LOW && 
      button_to_check == ability_button_pin){
    button_pressed = 1;
  }
  #else
  //if pyro or lightning or juggernaut, can hold ability button down
  if((CHARACTER_CLASS == 4 || CHARACTER_CLASS == 9 || CHARACTER_CLASS == 11) && button_state == HIGH && 
      button_to_check == ability_button_pin){
    button_pressed = 1;
  }
  #endif
  
  //check if you can send a tag
  if( button_pressed == 1){
    if(HEALTH > 0 &&
       millis() > cooldown_time){
         //if(Serial_debug_mode) Serial.println("got to here!");
         //if you can send a tag, pick which tag to send based on the button
         if( button_to_check == tag_button_pin){
           if((USE_AMMO > 0 && AMMO > 0) || USE_AMMO == 0){
            //if you are using ammo and have it, or if you aren't using ammo, then send tag
              send_main_tag();       //send normal tag
              
              //send tag sound
              play_sound(tag_sound, tag_sound_delay, tag_sound_length);                  
              
              //set cooldown
              cooldown_time = millis() + tag_cooldown + (tag_cooldown*ICE_STACKS);

              //update ammo
              if(USE_AMMO > 0)  AMMO = AMMO - 1;  //update ammo
  
              //if you had double damage, or used ammo update the display
              if(had_double_damage == 1 || USE_AMMO > 0) update_display(0, 0);
              
           }
           else{
            //if you can't send a tag, play the "out" sound
            play_sound(cant_tag_sound, cant_tag_sound_delay, 
                  cant_tag_sound_length);
           }
             
         }
         else if (button_to_check == ability_button_pin){
          //if I have mana and am not wet, I can send abilities (juggernaut can use ability with 0 mana)
          if ((MANA > 0 || CHARACTER_CLASS == 11) && WATER == 0) {
            //first, reset cooldown
            if(RUNE_SENT > 0){
              //if you are a rune base, add more cooldown
              cooldown_time = millis() + RUNE_COOLDOWN;
              //and telegraph this
              telegraph_hit(RUNE_SENT);
            }
            else{
              //otherwise, use normal cooldown
              cooldown_time = millis() + ability_cooldown;
            }
            
            //send an ability!
            send_ability();
            
            //send ability sound
            play_sound(ability_sound, ability_sound_delay, ability_sound_length);                   
            
            //deduct mana
            if(ARCANE_RUNE > 0){
              //if you have an arcane rune, you use the rune charge instead of mana
              ARCANE_RUNE = ARCANE_RUNE - 1;
            }
            else{
              //otherwise, use mana
              MANA = MANA - MANA_COST;

              //if this means you are out of mana, set next mana to 3 times normal delay
              if(MANA < 1){
                MANA = 0;
                MANA_TIME = millis() + 3*MANA_DELAY;
              }
              else{
                //otherwise, do a normal delay in getting mana back
                MANA_TIME = millis() + MANA_DELAY;
              }
            }
              
            //update display
            update_display(0, 0);
          }
          else{
            
            play_sound(cant_tag_sound, cant_tag_sound_delay, 
                  cant_tag_sound_length);
          }
          /*
           * if you make this an else if, you can check if you have mana. if not, can't send tag
           * You will need some way of storing mana usage. Could be a case statement here at the end
           * to take out mana abased on class and a defined variable up top 
           */
            
         }
         //no matter which button is pressed, don't send an aim assist tag for a bit
          AIM_ASSIST_TIME = millis() + AIM_ASSIST_TAG_COOLDOWN;
       }
   else{
    if(button_to_check == ability_button_pin){
      if(CHARACTER_CLASS != 4 && CHARACTER_CLASS != 9){
        //if you aren't pyro or lightning, send can't send ability (they  can hold button down)
        play_sound(cant_tag_sound, cant_tag_sound_delay, 
                  cant_tag_sound_length); 
      }
    }
    else{
      //if you aren't checking the ability pin, send can't tag sound
      //(only ability pin has special rules for holding down the button)
      if(HEALTH > 0){
        //if you have health, make the normal sound
        play_sound(cant_tag_sound, cant_tag_sound_delay, 
                cant_tag_sound_length); 
      }
      else{
        //if you do not have health, and you are trying to tag, Goose does not approve
        #if ATMEGA4809_TRUE
        play_mp3_sound(OTHER_EVENT, GOOSE_SOUND_NUM);
        #endif
      }
      
    }
   }
  }
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
  #if ATMEGA4809_TRUE
    //don't need PWM on for ATmega4809, as it's always on)
    if(TEAM == 7){
      digitalWrite(IR_SELECT, LOW);   //select IR without optics for zombies on purple team
    }
    else{
      digitalWrite(IR_SELECT, HIGH);   //select IR with optics
    }
    
  #else  //include libraries for ATmega328p
    Timer1.pwm(ir_led_pin, 512);       //send pulses
  #endif
  
       
  #if ZERO_POINT_NINE_EIGHT              //hardware version 0.98 or above
  if(CHARACTER_CLASS == 4){
    //pyro's have short distance tags, unless they are on zombie team
    digitalWrite(DISTANCE_SELECT, LOW);   //select short distance on IR LED with optics
  }
  else{
    //everone else has long distance tags
    digitalWrite(DISTANCE_SELECT, HIGH);   //select long distance on IR LED with optics
  }

  //if you are a zombie, do something different
  if(TEAM == 7){
      //digitalWrite(DISTANCE_SELECT, HIGH);   //select IR without optics on device
      digitalWrite(DISTANCE_SELECT, LOW);   //select IR without optics on the headband/hat
    }
  #endif

  byte bonus_damage_total = 0;          //start with 0 bonus damage
  byte normal_damage_total = 1;         //start doing 1 damage

  //Set what bonus damage to send based on what class I am
  if(CHARACTER_CLASS == 1 && HEALTH == MAX_HEALTH){
    //soldier has special ability to do double damage if at max health
    bonus_damage_total = bonus_damage_total + SOLDIER_MAX_HP_DAMAGE_BONUS;     //does 100% bonus damage. Each increase of 1 = 10% increase
  }
  
  if(DOUBLE_DAMAGE_RUNE > 0){
    //do double damage
    bonus_damage_total = bonus_damage_total + 10;
    DOUBLE_DAMAGE_RUNE = DOUBLE_DAMAGE_RUNE - 1;    //subtract one double damage
  }
  
  //make sure i didn't add too much bonus damage
  if(bonus_damage_total > MAX_BONUS_DAMAGE) bonus_damage_total = MAX_BONUS_DAMAGE;
  
  setup_tag_array( NORMAL_TAG_NUM, TEAM, bonus_damage_total, SENT_ARRAY_TAG);
  
  mySerial.write(SENT_ARRAY_TAG, TAG_ARRAY_LEN);               //send a tag!

  if(SERIAL_DEBUG){
    if (normal_damage_total == 1){
      Serial.println(F("Sent normal tag"));
    }
    else if(normal_damage_total == 2){
      Serial.println(F("Sent double damage tag"));
    }
    else{
      Serial.println(F("Sent triple damage tag"));
    }
  }
  
  //turn off PWM
  #if ATMEGA4809_TRUE
    //don't need PWM on for ATmega4809, as it's always on)
  #else  //include libraries for ATmega328p
    Timer1.disablePwm(ir_led_pin);
  #endif
  
}


/********
send_ability f(x)
the send ability function uses the class that you are and
sends out a different tag based on what class you are
inputs: void (uses global variable character class)
outputs: void (sends out tag via IR LED)
*/

void send_ability( void ){

  //first, turn on PWM
  #if ATMEGA4809_TRUE
    //don't need PWM on for ATmega4809, as it's always on)
    if(TEAM == 7){
      digitalWrite(IR_SELECT, LOW);   //select IR without optics for zombies on purple team
    }
    else{
      digitalWrite(IR_SELECT, HIGH);   //select IR with optics for other teams
    }
  #else  //include libraries for ATmega328p
    Timer1.pwm(ir_led_pin, 512);       //send pulses
  #endif
  
  
  //for each class, send out a different ability
  int tag_to_send = 1;
  #if ZERO_POINT_NINE_EIGHT              //hardware version 0.98 or above
  if(CHARACTER_CLASS == 4 || CHARACTER_CLASS == 10){
    //pyro's and necromancers have short distance tags, unless they are on zombie team
    digitalWrite(DISTANCE_SELECT, LOW);   //select short distance on IR LED with optics
  }
  else{
    //everone else has long distance tags
    digitalWrite(DISTANCE_SELECT, HIGH);   //select long distance on IR LED with optics
  }

  //however, if you are a zombie, set the IR LED based on the ones without optics
  if(TEAM == 7){
      digitalWrite(DISTANCE_SELECT, HIGH);   //select IR without optics on device
      //digitalWrite(DISTANCE_SELECT, LOW);   //select IR without optics on the headband/hat
    }
  #endif



  if(CHARACTER_CLASS != 3 && CHARACTER_CLASS != 11){
    //if I'm not a base or juggernaut, calculate and, send the tag!
    if(ARCANE_RUNE > 0){
      //I have an arcane rune, do bonuses
      setup_tag_array( calculate_ability_to_send(CHARACTER_CLASS), TEAM, ARCANE_RUNE_DAMAGE_BONUS, SENT_ARRAY_ABILITY);
    }
    else{
      //I don't have an arcane rune. No bonuses
      setup_tag_array( calculate_ability_to_send(CHARACTER_CLASS), TEAM, 0, SENT_ARRAY_ABILITY);
    }
    
    mySerial.write(SENT_ARRAY_ABILITY, TAG_ARRAY_LEN);               //send a tag!
    if(SERIAL_DEBUG) Serial.println(F("Sent Ability"));
  }
  else if(CHARACTER_CLASS == 11){
    //juggernaut doesn't send ability. Regains mana and armor
      MANA = MANA + JUG_MANA_REGEN;
      if(MANA > MAX_MANA) MANA = MAX_MANA;
      ROCK_STACKS = ROCK_STACKS + JUG_ARMOR_REGEN;
      if(ROCK_STACKS > MAX_ROCK_STACKS) ROCK_STACKS = MAX_ROCK_STACKS;
      telegraph_hit(3);   //show you are being healed
      if(SERIAL_DEBUG) Serial.println(F("Juggernaut restore shield"));
  }

  if(CHARACTER_CLASS == 1){
    //soldier's special ability also lights themselves on fire
    take_damage(4, 0, 0, 0, 0);
  }

  //turn off PWM
  #if ATMEGA4809_TRUE
    //don't need PWM on for ATmega4809, as it's always on)
  #else  //include libraries for ATmega328p
    Timer1.disablePwm(ir_led_pin);
  #endif

}

/*
send_tag f(x)
sends a tag of the specified length in microseconds
inputs: tag length (microseconds, int)
outputs: pulsed IR LED (void)
const int PROTOCOL_START_CODE = 1800; //start code is 1800 ms
const int PROTOCOL_0 = 600;           //0 is 600 ms
const int PROTOCOL_1 = 1200;          //1 is 1200 ms
const int PROTOCOL_DELAY = 600;       //600 ms delay between pulses
const int PROTOCOL_START_ERROR = 350; //350 ms error with start code
const int PROTOCOL_ERROR = 250;       //250 ms error with all other codes

void send_tag( int tag_to_send, byte team ){
  int device_id = DEVICE_ID;            //create local device id to use
  //if(team == 8) team = 0;
  if(tag_to_send == CHANGE_CLASS_RUNE){
    //if you are changing classes, send different things
    device_id = CONTROLLER_DEVICE_ID;
    tag_to_send = CLASS_CHANGED_TO;
    team = CHANGE_CLASS_RUNE_TEAM;
  }
  int tag_bits = tag_to_send*8 + team;
  int tag_bits_array[PROTOCOL_BITS];  //create array of tag bits
  int id_bits_array[PROTOCOL_BITS];   //create array of device id bits
  int bitmask = 128;                   //start by masking 0000 0000 1000 0000

  
  if(SERIAL_DEBUG){
      Serial.print(F("Tag to send: "));
      Serial.println(tag_to_send);
      Serial.print(F("Team to send: "));
      Serial.println(team);
      Serial.print(F("Device ID to send: "));
      Serial.println(device_id);
  }
  
  
  for( int i = 0; i < PROTOCOL_BITS ; i++ ){
    if( (tag_bits & bitmask) == bitmask){
      tag_bits_array[i] = 1;
    }
    else{
      tag_bits_array[i] = 0;
    }
    if( (device_id & bitmask) == bitmask){
      id_bits_array[i] = 1;
    }
    else{
      id_bits_array[i] = 0;
    }
    bitmask = bitmask/2;
  }
  //now that the array is built, send the protocol
  //start by sending a start code
  //note: disabling interrupts doesn't help timing.
  Timer1.pwm(ir_led_pin, 512);       //send pulse
  delayMicroseconds(PROTOCOL_START_CODE);       //delay for tag time
  Timer1.disablePwm(ir_led_pin);     //turn off Timer One

  //now, loop through the bits to send
  for( int i = 0; i < PROTOCOL_BITS ; i++ ){
    //first, delay based on the device id
    if( id_bits_array[i] == 0){
      delayMicroseconds(PROTOCOL_0);
    }
    else if(id_bits_array[i] == 1){
      delayMicroseconds(PROTOCOL_1);
    }
    Timer1.pwm(ir_led_pin, 512);               //start pulse
    //then send a pulse for either a 0 or 1
    if( tag_bits_array[i] == 0){
      delayMicroseconds(PROTOCOL_0);
    }
    else if(tag_bits_array[i] == 1){
      delayMicroseconds(PROTOCOL_1);
    }
    Timer1.disablePwm(ir_led_pin);               //turn stop pulse
  }
  //then, add a final delay and a final start code
  delayMicroseconds(PROTOCOL_DELAY);
  Timer1.pwm(ir_led_pin, 512);       //send pulse
  delayMicroseconds(PROTOCOL_START_CODE);       //delay for tag time
  Timer1.disablePwm(ir_led_pin);     //turn off Timer One
}
*/

/*
check_if_tagged f(x)
Checks if there is any serial data from the IR receivers. If there is a valid tag received,
- call take_damage to take damage
- change the color of the RGB LED
- make a sound
Input: nothing
Output: nothing (void)
*/

void check_if_tagged( void ){
  byte bonus_damage = 0;
  byte device_id = 0;
  static byte message_array[TAG_ARRAY_LEN];
  static byte message_array_index;
  unsigned long Sum_Tag = 0;  //used to send tag wirelessly
  while (mySerial.available()) {        // while data is available
    
    //if we are debugging, write what you see
    message_array[message_array_index] = mySerial.read();
    if(SERIAL_DEBUG){
      Serial.print(F("Received IR byte: "));
      Serial.println(message_array[message_array_index], BIN);
    }
    message_array_index = message_array_index + 1;
      
    //if you have 4 bytes, try to read it
    if(message_array_index == TAG_ARRAY_LEN){
      if(SERIAL_DEBUG){
        Serial.println(F("Received 4 bytes"));
      }

      //see if checksum is valid, and the header (first 5 bits) is 01010, then it's a valid tag
      if( (message_array[3] == calculate_checksum(message_array) ) &&
          ( (message_array[0] & 0xF8) ==  80 ) ){
        //reset the array after reading the message
        message_array_index = 0;

        
        if(SERIAL_DEBUG){
          Serial.println(F("Checksum matched"));
          Serial.print(F("Header: "));
          Serial.println((message_array[0] >> 3));
          Serial.print(F("Tag: "));
          Serial.println(((message_array[1] & 0x07) << 2) + (message_array[2] >> 6));
          Serial.print(F("Team: "));
          Serial.println((message_array[0] & 0x07));
          Serial.print(F("Bonus Damage: "));
          Serial.println( message_array[1] >> 3 );
          Serial.print(F("Device ID: "));
          Serial.println((message_array[2] & B00111111));
          Serial.print(F("Checksum: "));
          Serial.println(message_array[3]);
        }
        

        //do something! you got a valid tag (maybe - check for errors in the data)
        bonus_damage = message_array[1] >> 3;
        device_id = message_array[2] & B00111111;
        Sum_Tag = (long(message_array[0]) << 24) + (long(message_array[1]) << 16) + long(message_array[2] << 8) +  long(message_array[3]);
        
        //First, check the device id. you can't tag yourself and the id shouldn't be above those you're using, unless it's the controller
        if((device_id != DEVICE_ID) && ((device_id < MAX_USED_DEVICE_ID) || device_id == CONTROLLER_DEVICE_ID)){
          //next, verify that if the tag is changing the game or a rune, that it's from a base or controller
          if( (message_array[0] >> 3) >= MIN_RUNE_VALUE){
            //if I'm receiving a rune, it is valid only if sent from a base and has no bonus damage 
            if( (device_id >= MIN_BASE_ID) && (bonus_damage == 0) ){
              //if it's valid, process the tag
              take_damage((((message_array[1] & 0x07) << 2) + (message_array[2] >> 6)), (message_array[0] & 0x07), 
                bonus_damage, 
                device_id, Sum_Tag); 
            }
          }
          else if( (message_array[0] >> 3) >= MIN_CHANGE_GAME_STATE_VALUE){
            //if I'm receiving a "change game state" tag, the rules are more strict. bonus damage is 0, team is white, and device id must be controller
            if( (device_id == CONTROLLER_DEVICE_ID) && (bonus_damage == 0) && ( (message_array[0] & 0x07) == 0) ){
              //if it's valid, process the tag
              take_damage( (((message_array[1] & 0x07) << 2) + (message_array[2] >> 6)), (message_array[0] & 0x07), 
                bonus_damage, 
                device_id, Sum_Tag); 
            }
          }
          else{
            //if it's not one of the tags that have more rules around it, process it.
            take_damage( (((message_array[1] & 0x07) << 2) + (message_array[2] >> 6)), (message_array[0] & 0x07), 
                bonus_damage, 
                device_id, Sum_Tag);
          }
              
        }

        //if you are in debug mode, send everything!
        if(NRF_DEBUG_MODE){
          wireless_transceiver(device_id, 10, DEVICE_ID, Sum_Tag);
        }


      }
      else{
        //you didn't get a valid message yet. throw out the last byte and wait for the next one
        message_array[0] = message_array[1];
        message_array[1] = message_array[2];
        message_array[2] = message_array[3];
        message_array_index = 3;
        if(SERIAL_DEBUG){
          Serial.println(F("Checksum/Header error"));
          Serial.print(F("Checksum: "));
          Serial.println(calculate_checksum(message_array));
          Serial.print(F("Header (80): "));
          Serial.println(message_array[0] & 0xF8);
        }
      }
    }
  }
}

/*
time_pulse f(x)
Times how long a pin is one state and returns that value in microseconds1
Inputs: pin to read (int), state (int)
Outputs: (int) - time in microseconds that pin was that state
etc.
*/
int time_pulse( int Pin_Read, int state ){
  int ir_receiver = state;                     //keeps track of infrared receiver state
  int unsigned long microsec_start = micros(); //keeps track of when we start reading pin
  //wait for infrared receiver to stop being that state
  while (ir_receiver == state){
    //delayMicroseconds(5);
    ir_receiver = digitalRead( Pin_Read );
    if((micros() - microsec_start) > TIMEOUT) break;  //if you're over time, break.
  }
  return micros() - microsec_start;   //return how long the pin was in that state
}

/*
change_RGB f(x)
this function changes the color of the RGB LED to the
correct color depending on what is sent into the function
inputs: color_to_be (int) - determines what color to turn RGB's to
outputs: none (void) - changes RGB LED
*/

void change_RGB( int color_to_be ){
  int color = color_to_be;
  /*
  if(SERIAL_DEBUG){
    Serial.print(F("Color: "));
    Serial.println(color_to_be);
  }
  */
  if(RUNE_SENT >= MIN_RUNE_ID && color_to_be >= 0){
    //if I'm a rune, and not supposed to turn off, pick a color based on the rune, not the team
    if(RUNE_SENT == 27){
      //extra life rune
      color = 9;
    }
    else if(RUNE_SENT == 28){
      //double armor rune
      color = 7;
    }
    else if(RUNE_SENT == 29){
      //double mana rune
      color = 8;
    }
    else if(RUNE_SENT == 30){
      //double health rune
      color = 2;
    }
    else if(RUNE_SENT == 31){
      //double damage rune
      color = 4;
    }
    else if(RUNE_SENT == 32){
      //spawn as juggernaut
      color = 11;
    }
  }
  //see what color to change the RGB's to
  switch (color) {
    case 0:
      //normal tag - white
      analogWrite(RGB_Red, 100);       
      analogWrite(RGB_Green, 100);  
      analogWrite(RGB_Blue, 200);
    case 1:
      //soldier - yellow
      analogWrite(RGB_Red, 255);      
      analogWrite(RGB_Green, 255);  
      digitalWrite(RGB_Blue, LOW);
      break;
    case 2:
      //heal - magenta
      analogWrite(RGB_Red, 200);    
      analogWrite(RGB_Green, 0);  
      analogWrite(RGB_Blue, 123);
      break;
    case 3:
        //if I am a base, change color based on team
        if(CHARACTER_CLASS == 3){
          if(TEAM ==2){
            //if I'm on blue team, set RGB based on blue health
            color = (155/MAX_HEALTH)*HEALTH + 100;
            analogWrite(RGB_Blue, color);
            if(HEALTH < MAX_HEALTH){
              color = 75/HEALTH;
              analogWrite(RGB_Green, color);
            }
            else{
              digitalWrite(RGB_Green, LOW);
            }
            digitalWrite(RGB_Red, LOW); //turn off Green, since we don't us it
          }
          else if(TEAM ==1){
            //if I'm on red team, set RGB based on blue health
            color = (155/MAX_HEALTH)*HEALTH + 100;
            analogWrite(RGB_Red, color);
            if(HEALTH < MAX_HEALTH){
              color = 75/HEALTH;
              analogWrite(RGB_Green, color);
            }
            else{
              digitalWrite(RGB_Green, LOW);
            }
            digitalWrite(RGB_Blue, LOW); //turn off Green, since we don't us it
          }
          else if (TEAM == 0){
            //white team
            change_RGB( 0 );
          }
          else if (TEAM ==3){
            //green team
            change_RGB( 6 );
          }
          else if (TEAM == 4){
            //orange team
            change_RGB( 7 );
          }
          else if (TEAM == 5){
            //magenta team
            change_RGB( 2 );
          }
          else if (TEAM == 6){
            //yellow team
            change_RGB( 1 );
          }
          else if (TEAM == 7){
            //purple team
            change_RGB( 9 );
          }
        }
      break;
    case 4:
      //fire - red
      digitalWrite(RGB_Red, HIGH);
      digitalWrite(RGB_Green, LOW);
      digitalWrite(RGB_Blue, LOW);
      break;
    case 5:
      //ice - cyan
      analogWrite(RGB_Red, 0);
      analogWrite(RGB_Green, 45);
      analogWrite(RGB_Blue, 255);
      break;
    case 6:
      //poison - green
      digitalWrite(RGB_Red, LOW);
      analogWrite(RGB_Green, 255);
      digitalWrite(RGB_Blue, LOW);
      break;
    case 7:
      //rock - orange
      analogWrite(RGB_Red, 255);  
      analogWrite(RGB_Green, 50);  
      analogWrite(RGB_Blue, 0);
      break;
    case 8:
      //water - blue
      digitalWrite(RGB_Red, LOW);
      digitalWrite(RGB_Green, LOW);
      digitalWrite(RGB_Blue, HIGH);
      break;
    case 9:
      //LIGHTNING - purple
      analogWrite(RGB_Red, 50);
      analogWrite(RGB_Green, 0);
      analogWrite(RGB_Blue, 255);
      break;
    case 10:
      //necromancer - off-green
      analogWrite(RGB_Red, 0);
      analogWrite(RGB_Green, 255);
      analogWrite(RGB_Blue, 35);
      break;
    case 11:
      //juggernaut - off-white
      analogWrite(RGB_Red, 150);       
      analogWrite(RGB_Green, 100);  
      analogWrite(RGB_Blue, 255);
      break;
    default:
      //don't know what hit me. Turn off
      digitalWrite(RGB_Red, LOW);
      digitalWrite(RGB_Green, LOW);
      digitalWrite(RGB_Blue, LOW);
      break;
  }
}


/*
button_changed f(x)
checks to see if the button was just pressed. 
If the button was just pressed, it outputs a 1
If the button wasn't just pressed, it outputs a 0
input: button_state (int), button to check
output: 1 or 0 (int)
*/

int button_changed( int current_state, int button_checked ){
  //I need to remember two different buttons
  static int tag_button_last_state;
  static int ability_button_last_state;
  int last_state;               //last state button was in  
  
  if( button_checked == tag_button_pin){
    last_state = tag_button_last_state;
  }
  else if( button_checked == ability_button_pin){
    last_state = ability_button_last_state;
  }
  
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
  #if ZERO_POINT_NINE_EIGHT     //if the hardware version is above 0.98
  if (current_state == LOW){    //if the button is pressed
    if (last_state == HIGH){    //and it wasn't pressed last time
      result = 1;               //send a tag!
  #else
  if (current_state == HIGH){  //if the button is pressed
    if (last_state == LOW){    //and it wasn't pressed last time
      result = 1;              //send a tag!
  #endif
               
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
  
  if( button_checked == tag_button_pin){
    tag_button_last_state = current_state;
  }
  else if( button_checked == ability_button_pin){
    ability_button_last_state = current_state;
  }
  
  return result;                //return result
}


/*******
play_sound f(x)
This function plays all of the sounds that the arduino makes.
The sounds are stored in arrays in the beginning of the code.
It takes in what sound to play, and sets it on the piezo.
Periodically, we check to see if we should play a different note
There is always a delay of 10 ms between notes.
inputs: sound_to_play (int) - what sound to play
outputs: nothing (plays sound on piezo speaker)
********/

void play_sound( int sound_array[], int sound_delay_array[], int array_len){
  //the sound function needs to remember what sound it was playing:
  static int current_note;
  static int num_notes;
  static unsigned long note_time;
  
  //if array_len == 0, I'm checking to see if I play the next note
  if (array_len == 0){
    if (num_notes == current_note){
      //if I've played the last note, stop
    }
    else{
      if (millis() > note_time){
        //if the time has come to play a new note, set the tone!
        tone(piezo_pin, current_sound[current_note], current_sound_delay[current_note]);
        note_time = millis() + current_sound_delay[current_note] + extra_delay;
        current_note++;
      }
    }
  }
  else{
    
    //copy sound array into current sound
    memcpy(current_sound, sound_array, sizeof(sound_array[0])*array_len);
    memcpy(current_sound_delay, sound_delay_array, sizeof(sound_array[0])*array_len);
    
    //set up to play the first note
    current_note = 0;
    num_notes = array_len;
    note_time = millis();
    
    /*
    if(Serial_debug_mode){
      Serial.print("Array length: ");
      Serial.println(num_notes);
      for (int i = 0; i < array_len; i++){
        Serial.println(current_sound[i]);
      }
    }
    */
  }
 
  
}

#if ATMEGA4809_TRUE
/***************
play_mp3_sound f(x)
This function takes in an event of what just happened and plays the corresponding sound
The events are pre-defined as integers in the beginning of the code
inputs: event type (int) - the type of event (i was tagged, sending a tag, etc.)
        tag (int) - the tag this corresponds to (fire, water, etc.)
outputs: void (sends serial communication to mp3 player)
 */
void play_mp3_sound(int EventType, int Tag){
  //for each type of event, do something different
  static int Tag_hit_with_last_time;
  if(EventType > 0){
    //if I have a new sound to play
    if(int(player.checkPlayState()) == 1){       //if I'm playing anything already
      if(Tag_hit_with_last_time != Tag){         //if I'm already playing that sound, don't play it again
        player.stop();       
        pick_correct_sound(EventType, Tag);      //pick the correct next sound, then play it
        player.playSpecifiedDevicePath(DY::Device::Sd, NEXT_SOUND_ARRAY);
        if(SERIAL_DEBUG){
          Serial.print(F("Interrupted, then "));
          Serial.println(F("Played sound"));
        }
      }
    }
    else{
      //I'm not playing something, go ahead and play the sound
      pick_correct_sound(EventType, Tag);      //pick the correct next sound, then play it
      player.playSpecifiedDevicePath(DY::Device::Sd, NEXT_SOUND_ARRAY);
      if(SERIAL_DEBUG) Serial.println(F("Played sound"));
    }
    Tag_hit_with_last_time = Tag;
  }
  else{
    //I'm not playing a new sound. I'm checking if I have to play a state sound
    if(int(player.checkPlayState()) == 0){
      //if I'm not playing anything already, check if there is a state to play
      //add code here to play background states, like poison sound, ice sound, etc.
      if(FIRE > 0){
        //I'm on fire, play fire sound
        play_mp3_sound(HIT_EVENT, FIRE_TAG_NUM);
      }
      else if(ICE_STACKS > 0){
        play_mp3_sound(HIT_EVENT, ICE_TAG_NUM);
      }
      else if(POISON_STACKS > 0){
        play_mp3_sound(HIT_EVENT, POISON_TAG_NUM);
      }
       else if(WATER > 0){
        play_mp3_sound(HIT_EVENT, WATER_TAG_NUM);
      }
      else if(LIGHTNING_STACKS > 0){
        play_mp3_sound(HIT_EVENT, LIGHTNING_TAG_NUM);
      }
    }
  }
}

/*
void pick_correct_sound f(x)
This function sets a global array to the correct next sound to play based on the inputs
inputs: event type (int) - the type of event (i was tagged, sending a tag, etc.)
        tag (int) - the tag this corresponds to (fire, water, etc.)
outputs: void (sets global array)
 */
void pick_correct_sound(int EventType, int Tag){
  if(EventType == HIT_EVENT){
      //I'm making a sound because I am hit. What tagged me?
      switch (Tag){
       case NORMAL_TAG_NUM:
         //hit with normal tag, so set next sound to normal tag
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_NORMAL_S[i];
         }
         break;
       case HEAL_TAG_NUM:
         //hit with heal tag
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_HEAL_S[i];
         }
         break;
       case BASE_TAG_NUM:
         //hit with heal tag
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = EXTRA_LIFE_S[i];
         }
         break;
       case FIRE_TAG_NUM:
         //hit with fire tag
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = AMBIENT_FIRE_S[i];
         }
         break;
       case ICE_TAG_NUM:
         //hit with ice
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_ICE_S[i];
         }
         break;
       case POISON_TAG_NUM:
         //hit with poison
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = AMBIENT_POISON_S[i];
         }
         break;
       case ROCK_TAG_NUM:
         //hit with rock
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_ROCK_S[i];
         }
         break;
       case WATER_TAG_NUM:
         //hit with water
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = AMBIENT_WATER_S[i];
         }
         break;
       case LIGHTNING_TAG_NUM:
         //hit with lightning
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = AMBIENT_ELECTRICITY_S[i];
         }
         break;
       default:
         //hit with normal tag, so set next sound to normal tag
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_NORMAL_S[i];
         }
         break;
         break;
      }
  }
  else if(EventType == TAG_EVENT){
    //I'm sending a tag. Make that type of sound
  }
  else if(EventType == OTHER_EVENT){
    //I'm making some other sound
    switch (Tag){
       case GOOSE_SOUND_NUM:
         //Goose sound
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = CAN_NOT_TAG_GOOSE_S[i];
         }
         break;
       case SHIELD_RECHARGE_SOUND_NUM:
         //shield recharge sound
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = SHIELD_RECHARGE_S[i];
         }
         break;
       case SHIELD_OUT_SOUND_NUM:
         //shield out sound
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = NO_SHIELD_S[i];
         }
         break;
       case OUT_SOUND_NUM:
         //out sound
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = OUT_S[i];
         }
         break;
       case HIT_SOMEONE_SOUND_NUM:
         //hit someone sound
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = HIT_ENEMY_S[i];
         }
         break;
       case JUGGERNAUT_HERE_SOUND_NUM: 
         //Juggernaut is approaching
         for(int i = 0; i < sizeof(TAG_NORMAL_S); i++){
          NEXT_SOUND_ARRAY[i] = JUGGERNAUT_APPROACH_S[i];
         }
         break;
       default:
         break;
  }
 }
}

#endif


/*
respawn f(x)
the respawn function revives the player and sets their team, class and other variables
inputs: character_class, team, sent_device. 
if character_class or team is 0, you can pick a new class / team
otherwise, change to that class / team
outputs: void. Changes global variables.
*/
void respawn( byte character_class, byte team, byte bonus_damage, byte sent_device ){
  //if you are allowed to change classes, set the timer to allow that
  if(character_class == 0){
    change_class(1);     //set up display to change class, and allow class changes
  }
  else{
    //otherwise, I'm supposed to change classes, do so
    CHARACTER_CLASS = character_class;
    change_class(2);                  //update display for changed class, but don't allow class changes
  }

  //if you are allowed to change teams, change team (zombie checking happens in check_team() )
  if(team == 0){
    check_team();
  }
  else{
    TEAM = team;
  }

  //set all class variables based on current class and team, and use bonus damage for bonuses to health
  set_class(bonus_damage, 1);
  
  //if you have a rune active, deactivate it
  DOUBLE_DAMAGE_RUNE = 0;
  DOUBLE_HEALTH_RUNE = 0;
  ARCANE_RUNE = 0;
  DOUBLE_ARMOR_RUNE = 0;

  //play startup sound
  #if ATMEGA4809_TRUE
    //if you are using that sound module, check those sounds too
    play_mp3_sound(OTHER_EVENT, SHIELD_RECHARGE_SOUND_NUM);     //play shield recharge sound
  #else
    //using old module, use this sound
    play_sound(startup_sound, startup_sound_delay, startup_sound_length);
  #endif
  

  //upon startup, you can't send tags for a few seconds, to discourage spawning near people
  //unless you are hit by the controller or a base. Then send tags immediately after startup sound
  if(sent_device != CONTROLLER_DEVICE_ID && sent_device < MIN_BASE_ID){
    cooldown_time = millis() + RESPAWN_TIME; 
  }
}

/*************
check_team f(x)
Checks your team based on the voltage on the potentiometer
inputs: void
outputs: void (changes global variables)
 */
void check_team( void ){
  int team_voltage = analogRead(TEAM_PIN);
  int new_team = map(team_voltage, 0, 1023, 8, 0);
  //I'm getting some jitter on team 0 with the trim pot. To fix this, 
  //I'm going from 8 to 0, subtracting 1, and then setting negatives to 0, so the mapping works better
  if(new_team == 0){
    new_team = 0;
  }
  else{
    new_team = new_team - 1;
  }

  if(new_team != TEAM){
    //if the team you are registering isn't the team you are on, change teams, unless you were killed by a Zombie
    if(ZOMBIE_GOT_ME == 0){
      //if ZOMBIE_GOT_ME is 0, a zombie hasn't killed you, so you can change teams
      TEAM = new_team;
      //reset the screen to show new team
      update_display(2, 0);
      if(SERIAL_DEBUG) Serial.println(F("chose a new team"));
    }
  }
}

/*********
set_class f(x)
sets your class variables based on your current class
inputs: void
outputs: void (sets global variables)
 */
byte set_class( byte bonus_damage, byte use_new_bonus ){
  //set up variables based on class
  byte tag_sent = 1;             //start with a tag of 1
  byte ability_sent = 2;         //start with ability of healer
  AMMO = MAX_AMMO;               //no matter what, set ammo to max
  static int health_multiplier;  //whether or not you start at full health
  if(use_new_bonus == 0){
    //if I'm not using the new bonus damage, use the old health multiplier
    if(health_multiplier < MIN_RESPAWN_HEALTH_PERCENT){
      //if health multiplier is less than minimum, set to minimum
      health_multiplier = MIN_RESPAWN_HEALTH_PERCENT;
    }
    if(health_multiplier > 99){
      //if health multiplier is over 100, use 100
      health_multiplier = 100;
    }
  }
  else{
    //if I'm supposed to calculate a new bonus damage, do so
    //note that every 1 point of bonus damage is a 10% boost to health, so add that in
    byte bonus_damage_percent = bonus_damage*10;
    if(bonus_damage_percent > 99){
      //if you have more than 99% bonus damage, spawn with 100% hp
      health_multiplier = 100;
    }
    else if (bonus_damage_percent > MIN_RESPAWN_HEALTH_PERCENT){
      health_multiplier = bonus_damage;
    }
    else{
      //if you weren't above minimum respawn health percent, spawn with that
      health_multiplier = MIN_RESPAWN_HEALTH_PERCENT;
    }
  }
  
  
  switch(CHARACTER_CLASS){
    case 1:
      //I'm soldier. Set up variables
      MAX_HEALTH = SOLDIER_MAX_HEALTH;
      HEALTH = (SOLDIER_SART_HEALTH * health_multiplier)/100 ;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      MANA = 10;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 10;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      ability_sent = 11;   //ability is triple damage
      break;
    case 2:
      //I'm medic. Set up variables
      MAX_HEALTH = NORMAL_START_HEALTH;
      HEALTH = (NORMAL_START_HEALTH * health_multiplier)/100;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      MANA = 20;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 20;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      //change ability to heal sound and cooldown
      ability_sound_length = heal_ability_sound_length;
      for(int i = 0; i < ability_sound_length; i++){
        ability_sound[i] = heal_ability_sound[i];
        ability_sound_delay[i] = heal_ability_sound_delay[i];
      }
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      PASSIVE_ABILITY_COOLDOWN = MEDIC_PASSIVE_COOLDOWN;    //passive ability cooldown is 10 seconds for medit
      ability_sent = 2;
      break;
    case 3:
      //change settings for base
      MANA = 10;
      MAX_MANA = 50;
      MANA_COST = 1;
      if(KING_OF_THE_HILL > 0){
        //you are playing king of the hill
        MAX_HEALTH = 30;
        HEALTH = 30;
        PASSIVE_ABILITY_COOLDOWN = POINT_GAIN_DELAY;
      }
      else{
        MAX_HEALTH = 250;
        HEALTH = 250;
      }
      if(RUNE_SENT > 0){
        //if you are a rune, use rune cooldown
        ability_cooldown = RUNE_COOLDOWN;
      }
      else{
        //if you are not a rune, do normal cooldown
        ability_cooldown = NORMAL_COOLDOWN;
      }
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      MANA_REGEN = 3;
      tag_cooldown = NORMAL_COOLDOWN;
      ability_sent = 3;
      break;
    case 4:
      //I'm pyro. Set up variables
      if((SURVIVAL_MODE > 0) && (TEAM == SURVIVAL_TEAM)){
        //if you are playing survival mode and are on survival team, use survival stats
        MAX_HEALTH = SURVIVE_FIRE_MAX_HEALTH + SURVIVE_FIRE_MAX_HEALTH_PER_STACK*FIRE_DEATH_STACKS;
        HEALTH = ((SURVIVE_FIRE_STARTING_HEALTH + SURVIVE_FIRE_HEALTH_PER_STACK*FIRE_DEATH_STACKS)* health_multiplier)/100;
        PASSIVE_ABILITY_COOLDOWN = SURVIVE_FIRE_DEATH_DELAY;
        
      }
      else{
        //if I'm not playing survival mode, use normal stuff
        MAX_HEALTH = FIRE_MAX_HEALTH + FIRE_MAX_HEALTH_PER_STACK*FIRE_DEATH_STACKS;
        HEALTH = ((FIRE_STARTING_HEALTH + FIRE_HEALTH_PER_STACK*FIRE_DEATH_STACKS)* health_multiplier)/100;
        PASSIVE_ABILITY_COOLDOWN = FIRE_DEATH_DELAY;
      }
      MANA = 20;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 20;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      //note: pyro health is also set in check_passives. You'll need to change it there too.
      
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      //change ability to fire sound and cooldown
      ability_sound_length = fire_ability_sound_length;
      for(int i = 0; i < ability_sound_length; i++){
        ability_sound[i] = fire_ability_sound[i];
        ability_sound_delay[i] = fire_ability_sound_delay[i];
      }
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;

      ability_sent = 4;
      break;
    case 5:
      //I'm Ice. Set up variables. 
      MANA = 20;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 20;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      MAX_HEALTH = NORMAL_MAX_HEALTH;
      HEALTH = (NORMAL_START_HEALTH * health_multiplier)/100;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      ability_sent = 5;
      break;
    case 6:
      //I'm Poison. Set up variables
      MANA = 20;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 20;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      MAX_HEALTH = FRAGILE_MAX_HEALTH;
      HEALTH = (FRAGILE_START_HEALTH * health_multiplier)/100;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      ability_cooldown = NORMAL_COOLDOWN*2;
      tag_cooldown = NORMAL_COOLDOWN*2;
      ability_sent = 6;
      tag_sent = 0;  //send double damage
      break;
    case 7:
      //I'm Rock. Set up variables
      MANA = 30;
      MANA_COST = 2;
      MANA_REGEN = 1;
      MAX_MANA = 30;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      MAX_HEALTH = FRAGILE_MAX_HEALTH;
      HEALTH = (FRAGILE_START_HEALTH * health_multiplier)/100;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      ROCK_STACKS = HIGHER_MAX_ROCK_STACKS;
      MAX_ROCK_STACKS = HIGHER_MAX_ROCK_STACKS;
      PASSIVE_ABILITY_COOLDOWN = 5000;    //rock passive is 5 seconds
      ability_sent = 7;
      break;
    case 8:
      //I'm Water. Set up variables
      MANA = 5;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 5;
      MANA_DELAY = 2000;
      MAX_HEALTH = NORMAL_MAX_HEALTH;
      HEALTH = (NORMAL_START_HEALTH * health_multiplier)/100;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      ability_sent = 8;
      break;
    case 9:
      //I'm Lightning. Set up variables
      MANA = 35;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 35;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      MAX_HEALTH = NORMAL_MAX_HEALTH;
      HEALTH = (NORMAL_START_HEALTH * health_multiplier)/100;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      //change ability to fire sound and cooldown
      ability_sound_length = fire_ability_sound_length;
      for(int i = 0; i < ability_sound_length; i++){
        ability_sound[i] = fire_ability_sound[i];
        ability_sound_delay[i] = fire_ability_sound_delay[i];
      }
      ability_sent = 9;
      break;
    case 10:
      //I'm necromancer. Set up variables
      MANA = 5;
      MANA_COST = 1;
      MANA_REGEN = 1;
      MAX_MANA = 5;
      MANA_DELAY = NECRO_MANA_DELAY;
      MAX_HEALTH = NORMAL_MAX_HEALTH;
      HEALTH = (NORMAL_START_HEALTH * health_multiplier)/100;
      ability_cooldown = NORMAL_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      ability_sent = 3;  //send revive as ability
      break;
    case 11:
      //I'm juggernaut. Set up variables
      MANA = 500;
      MANA_COST = 0;
      MANA_REGEN = 1;
      MAX_MANA = 500;
      MANA_DELAY = DEFAULT_MANA_DELAY;
      MAX_HEALTH = JUG_MAX_HEALTH;
      HEALTH = (JUG_START_HEALTH * health_multiplier)/100;
      ROCK_STACKS = HIGHER_MAX_ROCK_STACKS;
      MAX_ROCK_STACKS = HIGHER_MAX_ROCK_STACKS;
      ability_cooldown = JUG_ABILITY_COOLDOWN;
      tag_cooldown = NORMAL_COOLDOWN;
      ability_sent = 0;  //ability is double damage (not used)
      tag_sent = 0;  //send double damage
      break;
    default:
      MANA = 10;
      MANA_COST = 1;
      MAX_HEALTH = 200;
      HEALTH = 150;
      MAX_ROCK_STACKS = NORMAL_MAX_ROCK_STACKS;
      ROCK_STACKS = 0;
      MANA_REGEN = 10;
      //change ability to heal sound and cooldown
      ability_sound_length = heal_ability_sound_length;
      for(int i = 0; i < ability_sound_length; i++){
        ability_sound[i] = heal_ability_sound[i];
        ability_sound_delay[i] = heal_ability_sound_delay[i];
      }
      ability_cooldown = 350;
      tag_cooldown = 350;
      break;
  }

  //if you are using ammo, use a multiple of mana and max mana
  if(USE_AMMO > 0){
    MANA = MANA*AMMO_MANA_MULTIPLIER;
    MAX_MANA = MAX_MANA*AMMO_MANA_MULTIPLIER;
  }

  //update display with class and team information
  update_display(2, 0);

  /*
  if(SERIAL_DEBUG){
    Serial.print(F("Character Class: "));
    Serial.println(CHARACTER_CLASS);
    Serial.print(F("Which is: "));
    if(CHARACTER_CLASS == 1) Serial.println(F("Soldier"));
    if(CHARACTER_CLASS == 2) Serial.println(F("Medic"));
    if(CHARACTER_CLASS == 3) Serial.println(F("Base"));
    if(CHARACTER_CLASS == 4) Serial.println(F("Pyro"));
    if(CHARACTER_CLASS == 5) Serial.println(F("Ice"));
    if(CHARACTER_CLASS == 6) Serial.println(F("Poison"));
    if(CHARACTER_CLASS == 7) Serial.println(F("Rock"));
    if(CHARACTER_CLASS == 8) Serial.println(F("Water"));
    if(CHARACTER_CLASS == 9) Serial.println(F("Lightning"));
    if(CHARACTER_CLASS == 10) Serial.println(F("Necromancer"));
    if(CHARACTER_CLASS == 11) Serial.println(F("Juggernaut"));
    #if USE_RESISTORS_FOR_CLASS_AT_STARTUP            //if you're checking resistors for class
    Serial.print(F("Analog Read of Team Voltage: "));
    Serial.println(team_voltage);
    Serial.print(F("Voltage in Volts: "));
    Serial.println(float(team_voltage*0.0049));
    #endif
    Serial.print(F("Team: "));
    Serial.println(TEAM);
    Serial.print(F("Which is: "));
    if(TEAM == 1) Serial.println(F("Red"));
    if(TEAM == 2) Serial.println(F("Blue"));
    if(TEAM == 3) Serial.println(F("Green"));
    if(TEAM == 0) Serial.println(F("White"));
    if(TEAM == 4) Serial.println(F("Orange"));
    if(TEAM == 5) Serial.println(F("Magenta"));
    if(TEAM == 6) Serial.println(F("Yellow"));
    if(TEAM == 7) Serial.println(F("Purple"));
  }
  */
  
  //define tag sent array
  setup_tag_array( tag_sent, TEAM, 0, SENT_ARRAY_TAG);

  //define ability sent array
  setup_tag_array( ability_sent, TEAM, 0, SENT_ARRAY_ABILITY);
  
  //calculate aim assist tag
  #if AUTOMATIC_AIM_ASSIST
  setup_tag_array( AIM_ASSIST_AUTOMATIC, TEAM, 0, SENT_ARRAY_AIM_ASSIST);
  #else
  setup_tag_array( AIM_ASSIST_TAG, TEAM, 0, SENT_ARRAY_AIM_ASSIST);
  #endif
}

/******
change_class f(x)
Checks to see if you are allowed to change your class, and is called in the main loop to change classes
and update the display as necessary
inputs: state: 0 = checking, 1 = allow changing classes. 2 = don't allow changing classes, just update display
outputs: none (updates display, class, and team)
 */

void change_class( byte state ){
  static int past_state;
  static byte changing_class_now;
  int current_state = digitalRead( CLASS_PIN );  //read current state of the class pin

  if(state > 0){
    //if state isn't 0 (meaning I'm not checking the buttons), save the current state, and set time to change class
    changing_class_now = state;
    CHANGE_CLASS_TIME = millis() + RESPAWN_CHANGE_CLASS_TIME;
  }

  if( changing_class_now > 0){
    //if we can check for changing classes, do all the checking. If not, ignore this stuff
    if(CHANGE_CLASS_TIME > millis() && changing_class_now == 1){
  
      //check team (this also updates display if team is changed)
      check_team();
  
      //read current state of the ability pin
      int current_ability_state = digitalRead( ability_button_pin );
    
      if( current_state == LOW && past_state == HIGH ){
        //change class, button was pressed
        CHARACTER_CLASS = CHARACTER_CLASS + 1;
        if(CHARACTER_CLASS == 3) CHARACTER_CLASS = CHARACTER_CLASS + 1;  //don't let players become bases
        if(current_ability_state == LOW){
          //if the ability button is held down, you can choose juggernaut
          if(CHARACTER_CLASS > NUMBER_OF_CLASSES){
            //if you have gone over the number of classes, reset to 1
            CHARACTER_CLASS = 1;
          }
        }
        else{
          //if the ability button is not held down, you can't pick juggernaut
          if(CHARACTER_CLASS == NUMBER_OF_CLASSES){
            //if you tried to pick juggernaut, reset to 1
            CHARACTER_CLASS = 1;
          }
        }
        
    
        //call the function to change all the class variables, using old bonuses to health, and update display
        set_class(0, 0);
    
        //ensure you have time to keep changing classes if you need to take some time to select your class
        if((CHANGE_CLASS_TIME - millis() ) < CHANGE_CLASS_MINIMUM_TIME){
          CHANGE_CLASS_TIME = millis() + CHANGE_CLASS_MINIMUM_TIME;
        }
      }
    
      //set past state to current state
      past_state = current_state;
    }
    else if(CHANGE_CLASS_TIME > millis() && changing_class_now == 2){
      //in this case, just keep the display updated, so hang out
    }
    else{
      if(changing_class_now > 0 && current_state == HIGH){
        //if you are no longer changing classes, and the change class button isn't pressed
        update_display(0, 0);
        changing_class_now = 0;
      }
    }
  }
  else{
    if( current_state == LOW){
        //show current team and class, and get rid of this once released
        changing_class_now = 2;
        //show team and class
        update_display(2, 0);
      }
  }

}

/********
telegraph_hit f(x)
Telegraph takes in the tag you were hit with and flashes LED's based on it.
This blinks an LED without using the delay function, so the arduino can do other stuff
while the LED is on / off.
It works as follows: 
1. When hit with a tag, turn off the RGB LED's until LED_off_time
2. After LED_off_time, displays the tag that hit you
3. at Time_to_go_back_to_normal, goes back to displaying your character class

inputs: tag_received (int) the tag that hit you (-1 is check everything, not receive a tag)
outputs: flashes LED's on hit_led_pin
*****/

void telegraph_hit( int tag_received ){
  static unsigned long LED_off_time;
  static unsigned long Time_to_go_back_to_normal;
  static byte update_things_once;
  unsigned long current_time = millis();
  
  if ( tag_received != -1 && HEALTH > 0){
    //if I've been tagged and have health turn on haptic feedback (if it's damaging)
    /*
    if(SERIAL_DEBUG){
            Serial.print(F("Tag received: "));
            Serial.println(tag_received);
            Serial.print(F("Character Class: "));
            Serial.println(CHARACTER_CLASS);
          }
          */
    if(tag_received != 2 && tag_received != 7 && tag_received != 3){
      //if you are immune to the hit, don't activate "damage" led or motor vibration
      digitalWrite(hit_led_pin, HIGH); 
    }

    //play the hit sound
    #if ATMEGA4809_TRUE
      play_mp3_sound(HIT_EVENT, tag_received);
    #else
      play_sound(received_tag_sound, received_tag_sound_delay,
                   received_tag_sound_length); 
    #endif

    //this tracks what you've been hit by, and that you are currently telegraphing it
    TELEGRAPHING_HIT = tag_received;
    //set a tracker to update display once
    update_things_once = 1;    

    //update display inverted if taking damage
    if(tag_received != 2 && tag_received != 7 && tag_received != 3) update_display(1, 0);

    //turn RGB's off
    change_RGB(-1);
    //turn LED's off for a short time
    if(RUNE_SENT > 0){
      //if you sent a rune, turn off for longer
      if(tag_received >= MIN_RUNE_ID && tag_received <= MAX_RUNE_ID){
        LED_off_time = current_time + RUNE_COOLDOWN;
      }
      else{
        LED_off_time = current_time + HIT_BASE_COOLDOWN_TIME;
      }
    }
    else{
      //otherwise, set to normal time
      LED_off_time = current_time + HIT_OFF_TIME;
    }

    //if you are not a base, you telegraph hits in one way and have haptic feedback
    if(CHARACTER_CLASS != 3){
      //turn RGB's back to normal after for some time, based on what hit you
      //and ignore telegraphing other statuses for that time
      switch (tag_received){
        case 3:
          Time_to_go_back_to_normal = current_time + RESPAWN_TIME;
          DISPLAY_STATUS_TIME = current_time + RESPAWN_TIME;
          break;
        case 4:
          Time_to_go_back_to_normal = current_time + FIRE_DELAY;
          DISPLAY_STATUS_TIME = current_time + FIRE_DELAY;
          break;
        case 5:
          Time_to_go_back_to_normal = current_time + ICE_DELAY;
          DISPLAY_STATUS_TIME = current_time + ICE_DELAY;
          break;
        case 6:
          Time_to_go_back_to_normal = current_time + POISON_DELAY;
          DISPLAY_STATUS_TIME = current_time + POISON_DELAY;
          break;
        case 7:
          Time_to_go_back_to_normal = current_time + ROCK_DISPLAY_TIME;
          DISPLAY_STATUS_TIME = current_time + ROCK_DISPLAY_TIME;
          break;
        case 8:
          Time_to_go_back_to_normal = current_time + WATER_DELAY;
          DISPLAY_STATUS_TIME = current_time + WATER_DELAY;
          break;
        case 9:
          Time_to_go_back_to_normal = current_time + LIGHTNING_DELAY;
          DISPLAY_STATUS_TIME = current_time + LIGHTNING_DELAY;
          break;
        default:
          Time_to_go_back_to_normal = current_time + HIT_DISPLAY_TIME;
          DISPLAY_STATUS_TIME = current_time + HIT_DISPLAY_TIME;
          break;
      }
    }
    else{
      //you are a base. Telegraph the hits for shorter so people don't get confused with red or blue
      if(RUNE_SENT > 0){
      //if you are a rune, and sent a rune, have RGB turn on after longer
      if(tag_received >= MIN_RUNE_ID && tag_received <= MAX_RUNE_ID){
        Time_to_go_back_to_normal = current_time + HIT_BASE_COOLDOWN_TIME + RUNE_COOLDOWN;
        DISPLAY_STATUS_TIME = current_time + HIT_BASE_COOLDOWN_TIME + RUNE_COOLDOWN;
      }
      else{
        Time_to_go_back_to_normal = current_time + HIT_BASE_COOLDOWN_TIME;
        DISPLAY_STATUS_TIME = current_time + HIT_BASE_COOLDOWN_TIME;
      }
      
    }
    else{
      //otherwise, set to normal time
      Time_to_go_back_to_normal = current_time + HIT_BASE_COOLDOWN_TIME;
      DISPLAY_STATUS_TIME = current_time + HIT_BASE_COOLDOWN_TIME;
    }
      
    }
  }
  else{
    //if I'm not flashing an LED, I'm checking to see if I should turn the LED's on or return to normal
    //first, check to see if it's time to stop telegraphing
    if (current_time > Time_to_go_back_to_normal && TELEGRAPHING_HIT > -1 && update_things_once == 0){
      //if you've updated stuff and are done telegraphing the hit, go back to normal
      if(HEALTH > 0){
        #if SHOW_CLASS_CONSTANTLY
          //if you have set to show class normally, do so
          change_RGB(CHARACTER_CLASS);
        #else
          //if you don't want to show class normally, show nothing
          change_RGB(-1);
        #endif
        if( TELEGRAPHING_HIT == 10 && POISON_STACKS > 0 ){
          //if you were taking poison damage, and still have stacks left, go back to showing poison
          change_RGB(6);
        }
      }
      if(TELEGRAPHING_HIT == 3) update_display(0, 0);   //reset display if you were revived
      TELEGRAPHING_HIT = -1;             //stop telegraphing a hit
      //turn off LED if you are  a base
      if(CHARACTER_CLASS == 3) digitalWrite(hit_led_pin, LOW); 
    }
    else if (current_time > LED_off_time && update_things_once == 1){
      //if it's past time to keep RGB's off, but not time to go back to normal,

      //turn off haptic feedback if not a base
      if(CHARACTER_CLASS != 3) digitalWrite(hit_led_pin, LOW);
      
      if (TELEGRAPHING_HIT == 1) {
          //if you get hit with a normal tag, make the normal tag color
          //that's a 0, not a 1
          change_RGB(0);
        }
      else{
        change_RGB(TELEGRAPHING_HIT);
      } 
      //return screen to normal
      update_display(0, 0);
      //don't do this update again
      update_things_once = 0;
    }
  }
}

/*******
check_passives f(x)
check_passives checks wether or not you have any passive abilities and uses them
 
*******/
void check_passives( int tag_received, int bonus_damage ){
  unsigned long current_time = millis();
  static unsigned long last_check;                 //for checking when you can revive
  unsigned long current_check = 0;                 //for checking when you can revive
  //if I'm hit by something, add a delay for when to turn it off
  switch (tag_received){
    case 4:
      //I'm hit by pyro
      if(bonus_damage > 0){
        //if my ability has a bonus, it lasts longer
        FIRE_TIME = current_time + ((FIRE_DELAY*(10 + bonus_damage))/10);
      }
      else{
        FIRE_TIME = current_time + FIRE_DELAY;
      }
      break;
    case 5:
      //I'm hit by ice
      if(bonus_damage > 0){
        //if my ability has a bonus, it lasts longer
        ICE_TIME = current_time + ((ICE_DELAY*(10 + bonus_damage))/10);
      }
      else{
        ICE_TIME = current_time + ICE_DELAY;
      }
      //if you already have water, reset duration (Note: I think this would shut people down way too much)
      //if(WATER > 0) WATER_TIME = current_time + WATER_DELAY;
      break;
    case 6:
      //I'm hit by poison. Only reset the duration if not doing so would make you take damage immediately
      if(POISON_TIME < current_time) POISON_TIME = current_time + POISON_DELAY;
      break;
    case 8:
      //I'm hit by water
      if(bonus_damage > 0){
        //if my ability has a bonus, it lasts longer
        WATER_TIME = current_time + ((WATER_DELAY*(10 + bonus_damage))/10);
        }
      else{
        WATER_TIME = current_time + WATER_DELAY;
      }
      //if you already have ice, reset duration
      if (ICE_STACKS > 0) ICE_TIME = current_time + ICE_DELAY;
        
      break;
    case 9:
      //I'm hit by lightning
      if(bonus_damage > 0){
        //if my ability has a bonus, it lasts longer
        LIGHTNING_TIME = current_time + ((LIGHTNING_DELAY*(10 + bonus_damage))/10);
        }
      else{
        LIGHTNING_TIME = current_time + LIGHTNING_DELAY;
      }
      break;
    default:
      break;
  }

  //check passive abilities, if you're alive
  if(HEALTH > 0){
    //check mana regeneration (can't regenerate mana while hit with lightning, or when AUTO_RESPAWN_MANA is off)
    if (current_time > MANA_TIME && LIGHTNING_STACKS == 0
        && AUTO_RESPAWN_MANA > 0){
      //add mana, but not over max
      if (MANA >= MAX_MANA){
        //reset time to add mana, and do nothing else
        MANA_TIME = current_time + MANA_DELAY;
      }
      else{
        //add to mana, but not over the max
        MANA = MANA + MANA_REGEN;
        if (MANA > MAX_MANA) MANA = MAX_MANA;
        //reset time to add mana
        MANA_TIME = current_time + MANA_DELAY; 
        //update display normally
        update_display(0, 0);
      }
    }

    //check to see if you need to send an aim assist
    if(ENABLE_AIM_ASSIST > 0){
      #if AUTOMATIC_AIM_ASSIST
        //aim assist is always on
        if(current_time > AIM_ASSIST_TIME){
          //you need to send an aim assist tag. Send it.
          mySerial.write(SENT_ARRAY_AIM_ASSIST, TAG_ARRAY_LEN);
          //then reset the aim assist cooldown
          AIM_ASSIST_TIME = current_time + AIM_ASSIST_COOLDOWN;
          if(SERIAL_DEBUG){
              Serial.println(F("Sent Aim Assist"));
            }
        }
        #else
        //Aim Assist is turned on by holding down the "tag" button
        if(current_time > AIM_ASSIST_TIME && current_time > cooldown_time 
            && digitalRead(tag_button_pin) == LOW){
          //you need to send an aim assist tag. Send it.
          mySerial.write(SENT_ARRAY_AIM_ASSIST, TAG_ARRAY_LEN);
          //then reset the aim assist cooldown
          AIM_ASSIST_TIME = current_time + AIM_ASSIST_COOLDOWN;
          if(SERIAL_DEBUG){
              Serial.println(F("Sent Aim Assist"));
            }
        }
        #endif
    }

    
    //check to see if you should take damage if you are playing that game type
    #if DAMAGE_EVERY_MINUTE
    if (current_time > DAMAGE_EVERY_MINUTE_TIME){
      //take damage
      take_damage(DMG_EVRY_MINUTE_TAG, 0, 0, 0, 0);
      //reset time to take damage
      DAMAGE_EVERY_MINUTE_TIME = current_time + DMG_EVRY_MINUTE_DELAY;
    }
    #endif
    
    //check passives for each class
    switch(CHARACTER_CLASS){
      case 2:
        //I'm a healer, and can heal every few seconds if I have that enabled
        #if HEALER_PASSIVE_HEALING
        if(current_time > PASSIVE_ABILITY_TIME){
          if(HEALTH < MAX_HEALTH){
            //if I'm not at full health
            take_damage(2, 0, 0, DEVICE_ID, 0);
          }
          PASSIVE_ABILITY_TIME = current_time + PASSIVE_ABILITY_COOLDOWN;
        }
        #endif
        break;
      case 3:
        //I'm a base, and have to be playing king of the hill. If so, score points if captured by red or blue
        if(KING_OF_THE_HILL > 0){
            if(current_time > PASSIVE_ABILITY_TIME){
              if(TEAM == 1){
                //score one for red team
                RED_TEAM_VICTORY_POINTS = RED_TEAM_VICTORY_POINTS + 1;
              }
              else if(TEAM == 2){
                //score one for blue team
                BLUE_TEAM_VICTORY_POINTS = BLUE_TEAM_VICTORY_POINTS + 1;
              }
              else if(TEAM == 4){
                //score one for orange team
                ORANGE_TEAM_VICTORY_POINTS = ORANGE_TEAM_VICTORY_POINTS + 1;
              }
              else if(TEAM == 5){
                //score one for blue team
                MAGENTA_TEAM_VICTORY_POINTS = MAGENTA_TEAM_VICTORY_POINTS + 1;
              }

            //if anyone has enough points to win, display it
            if(RED_TEAM_VICTORY_POINTS >= POINTS_TO_WIN || BLUE_TEAM_VICTORY_POINTS >= POINTS_TO_WIN ||
               ORANGE_TEAM_VICTORY_POINTS >= POINTS_TO_WIN || MAGENTA_TEAM_VICTORY_POINTS >= POINTS_TO_WIN){
                  //if you have won, show it
                  update_display(4, 0);  //update display with victory
                  //make startup sound for a long time
                  for (int i = 2000; i < 5000; i++){
                         tone(piezo_pin, i);
                         delay(50);
                         noTone(piezo_pin);
                         i = i + 49;
                       }
                  change_RGB(-1);
            }
            else{
              update_display(0, 0);  //update display normally
            }
            //update passive for next time
            PASSIVE_ABILITY_TIME = current_time + POINT_GAIN_DELAY;
          }
        }
        break;
      case 4:
        //if I'm a pyro, and I have extra health for a short time if you have that enabled
        #if PYRO_PASSIVE_EXTRA_HEALTH
        if(FIRE_DEATH_STACKS > 0){
          //check to see if I'm out of time
          if(current_time > PASSIVE_ABILITY_TIME){
            //if I'm out of time, reset health and subtract all stacks
            FIRE_DEATH_STACKS = 0;
            MAX_HEALTH = FIRE_MAX_HEALTH + FIRE_MAX_HEALTH_PER_STACK*FIRE_DEATH_STACKS;
            if(HEALTH > MAX_HEALTH) HEALTH = MAX_HEALTH;   //if current health is above max, lower to max
            update_display(0, 0);                             //show on display
            //PASSIVE_ABILITY_TIME = current_time + FIRE_DEATH_DELAY;
          }
        }
        #endif
        break;
      case 7:
        //I'm rock. I get rock stacks every few seconds if you have that enabled
        #if ROCK_PASSIVE_ARMOR
        if(current_time > PASSIVE_ABILITY_TIME){
          if(ROCK_STACKS < MAX_ROCK_STACKS){
            //if I'm not at full health
            take_damage(7, 0, 0, DEVICE_ID, 0);
          }
          PASSIVE_ABILITY_TIME = current_time + PASSIVE_ABILITY_COOLDOWN;
        }
        #endif
        break;
      default:
        break;
    }
  }
  else{
    //if you have no health, check if you can automatically revive
    if(AUTO_REVIVE > 0){
      /*
       * static byte last_check;                 //for checking when you can revive
        byte current_check = 0;                 //for checking when you can revive
       */
      current_check = (AUTO_REVIVE_TIME - current_time)/1000;
      if(AUTO_REVIVE_TIME < current_time){
        //if it's past the time you can be revived, then be revived (at full health, or 10 bonus damage)
        take_damage(3, 0, 10, DEVICE_ID, 0);
      }
      else if(current_check != last_check){
        //if the number of seconds to revive has changed, update display and update time
        last_check = current_check;
        update_display(3, -1);  //update display that you are out, keeping old person who tagged you out.
      }
    }
  }
  
  //check all statuses 
  if(FIRE == 1){
    if(current_time > FIRE_TIME){
      FIRE = 0;
      //update display normally
      update_display(0, 0);
      if(SERIAL_DEBUG) Serial.println(F("Fire off"));
    }
  }
  if (ICE_STACKS > 0){
    if(current_time > ICE_TIME){
      ICE_STACKS = ICE_STACKS - 1;
      if (ICE_STACKS > 0){
        //if I have more stacks, reset timer
        ICE_TIME = millis() + ICE_DELAY;
      }
      //update display normally
      if(SERIAL_DEBUG){
        Serial.print(F("Ice Stacks: "));
        Serial.println(ICE_STACKS);
      }
      update_display(0, 0);
    }
  }
  if(POISON_STACKS > 0){
    if(current_time > POISON_TIME){
      POISON_STACKS = POISON_STACKS - 1;
      take_damage(10, 0, 0, POISON_DEVICE, 0);     //10 means take poison damage, use the device id that hit you
      if (POISON_STACKS > 0){
        //if I have more stacks, reset timer
        POISON_TIME = millis() + POISON_DELAY;
      }
      if(SERIAL_DEBUG) Serial.println(F("Poison Damage"));
    }
  }
  if(WATER > 0){
    if(current_time > WATER_TIME){
      WATER = 0;
      //update display normally
      update_display(0, 0);
      if(SERIAL_DEBUG) Serial.println(F("Water off"));
    }
  }
  if(LIGHTNING_STACKS > 0){
    if(current_time > LIGHTNING_TIME){
      LIGHTNING_STACKS = LIGHTNING_STACKS - 1;
      if (LIGHTNING_STACKS > 0){
        //if I have more stacks, reset timer
        LIGHTNING_TIME = millis() + LIGHTNING_DELAY;
      }
      else{
        //if I'm out of stacks, don't start regenerating mana for normal delay
        MANA_TIME = current_time + MANA_DELAY;
      }
      //update display normally
      update_display(0, 0);
      if(SERIAL_DEBUG) Serial.println(F("Lightning Removed"));
    }
  }
  
  //check if you need to display any other status on RGB_LED's
  //don't do this for bases, as it's confusing
  if (DISPLAY_STATUS_TIME < current_time && HEALTH > 0 && CHARACTER_CLASS != 3){
    //if it's time to check statuses, see what to do
    if(DISPLAY_CHECK == 0){
      //if DISPLAY_CHECK == 0, be yourself
          #if SHOW_CLASS_CONSTANTLY
            //if you have set to show class, do so
            change_RGB(CHARACTER_CLASS);
          #else
            //don't show class, and if you have no statuses, turn off
            if( FIRE == 0 && ICE_STACKS == 0 && POISON_STACKS == 0 && WATER == 0 &&
                LIGHTNING_STACKS == 0){
                  change_RGB(-1);
                }
          #endif
      //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_CLASS_DELAY;
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;  //can't use ++, since this is a byte
    }
    else if(DISPLAY_CHECK == 1){
      //if it's 1, I'm checking if I'm on fire
      if(FIRE > 0){
        //I'm on fire, so show fire
        change_RGB(4);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;
    }
    else if(DISPLAY_CHECK == 2){
      //if it's 2, I'm checking if I have ice
      if(ICE_STACKS > 0){
        //I'm frozen, so show ice
        change_RGB(5);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check
      //skipping poison for now, as you see poison when the poison takes effect
      DISPLAY_CHECK = DISPLAY_CHECK + 2;
    }
    else if(DISPLAY_CHECK == 3){
      //if it's 3, I'm checking if I am poisoned
      if(POISON_STACKS > 0){
        //I'm poisoned. show poison
        change_RGB(6);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;
    }
    else if(DISPLAY_CHECK == 4){
      //if it's 4, I'm checking if I have rock armor
      if(ROCK_STACKS > 0){
        //I have rock armor. show it
        change_RGB(7);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;
    }
    else if(DISPLAY_CHECK == 5){
      //if it's 5, I'm checking if I am wet
      if(WATER > 0){
        //I am wet. show it
        change_RGB(8);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;
    }
    else if(DISPLAY_CHECK == 6){
      //if it's 6, I'm checking if I am electrified
      if(LIGHTNING_STACKS > 0){
        //I am electrified. show it
        change_RGB(9);
        //and set next check time to some delay in the future
      DISPLAY_STATUS_TIME = DISPLAY_STATUS_TIME + DISPLAY_STATUS_DELAY;
      }
      //and set the next thing to check as 1
      DISPLAY_CHECK = DISPLAY_CHECK + 1;
    }
    else{
      //if it's no status you know, go back to displaying yourself
      DISPLAY_CHECK = 0;
    }
  }
}

/*****
 * take_damage f(x)
 * The take damage function takes player damage. It takes in the tag that the player
 * was hit with and applies all buffs, debuffs, etc.
 * input: received_tag (int) - the tag you received
 * output: nothing. Changing health and status
******/
void take_damage( int tag, int tagger_team, int bonus_damage, int sent_device, unsigned long sum_of_tag ){
  //start off by taking no damage
  int damage = 0;
  int armor_damage = 0;
  byte telegraph_rock = 0;  //track if you should show you had rock armor
  byte telegraph_tag = 0;   //track if you should telegraph the tag

  //store what health currently is to see if you die due to this tag
  static int previous_health;
  previous_health = HEALTH;

  //if you are hit by the controller
  if(sent_device == CONTROLLER_DEVICE_ID && tag <= NUMBER_OF_CLASSES){
    //you are hit by the controller. Set your team and class to what hit you
    respawn(tag, tagger_team, 10, sent_device);   //respawn using sent class and team, with 100% health
    telegraph_tag = 3;                             //telegraph revive
    RUNE_SENT = 0;                                 //reset rune
    KING_OF_THE_HILL = 0;                          //reset king of the hill
  }
  
  if(sent_device != CONTROLLER_DEVICE_ID || tag > NUMBER_OF_CLASSES){
    //if you are not hit by a controller, or are hit by controller for a rune, do the normal tag thing
    switch (tag) {
      case 0:
        //I'm hit by double damage tag, take 2 damage
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          damage = DOUBLE_DAMAGE;
          if(SERIAL_DEBUG) Serial.println(F("hit DD"));
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit DD by my team"));
        }
        break;
      case 1:
        //I'm hit by a normal tag, take normal damage
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          damage = NORMAL_DAMAGE;
          if(SERIAL_DEBUG) Serial.println(F("hit normal tag"));
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit normal by my team"));
        }
        break;
      case 2:
        //I'm hit by a heal. Check health and heal if it's from my team or free for all team
        if(tagger_team == 0 || TEAM == tagger_team){
          if(WATER == 0){
            //I can only be healed if I am not wet
            if(HEALTH < MAX_HEALTH && HEALTH > 0){
              telegraph_tag = 1;
              HEALTH = HEALTH + HEALING_DAMAGE;
              if(HEALTH > MAX_HEALTH) HEALTH = MAX_HEALTH;
              if(SERIAL_DEBUG) Serial.println(F("healed"));
            }
            if(POISON_STACKS > 0){
              //if you are poisoned, set it to 1, and set POISON_TIME to now, so check_passives clears it
              POISON_STACKS = 1;
              POISON_TIME = millis();
            }
          }
          else{
              telegraph_rock = 1;  //show rock instead of the tag you were hit by
              if(SERIAL_DEBUG) Serial.println(F("heale while wet. No effect"));
          }
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("healed from enemy team"));
        }
        break;
      case 3:
        //if I'm hit by revive. Check health and revive if it's from my team or free for all team
        if(tagger_team == 0 || TEAM == tagger_team){
          if (HEALTH < 1){
            if(WATER == 0){
              //I can only be revived if I have less than 1 health and am not wet
              wireless_transceiver(sent_device, 6, DEVICE_ID, sum_of_tag);  //send message that you are alive
              //check how to change class when reviving
              if(SURVIVAL_MODE > 0){
                //if you're in survival mode, you always come back as a pyro on purple team
                respawn(4, SURVIVAL_TEAM, 10, 0);                      //respawn as a pyro on purple team with 100% health
              }
              else{
                //if you are not playing survival mode, respawn regularly
                if(sent_device < MIN_BASE_ID){
                  //if you are respawning from a player, use their bonus damage for what % of health you get
                  respawn(0, TEAM, bonus_damage, sent_device);              //respawn allowing class changes
                }
                else{
                  //you are respawning from a base. Get 100% health
                  respawn(0, TEAM, 10, sent_device);              //respawn allowing class changes
                }
              }

            telegraph_tag = 0;               //don't telegraph revive. Class change functions do that
            }
            else{
              //if you are wet, don't get revived, and telegraph the water
              telegraph_tag = 8;  //telegraph water hit
              //check_passives(8, 0);  //reset water duration
            }
            
          }
          if(SERIAL_DEBUG) Serial.println(F("revived"));
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("revived from enemy team"));
          if(ROCK_STACKS > 0){
            armor_damage = REVIVE_DAMAGE;
            telegraph_rock = 1;  //show rock instead of the tag you were hit by
          }
          else{
            if( POISON_STACKS > 0){
                  //if you are poisoned, necromantic energy does more damage
                  damage = REVIVE_POISON_DAMAGE;
                }
                else{
                  damage = REVIVE_DAMAGE;
                }
                telegraph_tag = 2;  //telegraph necromancer hit
            }
          }
        break;
      case 4:
        //I'm hit by pyro. Take 1 damage, set on fire flag, set passive
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          if(ROCK_STACKS > 0){
            armor_damage = FIRE_DAMAGE;
            telegraph_rock = 1;  //show rock instead of the tag you were hit by
          }
          else{
            if(WATER > 0) WATER_TIME = millis();    //fire removes water
            if(CHARACTER_CLASS != 8){
              FIRE = 1;                             //water class can't be lit on fire
              check_passives(4, bonus_damage);
              if(CHARACTER_CLASS == 5){
                //ice takes double damage from fire
                damage = FIRE_DAMAGE*2;
              }
              else{
                //otherwise, take normal damage from fire
                damage = FIRE_DAMAGE;
              }
              
            }
            else{
              telegraph_tag = 8;                   //telegraph you hit water, not fire
            }
          }
          if(SERIAL_DEBUG) Serial.println(F("hit with fire"));
        }
        else{
          if(WATER > 0) WATER_TIME = millis();    //fire removes water
          if(SERIAL_DEBUG) Serial.println(F("hit with fire from my team"));
        }
        break;
      case 5:
        //I'm hit by ice. Take 0 damage, set ice stacks, set passive
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          if(ROCK_STACKS > 0){
            armor_damage = ICE_DAMAGE;
            telegraph_rock = 1;  //show rock instead of the tag you were hit by
          }
          else{
            if(WATER > 0){
              if(ICE_STACKS < MAX_ICE_WATER_STACKS){
                ICE_STACKS = ICE_STACKS + 1;
              }
              if(CHARACTER_CLASS == 4){
                //pyro takes double damage from ice
                damage = ICE_WATER_DAMAGE*2;
              }
              else{
                damage = ICE_WATER_DAMAGE;
              }
            }
            else{
              if(ICE_STACKS < MAX_ICE_STACKS){
                ICE_STACKS = ICE_STACKS + 1;
              }
              else{
                if(CHARACTER_CLASS == 4){
                  //pyro takes double damage from ice
                  damage = ICE_DAMAGE*2;
                }
                else{
                  damage = ICE_DAMAGE;
                }
              }
            }
            check_passives(5, bonus_damage);
          }
          if(SERIAL_DEBUG) Serial.print(F("hit with ice. Stacks: "));
          if(SERIAL_DEBUG) Serial.println(ICE_STACKS);
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit with ice from my team"));
        }
        break;
      case 6:
        //I'm hit with Poison. Add poison stacks. Do no damage
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          POISON_DEVICE = sent_device;    //set who hit you with poison
          BONUS_DAMAGE = bonus_damage;    //save bonus damage for when the poison hits you
          //medic is immune to poison
          if(CHARACTER_CLASS != 2){
            if(ROCK_STACKS > 0){
              armor_damage = POISON_PER_HIT*POISON_DAMAGE;
              telegraph_rock = 1;  //show rock instead of the tag you were hit by
            }
            else{
              if(POISON_STACKS < MAX_POISON_STACKS) POISON_STACKS = POISON_STACKS + POISON_PER_HIT;
              check_passives(6, bonus_damage);
            }
          }
          else{
            //show medic, that you are immune to poison
            telegraph_tag = 6;    
          }
          if(SERIAL_DEBUG){
            Serial.print(F("hit with Poison. Stacks: "));
            Serial.println(POISON_STACKS);
          }
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit with poison from my team"));
        }
        break;
      case 7:
        //I'm hit with rock. Add 1 to rock armor if from my team or free for all team
        if(tagger_team == 0 || TEAM == tagger_team){
          telegraph_tag = 1; 
          if(ROCK_STACKS < MAX_ROCK_STACKS){
            ROCK_STACKS = ROCK_STACKS + ROCK_ARMOR_DAMAGE;
            if(ROCK_STACKS > MAX_ROCK_STACKS) ROCK_STACKS = MAX_ROCK_STACKS;
          }
          //remove debuffs
          /*
          if(FIRE > 0) FIRE_TIME = millis();
          if(ICE_STACKS > 0){
            ICE_TIME = millis();
            ICE_STACKS = 1;
          }
          if(POISON_STACKS > 0){
            POISON_STACKS = 1;
            POISON_TIME = millis();
          }
          if(WATER > 0) WATER_TIME = millis();
          */
          //for now, Rock only purges lightning
          if(LIGHTNING_STACKS > 0){
            LIGHTNING_STACKS = 1;
            LIGHTNING_TIME = millis();
          }
          //add mana
          if(CHARACTER_CLASS != 3){
            //bases can't get extra mana from rock
            MANA = MANA + ROCK_MANA;
            if(MANA > (MAX_ROCK_EXTRA_MANA + MAX_MANA)) MANA = MAX_ROCK_EXTRA_MANA + MAX_MANA;
          }
          if(SERIAL_DEBUG) Serial.print(F("Hit with Rock. Stacks: "));
          if(SERIAL_DEBUG) Serial.println(ROCK_STACKS);
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit with rock from enemy team"));
        }
        break;
      case 8:
        //I'm hit by water. set passive
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          if(ROCK_STACKS > 0){
            armor_damage = WATER_ARMOR_DAMAGE;
            telegraph_rock = 1;  //show rock instead of the tag you were hit by
          }
          else{
            if(FIRE > 0) FIRE_TIME = millis();   //water removes fire
            WATER = 1;
            check_passives(8, bonus_damage);
          }
          if(SERIAL_DEBUG) Serial.println(F("hit with water"));
        }
        else{
          if(FIRE > 0) FIRE_TIME = millis();   //water removes fire
          if(SERIAL_DEBUG) Serial.println(F("hit with water from my team"));
        }
        break;
      case 9:
        //I'm hit by lightning. set passive and take damage if at max stacks
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          if(ROCK_STACKS > 0){
            //do nothing. 
            armor_damage = LIGHTNING_ARMOR_DAMAGE;
            telegraph_rock = 1;  //show rock instead of the tag you were hit by
          }
          else{
            //if you have fewer than max stacks, add a stack
            if(LIGHTNING_STACKS < MAX_LIGHTNING_STACKS){
              if(WATER > 0 || CHARACTER_CLASS == 8){
                //If the person is wet, or if theya re the water class they get full lightning stacks and take more damage
                LIGHTNING_STACKS = MAX_LIGHTNING_STACKS;
                damage = LIGHTNING_WATER_DAMAGE;
              }
              else{
                //otherwise, add 1 stack
                LIGHTNING_STACKS = LIGHTNING_STACKS + 1;
              }
            }
            else if(LIGHTNING_STACKS == MAX_LIGHTNING_STACKS){
              //if you are at max stacks, take damage
              damage = LIGHTNING_DAMAGE;
            }
            //now, remove mana from player
            MANA = MANA - LIGHTNING_MULTIPLIER*LIGHTNING_STACKS;
            if(MANA < 0) MANA = 0;
    
            //set lightning time
            check_passives(9, bonus_damage);
          }
          if(SERIAL_DEBUG) Serial.print(F("hit with Lightning. Stacks: "));
          if(SERIAL_DEBUG) Serial.println(LIGHTNING_STACKS);
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit with lightning from my team"));
        }
        break;
      case 10:
        //I'm taking passive poison damage
        damage = POISON_DAMAGE;
        bonus_damage = BONUS_DAMAGE;    //use saved bonus damage
        telegraph_tag = 2;  //telegraph necromancer hit
        break;
      case 11:
        //I'm hit by triple damage tag, take triple damage
        //Works if tag is from free for all team or enemy team
        if(tagger_team == 0 || TEAM != tagger_team){
          telegraph_tag = 1;
          damage = TRIPLE_DAMAGE;
          if(SERIAL_DEBUG) Serial.println(F("hit by triple damage tag"));
        }
        else{
          if(SERIAL_DEBUG) Serial.println(F("hit by triple damage from my team"));
        }
        break;
      ////****rune and game mechanics changing tags ///
      case 17:
        //I'm hit with an turn on / off auto-revive mode tag.
        if(AUTO_REVIVE > 0){
          //if auto-revive mode is not 0, it's already on. Turn it back off by setting it to 0
          AUTO_REVIVE = 0;
          wireless_transceiver(4, 12, DEVICE_ID, sum_of_tag);  //send message that auto-revive mode is off
          play_sound(out_sound, out_sound_delay, out_sound_length);
          
        }
        else{
          AUTO_REVIVE = 1;
          wireless_transceiver(3, 12, DEVICE_ID, sum_of_tag);  //send message that auto-revive mode is on
          play_sound(startup_sound, startup_sound_delay, startup_sound_length);
        }
        if(SERIAL_DEBUG) Serial.println(F("Received on/off auto revive mode"));
        break;
      case 18:
        //I'm hit with an turn on / off survival mode tag.
        if(SURVIVAL_MODE > 0){
          //if survival mode is not 0, set it to 1 to turn it on. Otherwise, turn it back off by setting it to 0
          SURVIVAL_MODE = 0;
          wireless_transceiver(2, 12, DEVICE_ID, sum_of_tag);  //send message that survival mode is off

          //if you turned on automatically reviving in survival mode
          #if AUTO_REVIVE_SURVIVAL
          AUTO_REVIVE = 0;
          #endif
          
          play_sound(out_sound, out_sound_delay, out_sound_length);
        }
        else{
          SURVIVAL_MODE = 1;
          wireless_transceiver(1, 12, DEVICE_ID, sum_of_tag);  //send message that survival mode is on

          //if you turned on automatically reviving in survival mode
          #if AUTO_REVIVE_SURVIVAL
          AUTO_REVIVE = 1;
          #endif
          
          play_sound(startup_sound, startup_sound_delay, startup_sound_length);
        }
        if(SERIAL_DEBUG) Serial.println(F("Received on/off survival mode"));
        break;
      case 19:
        //I'm hit with an turn on / off aim assist.
        if(ENABLE_AIM_ASSIST > 0){
          ENABLE_AIM_ASSIST = 0;
          wireless_transceiver(8, 12, DEVICE_ID, sum_of_tag);  //send message that aim assist mode is off
          play_sound(out_sound, out_sound_delay, out_sound_length);
        }
        else{
          ENABLE_AIM_ASSIST = 1;
          wireless_transceiver(7, 12, DEVICE_ID, sum_of_tag);  //send message that aim assist mode is on
          play_sound(startup_sound, startup_sound_delay, startup_sound_length);
        }
        if(SERIAL_DEBUG) Serial.println(F("Received on/off aim assist"));
        break;
      case 20:
        //I'm hit with an turn on / off ammo tag. Turn on / off automatic mana regeneration and using ammo for tags
        if(AUTO_RESPAWN_MANA > 0){
          //if automatic mana respawn is not 0, you are respawning mana. turn it off and turn on ammo mode
          AUTO_RESPAWN_MANA = 0;
          USE_AMMO = 1;
          wireless_transceiver(5, 12, DEVICE_ID, sum_of_tag);  //send message that use ammo mode is on
          //update the mana and mana multiplier
          MANA = MANA*AMMO_MANA_MULTIPLIER;
          MAX_MANA = MAX_MANA*AMMO_MANA_MULTIPLIER;
        }
        else{
          AUTO_RESPAWN_MANA = 1;
          USE_AMMO = 0;
          wireless_transceiver(6, 12, DEVICE_ID, sum_of_tag);  //send message that use ammo mode is off
          //get rid of the extra mana you got for using ammo
          MAX_MANA = MAX_MANA/AMMO_MANA_MULTIPLIER;
          MANA = MAX_MANA;
        }
        update_display(0, 0);   //update display
        if(SERIAL_DEBUG) Serial.println(F("Received on/off ammo tag"));
        break;
      case 21:
        //I'm hit with an aiming check tag. Respond back via radio that you've been hit with this tag 
        //int tag, int tagger_team, int bonus_damage,
        wireless_transceiver(sent_device, 9, DEVICE_ID, sum_of_tag);
        if(SERIAL_DEBUG) Serial.println(F("Received aim check tag"));
        break;
      case 22:
        //I'm hit with an aiming check tag. Respond back via radio that you've been hit with this tag
        wireless_transceiver(sent_device, 8, DEVICE_ID, sum_of_tag);
        if(SERIAL_DEBUG) Serial.println(F("Received aim check tag"));
        break;
      case 23:
        //I'm hit with king of the hill rune. Do nothing.
        if(SERIAL_DEBUG) Serial.println(F("Received King of the Hill Rune"));
        break;
      case 24:
        //this is reserved for a rune in the future
        
        break;
      case 25:
        //I'm hit with change_class rune. change class
        wireless_transceiver(sent_device, 7, DEVICE_ID, sum_of_tag);  //send message that you got change class rune
        respawn(CLASS_CHANGED_TO, TEAM, 100, sent_device);       //respawn using current class and team, with 100% health
        if(SERIAL_DEBUG) Serial.println(F("Hit with Change Class Rune"));
        break;
      case 26:
        //I'm hit with an ammo tag. Get 50% of max ammo and mana
        wireless_transceiver(14, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
        if(USE_AMMO > 0){
          //if using ammo, then give 50% of max ammo and mana
          MANA = MANA + (MAX_MANA / 2);
          if(MANA > MAX_MANA) MANA = MAX_MANA;

          AMMO = AMMO + (MAX_AMMO / 2);
          if(AMMO > MAX_AMMO) AMMO = MAX_AMMO;
        }
        update_display(0, 0);   //update display
        telegraph_tag = 3;
        if(SERIAL_DEBUG) Serial.println(F("Received ammo tag"));
        break;
      case 27:
        //I'm hit with extra life rune. Add one to lives
        wireless_transceiver(13, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
        LIVES = LIVES + 1;
        if(SERIAL_DEBUG) Serial.println(F("Received Extra Life Rune"));
        telegraph_tag = 1;
        break;
      case 28:
        //I'm hit with double armor rune. Double max armor and set armor to new max
        wireless_transceiver(12, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
          if(DOUBLE_ARMOR_RUNE == 0){
            MAX_ROCK_STACKS = DOUBLE_ARMOR_MAX_ROCK_STACKS;
            DOUBLE_ARMOR_RUNE = 1;
          }
          //set to max armor
          ROCK_STACKS = MAX_ROCK_STACKS;
        if(SERIAL_DEBUG) Serial.println(F("Received Double Armor Rune"));
        telegraph_tag = 1;
        break;
      case 29:
        //I'm hit with a arcane mana rune. Add upgraded abilities
        wireless_transceiver(11, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
          if(ARCANE_RUNE == 0){
            if(CHARACTER_CLASS == 8 || CHARACTER_CLASS == 10){
              ARCANE_RUNE = ARCANE_RUNE_ATTACKS_LOW;
            }
            else if(CHARACTER_CLASS == 9){
              ARCANE_RUNE = ARCANE_RUNE_ATTACKS_HIGH;
            }
            else{
              ARCANE_RUNE = ARCANE_RUNE_ATTACKS_NORMAL;
            }
            
          }
          //give yourself full mana
          MANA = MAX_MANA;
       
        if(SERIAL_DEBUG) Serial.println(F("Received Double Mana Rune"));
        telegraph_tag = 1;
        break;
      case 30:
        //I'm hit with a double health rune. Double current and max health
        wireless_transceiver(10, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
        //Double health
        if(DOUBLE_HEALTH_RUNE == 0){
          //if I haven't been hit with double health already, double my health
          MAX_HEALTH = MAX_HEALTH*2;
          DOUBLE_HEALTH_RUNE = 1;
        }
        //set health to max
        HEALTH = MAX_HEALTH;

        if(SERIAL_DEBUG) Serial.println(F("Received Double Health Rune"));
        telegraph_tag = 1;
        break;
      case 31:
        //I'm hit with a double damage rune. Double main tag damage
        wireless_transceiver(9, 12, DEVICE_ID, sum_of_tag);  //send message for this rune
        //Add stacks (number of shots times you have double damage)
        DOUBLE_DAMAGE_RUNE = DOUBLE_DAMAGE_RUNE + DOUBLE_DAMAGE_RUNE_ATTACKS;

        if(SERIAL_DEBUG) Serial.println(F("Received Double Damage Rune"));
        telegraph_tag = 1;
        break;
      default:
        //if I can't tell, do nothing
        if(SERIAL_DEBUG) Serial.println(F("don't know what hit me"));
        break;
    }
  }

  //check if you are subtracking armor
  if(armor_damage > 0){
    //add damage bonuses (every 1 of damage bonus is a 10% bonus)
    if(bonus_damage > 0) armor_damage = (armor_damage*(10 + bonus_damage))/10;
    
    ROCK_STACKS = ROCK_STACKS - armor_damage;
    if(ROCK_STACKS < 0) ROCK_STACKS = 0;            //ensure you don't go negative
  }

  //check if you are on fire
  if(FIRE == 1){
    if(ICE_STACKS > 0){
      //if they have ice stacks, and they also are on fire, they explode
      damage = damage + ICE_STACKS*ICE_EXPLOSION_MULTIPLIER;
      //reset fire and ice times, because this clears that status
      FIRE_TIME = millis();
      ICE_TIME = FIRE_TIME;
      ICE_STACKS = 1;       //reset this to 1, so that when you check passives, you set it to 0.
    }
    //you're on fire, so you take double damage
    damage = damage*2;
  }

  //add damage bonuses (every 1 of damage bonus is a 10% bonus)
  if(bonus_damage > 0) damage = (damage*(10 + bonus_damage))/10;
  
  //now, take damage
  if(CHARACTER_CLASS == 11){
    //if you are the juggernaut, damage goes to mana first, and overflow goes to health
    MANA = MANA - damage;
    //dont regenerate mana for a short time
    MANA_TIME = millis() + JUG_HIT_MANA_DELAY;
    if(MANA < 0){
      //if mana is negative, add it to health
      HEALTH = HEALTH + MANA;
      MANA = 0;
    }
  }
  else{
    HEALTH = HEALTH - damage;
  }

  //now, check for auto-reviving
  if(AUTO_REVIVE > 0 && HEALTH < 0){
    //if auto revive is on, and you've taken damage from anyone, you don't respawn for a bit
     AUTO_REVIVE_TIME = millis() + AUTO_REVIVE_TIME_OUT;
     POISON_STACKS = 0;      //and make sure you can't be hit infinitely by poison. That's not cool.
  }
  

  //if a base takes damage, it can't send tags for a little bit
  if(HEALTH < previous_health && CHARACTER_CLASS == 3){
    //if you want to play where runes can be contested, any base that is tagged can't send
    //a tag for a little bit. If you don't, runes will not have this delay
    #if CONTEST_RUNES
      cooldown_time = millis() + HIT_BASE_COOLDOWN_TIME;
    #else
      if(RUNE_SENT == 0) cooldown_time = millis() + HIT_BASE_COOLDOWN_TIME;
    #endif
  }

  //see if I am now out because of that tag
  if (HEALTH < 1 && previous_health > 0){
    if (LIVES > 0){
      //if you have lives left, restart
      #if ATMEGA4809_TRUE
        play_mp3_sound(OTHER_EVENT, OUT_SOUND_NUM);
        delay(1250);
      #else
        for(int i = 0; i < out_sound_length; i++){
          tone(piezo_pin, out_sound[i], out_sound_delay[i]);
          delay(out_sound_delay[i]);
        }
        noTone(piezo_pin);
      #endif
      
      LIVES = LIVES - 1;
      #if LIVES_CAN_CHANGE_CLASS                         //if you're allowed to change classes
      respawn(0, TEAM, 100, sent_device);                //let you change classes
      #else                                              //if not
      respawn(CHARACTER_CLASS, TEAM, 100, sent_device);  //respawn using current class and team
      #endif
      telegraph_hit( 3 );                 //telegraph the revive
      //delay(RESPAWN_TIME);
      //update_display(0, 0);    
    }
    else{
      //if this last tag got you out, indicate you are out
      update_display(3, sent_device);                //invert display and say who got you out
      if(CHARACTER_CLASS == 11){
        //if you were the juggernaut, let everyone know the juggernaut is out
        wireless_transceiver(sent_device, 4, DEVICE_ID, sum_of_tag);  //send message that you are out and were juggernaut
      }
      else{
        //otherwise, send a normal message that you are out
        wireless_transceiver(sent_device, 3, DEVICE_ID, sum_of_tag);  //send message that you are out
      }
      #if ATMEGA4809_TRUE
        play_mp3_sound(OTHER_EVENT, OUT_SOUND_NUM);
        for(int i = NUMBER_OF_CLASSES; i > 0; i--){  
          change_RGB(i);                               //change RGB's
          delay(OUT_COLOR_CHANGE_TIME);
        }
      #else
        for(int i = NUMBER_OF_CLASSES; i > 0; i--){  
          tone(piezo_pin, (500*i));                    //make a sound
          change_RGB(i);                               //change RGB's
          delay(OUT_COLOR_CHANGE_TIME);
        }
        noTone(piezo_pin);
      #endif
        
      change_RGB(-1);    //turn RGB's off

      //however, certain classes have special features. Check for them
      if(CHARACTER_CLASS == 3){
        //wait 3 seconds to show that you are out
        //if I'm a base, then I've been captured. Switch teams to who tagged me
        TEAM = tagger_team;

        //reset health, change RGB color, and update display
        HEALTH = MAX_HEALTH;
        change_RGB(3);
        update_display(0, 0);
        //clear poison stacks
        POISON_STACKS = 0;
        
      }
      else if (CHARACTER_CLASS == 4){
        //if you are a pyro add 1 to death stacks, and reset time
        if((SURVIVAL_MODE > 0) && (TEAM == SURVIVAL_TEAM)){
          //if you are playing survival mode and are on the survival team, use a different counter, 
          if( FIRE_DEATH_STACKS < SURVIVE_MAX_FIRE_DEATH_STACKS) FIRE_DEATH_STACKS = FIRE_DEATH_STACKS + 1;
        }
        else{
          //otherwise, use normal 
          if( FIRE_DEATH_STACKS < MAX_FIRE_DEATH_STACKS) FIRE_DEATH_STACKS = FIRE_DEATH_STACKS + 1;
        }
        
        PASSIVE_ABILITY_TIME = FIRE_DEATH_DELAY + millis();
      }

      //now, check to see if you were tagged out by someone on a zombie team
      if(tagger_team == ZOMBIE_TEAM_1){
        //if you are out from someone on the zombie team 1, you are now on that team
        TEAM = tagger_team;
        ZOMBIE_GOT_ME = tagger_team;                //remember which team you stay on when you try to switch classes
        #if REVIVE_ZOMBIE_WHEN_OUT                  //if you've set to revive when you are out from zombie team
          #if ZOMBIE_REVIVES_CHANGE_CLASS           //if zombies can change class
          respawn(0, TEAM, 100, 0);                      //allow class changes
          #else                                     
          respawn(CHARACTER_CLASS, TEAM, 100, 0);        //respawn using current class and team
          #endif
        delay(RESPAWN_TIME);                        //wait for a few seconds
        telegraph_hit( 3 );                         //telegraph the revive
        #else
        update_display(2, 0);                       //show your new team
        play_sound(out_sound, out_sound_delay, out_sound_length);
        #endif
      }
      else if(tagger_team == ZOMBIE_TEAM_2){
        //if you are out from someone on the zombie team 2, you are now on that team
        TEAM = tagger_team;
        ZOMBIE_GOT_ME = tagger_team;                //remember which team you stay on when you try to switch classes
        #if REVIVE_ZOMBIE_WHEN_OUT                  //if you've set to revive when you are out from zombie team
          #if ZOMBIE_REVIVES_CHANGE_CLASS           //if zombies can change class
          respawn(0, TEAM, 100, 0);                      //allow class changes
          #else                                     
          respawn(CHARACTER_CLASS, TEAM, 100, 0);        //respawn using current class and team
          #endif
        delay(RESPAWN_TIME);                        //wait for a few seconds
        telegraph_hit( 3 );                         //telegraph the revive
        #else
        update_display(2, 0);                       //show your new team
        play_sound(out_sound, out_sound_delay, out_sound_length);
        #endif
      }
      else if(tagger_team == ZOMBIE_TEAM_3){
        //if you are out from someone on the zombie team 3, you are now on that team
        TEAM = tagger_team;
        ZOMBIE_GOT_ME = tagger_team;                //remember which team you stay on when you try to switch classes
        #if REVIVE_ZOMBIE_WHEN_OUT                  //if you've set to revive when you are out from zombie team
          #if ZOMBIE_REVIVES_CHANGE_CLASS           //if zombies can change class
          respawn(0, TEAM, 100, 0);                      //allow class changes
          #else                                     
          respawn(CHARACTER_CLASS, TEAM, 100, 0);        //respawn using current class and team
          #endif
        delay(RESPAWN_TIME);                        //wait for a few seconds
        telegraph_hit( 3 );                         //telegraph the revive
        #else
        update_display(2, 0);                       //show your new team
        play_sound(out_sound, out_sound_delay, out_sound_length);
        #endif
      }
      else if(tagger_team == ZOMBIE_TEAM_4){
        //if you are out from someone on the zombie team 4, you are now on that team
        TEAM = tagger_team;
        ZOMBIE_GOT_ME = tagger_team;                //remember which team you stay on when you try to switch classes
        #if REVIVE_ZOMBIE_WHEN_OUT                  //if you've set to revive when you are out from zombie team
          #if ZOMBIE_REVIVES_CHANGE_CLASS           //if zombies can change class
          respawn(0, TEAM, 100, 0);                      //allow class changes
          #else                                     
          respawn(CHARACTER_CLASS, TEAM, 100, 0);        //respawn using current class and team
          #endif
        delay(RESPAWN_TIME);                        //wait for a few seconds
        telegraph_hit( 3 );                         //telegraph the revive
        #else
        update_display(2, 0);                       //show your new team
        play_sound(out_sound, out_sound_delay, out_sound_length);
        #endif
      }
      else{
        //you are out, from someone not on a zombie team. You don't need to play another sound
        //play_sound(out_sound, out_sound_delay, out_sound_length);
      }
    }
  }
  else{
    //if you're not out, blink LED's to show that you've been tagged
    if(telegraph_rock > 0){
      //show rock instead of the tag you were hit by
      telegraph_hit( 7 );
      //send a transmission, to who tagged me, that they hit my armor, so long as I'm alive and it wasn't me
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 2, DEVICE_ID, sum_of_tag);
    }
    else if (telegraph_tag == 1){
      //if I'm hit by an enemy and should show it, do so.
      telegraph_hit( tag );
      //send a transmission, to who tagged me, that they tagged me
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 1, DEVICE_ID, sum_of_tag);
    }
    else if (telegraph_tag == 2){
      //I'm hit by damaging necromancer. Show it
      telegraph_hit( 10 );
      //send a transmission, to who tagged me, that they tagged me
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 1, DEVICE_ID, sum_of_tag);
    }
    else if (telegraph_tag == 3){
      //I'm hit by revive. Show it
      telegraph_hit( 3 );
      //send a transmission, to who tagged me, that they tagged me
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 1, DEVICE_ID, sum_of_tag);
    }
    else if (telegraph_tag == 6){
      //I'm hit by poison, but I'm medic class (immune). Show medic.
      telegraph_hit( 2 );
      //send a transmission, to who tagged me, that they hit my armor
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 2, DEVICE_ID, sum_of_tag);
    }
    else if (telegraph_tag == 8){
      //I'm hit by revive, but wet. Or hit with fire, but I'm water class (immune). Show water.
      telegraph_hit( 8 );
      //send a transmission, to who tagged me, that they hit my armor
      if(HEALTH > 0 && sent_device != DEVICE_ID) wireless_transceiver(sent_device, 2, DEVICE_ID, sum_of_tag);
    }
  }
  

 
  if(SERIAL_DEBUG){
    Serial.print(F("Health: "));
    Serial.println(HEALTH);
    Serial.println(F(""));      //print break after each tag
  }
  
}

/*****
 * update_display f(x)
 * The take damage function takes player damage. It takes in the tag that the player
 * was hit with and applies all buffs, debuffs, etc.
 * input: invert (int) - whether or not to invert the screen. 1 = invert, 0 = normal, new_device_id: if greater than -1, update device that killed you
 * output: nothing. changes the display
******/
void update_display( int invert, int new_device_id ){
  static int device_id;
  if(new_device_id >= 0) device_id = new_device_id;
  #if ATMEGA4809_TRUE  //using a different library for ATmega4809
    const byte y_coordinate = 0;
    const byte lower_y_coordinate = 24;
    const byte lower_x_coordinate = 9;           //how many pixels to add to x to print a number
    const byte lower_x_coordinate_over_10 = 6;   //how many pixels to add to x if there are over 10 numbers to print
    const byte lower_x_coordinate_over_100 = 3;  //how many pixels to add to x if there are over 10 numbers to print
    const byte max_x_coordinate = 104;           //how many pixels you can go in X
    byte x_coordinate = 0;                       //keeps track of where you are in x coordinate
    //Old code with old library
    if(invert == 0){
      display.invertDisplay(false);       //clear any inversion of the display
      display.clearDisplay();             //clear the screen first
      display.setTextSize(1);             // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);   // Draw white text
      //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text (not doing right now)
      display.drawBitmap(x_coordinate, y_coordinate, health_image, 26, 24, 1);
      if(HEALTH > 99){
          display.setCursor(x_coordinate + lower_x_coordinate_over_100, lower_y_coordinate);
        }
        else if(HEALTH > 9){
          display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);
        }
        else{
          display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);
        }
      display.println(HEALTH);
      x_coordinate = x_coordinate + 26;    //all images are 26 pixels wide
      
      if (CHARACTER_CLASS > 1){
        //Soldier doesn't have mana
        display.drawBitmap(x_coordinate, 0, mana_image, 26, 24, 1);
        if(MANA > 99){
          display.setCursor(x_coordinate + lower_x_coordinate_over_100, lower_y_coordinate);
        }
        else if(MANA > 9){
          display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);
        }
        else{
          display.setCursor(36, lower_y_coordinate);
        }
        display.println(MANA);
        x_coordinate = x_coordinate + 26;    //all images are 26 pixels wide
      }
      
      if(USE_AMMO > 0){
        //if you are using ammo, show it
        display.drawBitmap(x_coordinate, 0, ammo_image, 26, 24, 1);
        if(AMMO > 99){
          display.setCursor(x_coordinate + lower_x_coordinate_over_100, lower_y_coordinate);
        }
        else if(AMMO > 9){
          display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);
        }
        else{
          display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);
        }
        
        display.println(AMMO);
        x_coordinate = x_coordinate + 26;    //all images are 26 pixels wide
      }

      if(ARCANE_RUNE > 0){
        //if you have an arcane rune, show it
        display.drawBitmap(x_coordinate, 0, arcane_image, 26, 24, 1);
        if(ARCANE_RUNE > 99){
          display.setCursor(x_coordinate + lower_x_coordinate_over_100, lower_y_coordinate);
        }
        else if(ARCANE_RUNE > 9){
          display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);
        }
        else{
          display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);
        }
        display.println(ARCANE_RUNE);
        x_coordinate = x_coordinate + 26;    //all images are 26 pixels wide
      }

      //display all of the statuses
      display_status_helper();

      //then display everything you've put into the buffer
      display.display();  
    }
    else if (invert == 1){
      //if I'm supposed to invert the display, do so
      display.invertDisplay(true);
    }
    else if (invert == 2){
      //Show what class you are and what team
      display.invertDisplay(false);       //clear any inversion of the display
      display.clearDisplay();             //clear the screen first
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE);   // Draw white text
      display.setCursor(0,0);             // Start at top-left corner
      display.println(F("Class:"));
      if(CHARACTER_CLASS == 1) display.println(F("Soldier"));
      if(CHARACTER_CLASS == 2) display.println(F("Medic"));
      if(CHARACTER_CLASS == 3) display.println(F("Base"));
      if(CHARACTER_CLASS == 4) display.println(F("Pyro"));
      if(CHARACTER_CLASS == 5) display.println(F("Ice"));
      if(CHARACTER_CLASS == 6) display.println(F("Poison"));
      if(CHARACTER_CLASS == 7) display.println(F("Rock"));
      if(CHARACTER_CLASS == 8) display.println(F("Water"));
      if(CHARACTER_CLASS == 9) display.println(F("Lightning"));
      if(CHARACTER_CLASS == 10) display.println(F("Necromancer"));
      if(CHARACTER_CLASS == 11) display.println(F("Juggernaut"));
      display.println(F(" "));
      if(TEAM == 0) display.print(F("White "));
      if(TEAM == 1) display.print(F("Red "));
      if(TEAM == 2) display.print(F("Blue "));
      if(TEAM == 3) display.print(F("Green "));
      if(TEAM == 4) display.print(F("Orange "));
      if(TEAM == 5) display.print(F("Magenta "));
      if(TEAM == 6) display.print(F("Yellow "));
      if(TEAM == 7) display.print(F("Purple "));
      display.print(F("Team"));
      display.display();
    }
    else if (invert == 3){
      //show who killed you
      display.clearDisplay();             //clear the screen first
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.println(F("Out from"));
      display.print(F("Player:"));
      display.println(device_id);
      //if using auto revive, show how long before you are back in
      if(AUTO_REVIVE > 0){
        //if Auto revive is on, show how long you have until revived
        display.print(F("Spawn in:"));
        display.println(int((AUTO_REVIVE_TIME - millis())/1000));
      }
      if (WATER == 1) display.drawBitmap(52, 32, water_image, 26, 32, 1);
      display.display();
      display.invertDisplay(true);
    }
    else if (invert == 4){
      //show who won king of the hill
      display.clearDisplay();             //clear the screen first
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.print(F("Red Team: "));
      display.println(RED_TEAM_VICTORY_POINTS);
      display.print(F("Blue Team: "));
      display.println(BLUE_TEAM_VICTORY_POINTS);
      display.print(F("Orange Team: "));
      display.println(ORANGE_TEAM_VICTORY_POINTS);
      display.print(F("Magenta Team: "));
      display.println(MAGENTA_TEAM_VICTORY_POINTS);
      display.display();
    }
    //need to add invert 4, king of the hill
    
  #else  //include libraries for ATmega328p 

  if(HEALTH > 0){
    //if you still have health, update display to show what your current status is
    if(invert == 0){
      if(KING_OF_THE_HILL == 0){
        ssd1306_clearScreen( );
        ssd1306_normalMode();
        if (CHARACTER_CLASS == 3){     //base uses health space to display team
            //the string_table is set up in the order of the teams
            strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[TEAM])));  // Necessary casts and dereferencing string in PROGMEM
            ssd1306_printFixed(0,  32, BUFFER, STYLE_BOLD); 
          }
        else{
          String HEALTH_STRING = "HP:" + String(HEALTH);
          ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
          ssd1306_printFixed(0,  32, HEALTH_STRING.c_str(), STYLE_BOLD);
        }
    
        if (CHARACTER_CLASS > 1){         //soldier class doesn't have mana
          if(CHARACTER_CLASS == 3){
            //say revives for base
            ssd1306_printFixed(0, 48, "LIVES:", STYLE_NORMAL);
            ssd1306_printFixed(48, 48, String(MANA).c_str(), STYLE_BOLD);
          }
          else{
            ssd1306_printFixed(0, 48, "MANA:", STYLE_NORMAL);
            ssd1306_printFixed(40, 48, String(MANA).c_str(), STYLE_BOLD);
          }
        }
        //set font for the rest of printing
        ssd1306_setFixedFont(ssd1306xled_font6x8); //set font
        if (FIRE == 1) ssd1306_drawXBitmap(0, 0, 26, 32, fire_image);
        if (ICE_STACKS > 0){
          ssd1306_drawXBitmap(26, 0, 26, 24, snowflake_image);   //draw image
          String ICE_STRING = String(ICE_STACKS);
          ssd1306_printFixed(37, 24, ICE_STRING.c_str(), STYLE_NORMAL);
        }
        if (WATER > 0) ssd1306_drawXBitmap(52, 0, 26, 32, water_image);
        if (POISON_STACKS > 0){
          ssd1306_drawXBitmap(78, 0, 26, 24, poison_image);
          String POISON_STRING = String(POISON_STACKS);
          if(POISON_STACKS < 10){
            ssd1306_printFixed(89, 24, POISON_STRING.c_str(), STYLE_NORMAL);
          }
          else{
            ssd1306_printFixed(89, 24, POISON_STRING.c_str(), STYLE_NORMAL);
          }
        }
        if (LIGHTNING_STACKS > 0){
          ssd1306_drawXBitmap(102, 0, 26, 24, lightning_image);
          String Lightning_STRING = String(LIGHTNING_STACKS);
          ssd1306_printFixed(115, 24, Lightning_STRING.c_str(), STYLE_NORMAL);
        }
        //set font for the two other items to print
        ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
        ssd1306_negativeMode();
        ssd1306_fillRect(62, 32, 127, 63);  //first are x and y coordinates)
        if (ROCK_STACKS > 0) {
          String ROCK_STRING = "Rock:" + String(ROCK_STACKS);
          ssd1306_printFixed(62, 32, ROCK_STRING.c_str(), STYLE_BOLD);
        }
        if (LIVES > 0){
          String Lives_STRING = "Lives:" + String(LIVES);
          ssd1306_printFixed(62, 48, Lives_STRING.c_str(), STYLE_BOLD);
        }
        ssd1306_positiveMode();   //get out of negative mode before looping
      }
      else{
        //I am playing king of the hill, and need to show the score
        ssd1306_clearScreen( );
        ssd1306_normalMode();
        ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
        String POINTS_STRING = "Red Team:" + String(RED_TEAM_VICTORY_POINTS);
        ssd1306_printFixed(0,  0, POINTS_STRING.c_str(), STYLE_BOLD);
        POINTS_STRING = "Blue Team:" + String(BLUE_TEAM_VICTORY_POINTS);
        ssd1306_printFixed(0,  16, POINTS_STRING.c_str(), STYLE_BOLD);
        POINTS_STRING = "Orange Team:" + String(ORANGE_TEAM_VICTORY_POINTS);
        ssd1306_printFixed(0,  32, POINTS_STRING.c_str(), STYLE_BOLD);
        POINTS_STRING = "Magenta Team:" + String(MAGENTA_TEAM_VICTORY_POINTS);
        ssd1306_printFixed(0,  48, POINTS_STRING.c_str(), STYLE_BOLD);
      }
    }
    else if (invert == 1){
      //if I'm supposed to invert the display, do so
      //ssd1306_fillRect(0, 0, 127, 63);  //first are x and y coordinates)
      //delay(500);
      //ssd1306_clearScreen( );
      //delay(500);
      ssd1306_invertMode();
      delay(100);
    }
    else if (invert == 2){
      //Show what class you are and what team
      ssd1306_normalMode();
      ssd1306_clearScreen( );
      ssd1306_setFixedFont(ssd1306xled_font8x16); //set font

      //the string_table is set up in the order of the classes, offset by the first 8 items in the array
      //string_table is saved in program memory to save RAM (which is where strings are normally saved)
      //so, add 7 to character class, since item 8 is soldier. 9 is medic, etc.
      strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[(CHARACTER_CLASS + 7)])));  // Necessary casts and dereferencing string in PROGMEM
      ssd1306_printFixed(0,  0, BUFFER, STYLE_BOLD);
      
      //the string_table is set up in the order of the teams
      strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[TEAM])));  // Necessary casts and dereferencing string in PROGMEM
      ssd1306_printFixed(0,  32, BUFFER, STYLE_BOLD);
      /*
      if(TEAM == 0){
        strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[0])));  // Necessary casts and dereferencing, just copy.
        ssd1306_printFixed(0,  32, BUFFER, STYLE_BOLD);
      }
      if(TEAM == 1){
        strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[1])));
        ssd1306_printFixed(0,  32, BUFFER, STYLE_BOLD);
      }
      if(TEAM == 2){
        strcpy_P(BUFFER, (char *)pgm_read_word(&(string_table[2])));
        ssd1306_printFixed(0,  32, BUFFER, STYLE_BOLD);
      }
      if(TEAM == 3) ssd1306_printFixed(0,  32, "Green Team", STYLE_BOLD);
      if(TEAM == 4) ssd1306_printFixed(0,  32, "Orange Team", STYLE_BOLD);
      if(TEAM == 5) ssd1306_printFixed(0,  32, "Magenta Team", STYLE_BOLD);
      if(TEAM == 6) ssd1306_printFixed(0,  32, "Yellow Team", STYLE_BOLD);
      if(TEAM == 7) ssd1306_printFixed(0,  32, "Purple Team", STYLE_BOLD);
      */
    }
    else if (invert == 4){
      //show who won king of the hill
      ssd1306_clearScreen( );
      ssd1306_normalMode();
      ssd1306_setFixedFont(ssd1306xled_font8x16); //set font
      String POINTS_STRING = "Red Team:" + String(RED_TEAM_VICTORY_POINTS);
      ssd1306_printFixed(0,  0, POINTS_STRING.c_str(), STYLE_BOLD);
      POINTS_STRING = "Blue Team:" + String(BLUE_TEAM_VICTORY_POINTS);
      ssd1306_printFixed(0,  16, POINTS_STRING.c_str(), STYLE_BOLD);
      POINTS_STRING = "Orange Team:" + String(ORANGE_TEAM_VICTORY_POINTS);
      ssd1306_printFixed(0,  32, POINTS_STRING.c_str(), STYLE_BOLD);
      POINTS_STRING = "Magenta Team:" + String(MAGENTA_TEAM_VICTORY_POINTS);
      ssd1306_printFixed(0,  48, POINTS_STRING.c_str(), STYLE_BOLD);
      ssd1306_invertMode();
    }
  }
  else{
    //if you don't have any health left, keep the display of who tagged you out, and do nothing else
    if (invert == 3){
      //show who killed you
      ssd1306_clearScreen( );
      ssd1306_printFixed(0,  0, "Out From device", STYLE_BOLD);
      ssd1306_printFixed(0,  16, String(device_id).c_str(), STYLE_BOLD);
      if (WATER > 0) ssd1306_drawXBitmap(52, 0, 26, 32, water_image);
      //if using auto revive, show how long before you are back in
      if(AUTO_REVIVE > 0){
        //if Auto revive is on, show how long you have until revived
        ssd1306_printFixed(0,  32, "Spawn in:", STYLE_BOLD);
        ssd1306_printFixed(0,  48, String(int((AUTO_REVIVE_TIME - millis())/1000)).c_str(), STYLE_BOLD);
      }
      ssd1306_invertMode();
    }
  }

  #endif 
  
}

/******
display_status_helper f(x)
This function helps display all of the different status symbols one after another
inputs: nothing
outputs: nothing (sends information to the oled
IMAGE_NUM_PIXELS_X is the variable for number of pixels to move 
 ******/

void display_status_helper( void ){
 #if ATMEGA4809_TRUE  //using a different library for ATmega4809
  const byte y_coordinate = 32;
  const byte lower_y_coordinate = 56;
  const byte lower_x_coordinate = 11;          //how many pixels to add to x to print a number
  const byte lower_x_coordinate_over_10 = 8;   //how many pixels to add to x if there are over 10 numbers to print
  const byte max_x_coordinate = 104;           //how many pixels you can go in X
  byte x_coordinate = 0;                       //keeps track of where you are in x coordinate
  if (FIRE == 1){
    display.drawBitmap(x_coordinate, y_coordinate, fire_image, 26, 32, 1);
    x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (ICE_STACKS > 0){
    display.drawBitmap(x_coordinate, y_coordinate, snowflake_image, 26, 24, 1);   //draw image
    display.setTextSize(1);
    display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);        //set cursor to just below the image
    display.print(ICE_STACKS);
    x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (WATER == 1){
    display.drawBitmap(x_coordinate, y_coordinate, water_image, 26, 32, 1);
    x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (POISON_STACKS > 0){
    display.drawBitmap(x_coordinate, y_coordinate, poison_image, 26, 24, 1);
    display.setTextSize(1);
    if(POISON_STACKS < 10){
      display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);        //set cursor to just below the image
    }
    else{
      display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);        //set cursor to just below the image
    }
    display.print(POISON_STACKS);
    x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (WIND > 0){
    display.drawBitmap(x_coordinate, y_coordinate, wind_image, 26, 24, 1);
    display.setTextSize(1);
    if(WIND < 10){
      display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);        //set cursor to just below the image
    }
    else{
      display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);        //set cursor to just below the image
    }
    display.print(WIND);
    x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (DOUBLE_DAMAGE_RUNE > 0){
    display.drawBitmap(x_coordinate, y_coordinate, double_damage_image, 26, 24, 1);   //draw image
    display.setTextSize(1);
    if(DOUBLE_DAMAGE_RUNE < 10){
      display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);        //set cursor to just below the image
    }
    else{
      display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);        //set cursor to just below the image
    }
    display.print(DOUBLE_DAMAGE_RUNE);
    if(x_coordinate < max_x_coordinate) x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;   //you only have the potential to overlap on the last 2
  }
  if (LIVES > 0){
    display.drawBitmap(x_coordinate, y_coordinate, lives_image, 26, 24, 1);   //draw image
    display.setTextSize(1);
    if(LIVES < 10){
      display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);        //set cursor to just below the image
    }
    else{
      display.setCursor(x_coordinate + lower_x_coordinate_over_10, lower_y_coordinate);        //set cursor to just below the image
    }
    display.print(LIVES);
    if(x_coordinate < max_x_coordinate) x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;   //you only have the potential to overlap on the last 2
  }
  if (LIGHTNING_STACKS > 0){
    display.drawBitmap(x_coordinate, y_coordinate, lightning_image, 26, 24, 1);
    display.setTextSize(1);
    display.setCursor(x_coordinate + lower_x_coordinate, lower_y_coordinate);
    display.print(LIGHTNING_STACKS);
    if(x_coordinate < max_x_coordinate) x_coordinate = x_coordinate + IMAGE_NUM_PIXELS_X;
  }
  if (ROCK_STACKS > 0) {
    display.drawBitmap(104, 0, rock_image, 26, 24, 1);
    display.setCursor(104,8);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.setTextSize(2);
    display.print(ROCK_STACKS);
  }
 #endif
}


/*******
wireless_transceiver f(x)
This function sends wireless messages and receives wireless messages
Messages are sent / received through the Serial port and the HC-12 transceiver
the send part of the function checks if it is time to send a packet, then sends it.
If it receives a packet to send, it stores it in a buffer
If it is time to send packets, it also checks for any received, and sends them to be processed
message -1 is used to loop this function and check if it's time to send
message -2 is used to set the time that is looped from.
struct payload_t {                   // Structure of our payload
    byte command;
    byte who_was_hit;
    byte hit_by;
    byte tag_received;
    byte team_received;
    byte bonus_damage;
  };
 */

void wireless_transceiver( byte hit_by_device_id, int message, byte my_device_id, 
                           unsigned long tag){
  #if ATMEGA4809_TRUE
    //set all parts of the payload
    //in wireless commands 1-9, device_id is who tagged me, and value_2 is my device id
     payload.command = byte(message);
     payload.who_was_hit = my_device_id;
     payload.hit_by = hit_by_device_id;                                          //device id that hit
     payload.tag_received = tag;
     RF24NetworkHeader header(node_lookup(CONTROLLER_DEVICE_ID));         //create header with other node to send to (controller)
     network.write(header, &payload, sizeof(payload));                    //send payload to other node
  #else  //include libraries for ATmega328p
    static unsigned long next_event_time;              //time to do next thing
    static byte current_status;                        //current step we are on
    static byte array_to_send[WIRELESS_SEND_BUFFER];   //aray to send
    static byte array_index;                           //current array index
    unsigned long current_time = millis();

    //If you get a new message to send, add that to the buffer
    if(message != -1){
      //if message is -1, we are checking if we can send. (that should go to the "else" below to check to send)
      if(message == -2){
        //if the message is to set the time for transmissions.
        next_event_time = current_time + (DEVICE_ID*(RADIO_TIME_PER_DEVICE + RADIO_SILENCE_TIME));
        if(SERIAL_DEBUG){
          //if we are debugging, write what you see
           Serial.println(F("Synchronize Wireless to now"));
          }
      }
      else if(array_index < WIRELESS_SEND_BUFFER){
        //if we aren't setting the time, and there is room in the array
        array_to_send[array_index] = hit_by_device_id;       //set array to send
        array_to_send[array_index + 1] = message;
        array_to_send[array_index + 2] = my_device_id;
        array_to_send[array_index + 3] = byte(tag >> 24);    //send the tag and team bits
        array_to_send[array_index + 4] = wireless_checksum(array_to_send[array_index], array_to_send[array_index + 1],
                                                           array_to_send[array_index + 2], array_to_send[array_index + 3] );
        array_index = array_index + WIRELESS_MESSAGE_LENGTH;
        if(SERIAL_DEBUG){
          //if we are debugging, write what you see
          for(int i = 0; i < array_index; i++){
            Serial.print(F("Array value: "));
            Serial.println(array_to_send[i]);
          }
        }
      }
    }
  
    //check to see if it is time to send
    if(current_time >= next_event_time){
      if(current_time <= (next_event_time + RADIO_TIME_PER_DEVICE)){
        //if we are within the time to send and have data to send, send our data
        if(array_index > 0){
          Serial.write(array_to_send, array_index);  //send all bytes
          if(SERIAL_DEBUG){
          //if we are debugging, write what you see
          Serial.println(F("sent wireless transmission"));
          }
          //reset array_index to indicate all data is sent
          array_index = 0;
        }
        
      }
      //update time to send next transmission, leaving time for every other radio to transmit
      next_event_time = next_event_time + MAX_RADIOS_USED*(RADIO_TIME_PER_DEVICE + RADIO_SILENCE_TIME);
    }
  #endif
}

/*****
wireless_checksum f(x)
This function calculates the checksum of a data packet of 3 
 */
byte wireless_checksum(byte first, byte second, byte third, byte fourth ){
  int result = first + second + third + fourth;
  result = (result & 0xFF) + (result >> 8);             //add together any carryover
  return byte(result & 0xFF);
}

/*******
node_lookup f(x)
this function takes a device id and looks up the node id for that device id
input: device_id (int)
output: node_id (int)
 */
uint16_t node_lookup( int device_id_to_lookup ){
  switch (device_id_to_lookup){
    case 14:
      return 011;
      break;
    case 15:
      return 021;
      break;
    case 16:
      return 031;
      break;
    case 17:
      return 041;
      break;
    case 18:
      return 012;
      break;
    case 19:
      return 022;
      break;
    case 20:
      return 032;
      break;
    case 21:
      return 042;
      break;
    case 22:
      return 013;
      break;
    case 23:
      return 023;
      break;
    case 24:
      return 033;
      break;
    case 25:
      return 043;
      break;
    case 26:
      return 014;
      break;
    case 27:
      return 024;
      break;
    case 28:
      return 034;
      break;
    case 29:
      return 044;
      break;
    case 30:
      return 01;
      break;
    case 31:
      return 02;
      break;
    case 32:
      return 03;
      break;
    case 33:
      return 04;
      break;
    case 64:
      return 00;
      break;
    default:
      return 00;
      break;
  }
}
 

/******
receive_wireless f(x)
Checks to see if there was a wireless message sent to the device and processes it.  
For the atmega328p, If it's an AT command, it is ignored. Otherwise, checks to see if it's 3 bytes, and is for this device
inputs - nothing
outputs - nothing (calls other functions that output)
 */
void receive_wireless(void) {
  #if ATMEGA4809_TRUE
    //using the nrf radio's
    network.update();                  // Check the network regularly
    while (network.available()) {      // Is there anything ready for us?
    RF24NetworkHeader header;          // If so, grab it and print it out
    network.read(header, &payload, sizeof(payload));

    //then, do something with the received message
    switch (payload.command){
            case 1:
              //I tagged someone, and should indicate it
              //Serial.println("Tagged Someone!");
              telegraph_tags(1);
              play_mp3_sound(OTHER_EVENT, HIT_SOMEONE_SOUND_NUM);
              break;
            case 2:
              //I tagged someone who had armor, and should indicate it
              //Serial.println("Tagged Someone's Armor!");
              telegraph_tags(2);
              break;
            case 3:
              //I tagged someone, and they are out. Do something because of it
              //note, you haven't programmed any classes to heal when you tag someone out
              break;
            case 4:
              //I tagged the juggernaut, and they are out because of it. Do something because of it
              //Serial.println("I'm the juggernaut now!");
              #if CHANGE_TO_JUG_WHEN_KILL_JUG
              respawn(11, TEAM, 100, 0);       //respawn using current team, with 100% health
              #endif
              
              break;
            case 5:
              //received "hello world" from a device turning on
              //Serial.println("I'm the juggernaut now!");
              break;
            case 6:
              //Juggernaut has entered the playing field. Make a sound
              play_mp3_sound(OTHER_EVENT, JUGGERNAUT_HERE_SOUND_NUM);
              break;
            case 7:
              //not used
              break;
            case 8:
              //My automatic aim assist tag hit someone, and I should indicate that
              telegraph_tags(3);
              break;
            case 9:
              //My manual aim assist tag hit someone, and I should indicate that
              telegraph_tags(4);
              break;
            case 11:
              //Received instruction to change variables (on the third byte, the "hit_by" byte)
              //send message back that you received the command
              wireless_transceiver(payload.hit_by, 12, DEVICE_ID, 0);  //send message for this back to base for confirmation
              //then act on the command
              switch (payload.hit_by){
                case 1:
                  //turn survival mode on
                  SURVIVAL_MODE = 1;
                  play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                  break;
                case 2:
                  //turn survival mode off
                  SURVIVAL_MODE = 0;
                  play_sound(out_sound, out_sound_delay, out_sound_length);
                  break;
                case 3:
                  //turn auto revive on
                  AUTO_REVIVE = 1;
                  play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                  break;
                case 4:
                  //turn auto revive off
                  AUTO_REVIVE = 0;
                  play_sound(out_sound, out_sound_delay, out_sound_length);
                  break;
                case 5:
                  //turn ammo use mode on
                  AUTO_RESPAWN_MANA = 0;
                  USE_AMMO = 1;
                  //update the mana and mana multiplier
                  MANA = MANA*AMMO_MANA_MULTIPLIER;
                  MAX_MANA = MAX_MANA*AMMO_MANA_MULTIPLIER;
                  play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                  break;
                case 6:
                  //turn ammo use mode off
                  AUTO_RESPAWN_MANA = 1;
                  USE_AMMO = 0;
                  //get rid of the extra mana you got for using ammo
                  MAX_MANA = MAX_MANA/AMMO_MANA_MULTIPLIER;
                  MANA = MAX_MANA;
                  play_sound(out_sound, out_sound_delay, out_sound_length);
                  break;
                case 7:
                  //turn auto aim assist on
                  ENABLE_AIM_ASSIST = 1;
                  play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                  break;
                case 8:
                  //turn auto aim assist off
                  ENABLE_AIM_ASSIST = 0;
                  play_sound(out_sound, out_sound_delay, out_sound_length);
                  break;
                case 9:
                  //give double damage rune
                  take_damage( 31, 0, 0, 0, 0 );
                  break;
                case 10:
                  //give double health rune
                  take_damage( 30, 0, 0, 0, 0 );
                  break;
                case 11:
                  //give arcane damage rune
                  take_damage( 29, 0, 0, 0, 0 );
                  break;
                case 12:
                  //give double armor rune
                  take_damage( 28, 0, 0, 0, 0 );
                  break;
                case 13:
                  //give extra life rune
                  take_damage( 27, 0, 0, 0, 0 );
                  break;
                case 14:
                  //give ammo refill rune
                  take_damage( 26, 0, 0, 0, 0 );
                  break;
                default:
                  break;
              }
              break;
            default:
              break;
          }
    /*
    dataStr[0] = 0; //clean out string for the next write

     ltoa( payload.ms,buffer,10); //convert long to charStr
     strcat(dataStr, buffer); //add it to the end
     strcat( dataStr, ", "); //append the delimiter
     
     //dtostrf(floatVal, minimum width, precision, character array);
     dtostrf(payload.x_val, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
     strcat( dataStr, buffer); //append the converted float
     strcat( dataStr, ", "); //append the delimiter
    
     dtostrf(payload.y_val, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
     strcat( dataStr, buffer); //append the converted float
     strcat( dataStr, ", "); //append the delimiter
    
     dtostrf(payload.z_val, 5, 1, buffer);  //5 is minimum width, 1 is precision; float value is copied onto buff
     strcat( dataStr, buffer); //append the converted float
     strcat( dataStr, 0); //terminate correctly 
     Serial.println(dataStr); 
     */
    }
  #else  //include libraries for ATmega328p
    
    static byte message_array[WIRELESS_MESSAGE_LENGTH];
    static byte message_array_index;
    while (Serial.available()) {        // while data is available
      if(SERIAL_DEBUG){
          //if we are debugging, write what you see
          message_array[message_array_index] = Serial.read();
          Serial.println(message_array[message_array_index]);
          message_array_index = message_array_index + 1;
        }
        else{
          //otherwise, just read it and don't tell anyone
          message_array[message_array_index] = Serial.read();
          //
          //Serial.println(message_array[message_array_index]);
          message_array_index = message_array_index + 1;
        }
      
      if(message_array_index == WIRELESS_MESSAGE_LENGTH){
        if(SERIAL_DEBUG){
          Serial.println(F("Received 5 bytes"));
          for(int i = 0; i < message_array_index; i++){
            Serial.print(F("Array value: "));
            Serial.println(message_array[i]);
          }
        }
        
        //check if the last byte is a valid checksum
        int check = wireless_checksum(message_array[0], message_array[1], message_array[2], message_array[3]);
        if(message_array[(WIRELESS_MESSAGE_LENGTH - 1)] == check){
          //Serial.println(F("match"));
          //check to see if you should do anything witht he message that was received
          if (message_array[0] == DEVICE_ID || message_array[0] == CONTROLLER_DEVICE_ID){
            //if the message is sent to you, or is a global message, parse it and do what it instructs
            switch (message_array[1]){
              case 1:
                //I tagged someone, and should indicate it
                //Serial.println("Tagged Someone!");
                telegraph_tags(1);
                break;
              case 2:
                //I tagged someone who had armor, and should indicate it
                //Serial.println("Tagged Someone's Armor!");
                telegraph_tags(2);
                break;
              case 3:
                //I tagged someone, and they are out. Do something because of it
                break;
              case 4:
                //I tagged the juggernaut, and they are out because of it. Do something because of it
                //Serial.println("I'm the juggernaut now!");
                #if CHANGE_TO_JUG_WHEN_KILL_JUG
                respawn(11, TEAM, 100, 0);       //respawn using current team, with 100% health
                #endif
                break;
              case 5:
                //received "hello world" from a device turning on
                //Serial.println("I'm the juggernaut now!");
                break;
              case 6:
                //sync radio time: set the time for radio transmissions in wireless_transceiver f(x)
                wireless_transceiver( 0, -2, 0, 0);
                RADIO_IN_SYNC_MODE = 0;                          //turn off syncing radios
                if(SERIAL_DEBUG){
                  Serial.println(F("Done Synching Radios"));
                }
                break;
              case 7:
                //enter sync mode
                RADIO_IN_SYNC_MODE = 1;                                //turn on syncing radios
                RADIO_TIMEOUT_TIME = millis() + RADIO_TIMEOUT_DELAY;   //set timeout time
                if(SERIAL_DEBUG){
                  Serial.println(F("Synching Radios"));
                }
                break;
              case 8:
                //My automatic aim assist tag hit someone, and I should indicate that
                telegraph_tags(3);
                break;
              case 9:
                //My manual aim assist tag hit someone, and I should indicate that
                telegraph_tags(4);
                break;
              case 11:
                //Received instruction to change variables (on the third byte, the "hit_by" byte)
                //send a response back that you received this
                wireless_transceiver(message_array[2], 12, DEVICE_ID, 0);  //send message for this back to base for confirmation
  
                //then change variables based on what you were hit by
                switch (message_array[2]){
                  case 1:
                    //turn survival mode on
                    SURVIVAL_MODE = 1;
                    play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                    break;
                  case 2:
                    //turn survival mode off
                    SURVIVAL_MODE = 0;
                    
                    play_sound(out_sound, out_sound_delay, out_sound_length);
                    break;
                  case 3:
                    //turn auto revive on
                    AUTO_REVIVE = 1;
                    
                    play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                    break;
                  case 4:
                    //turn auto revive off
                    AUTO_REVIVE = 0;
                    
                    play_sound(out_sound, out_sound_delay, out_sound_length);
                    break;
                  case 5:
                    //turn ammo use mode on
                    AUTO_RESPAWN_MANA = 0;
                    USE_AMMO = 1;
                    //update the mana and mana multiplier
                    MANA = MANA*AMMO_MANA_MULTIPLIER;
                    MAX_MANA = MAX_MANA*AMMO_MANA_MULTIPLIER;
                    
                    play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                    break;
                  case 6:
                    //turn ammo use mode off
                    AUTO_RESPAWN_MANA = 1;
                    USE_AMMO = 0;
                    //get rid of the extra mana you got for using ammo
                    MAX_MANA = MAX_MANA/AMMO_MANA_MULTIPLIER;
                    MANA = MAX_MANA;
                    
                    play_sound(out_sound, out_sound_delay, out_sound_length);
                    break;
                  case 7:
                    //turn auto aim assist on
                    ENABLE_AIM_ASSIST = 1;
                    
                    play_sound(startup_sound, startup_sound_delay, startup_sound_length);
                    break;
                  case 8:
                    //turn auto aim assist off
                    ENABLE_AIM_ASSIST = 0;
                    
                    play_sound(out_sound, out_sound_delay, out_sound_length);
                    break;
                  case 9:
                    //give double damage rune
                    take_damage( 31, 0, 0, 0, 0 );
                    break;
                  case 10:
                    //give double health rune
                    take_damage( 30, 0, 0, 0, 0 );
                    break;
                  case 11:
                    //give arcane damage rune
                    take_damage( 29, 0, 0, 0, 0 );
                    break;
                  case 12:
                    //give double armor rune
                    take_damage( 28, 0, 0, 0, 0 );
                    break;
                  case 13:
                    //give extra life rune
                    take_damage( 27, 0, 0, 0, 0 );
                    break;
                  case 14:
                    //give ammo refill rune
                    take_damage( 26, 0, 0, 0, 0 );
                    break;
                  default:
                    break;
                }
                
                break;
              default:
                break;
            }
          }
          //reset the array after reading the message
          message_array_index = 0;
        }
        else{
          //if the checksum didn't match, it's not valid. Check if the next one is
            for(int i = 0; i < (WIRELESS_MESSAGE_LENGTH - 1); i++){
              message_array[i] = message_array[(i + 1)];
            }
            message_array_index = (WIRELESS_MESSAGE_LENGTH - 1);   //set to check the last byte again
        } 
        
      }
      else{
        //if the last item in the array isn't 255, it's not valid. Check if the next one is
            for(int i = 0; i < (WIRELESS_MESSAGE_LENGTH - 1); i++){
              message_array[i] = message_array[(i + 1)];
            }
            message_array_index = (WIRELESS_MESSAGE_LENGTH - 1);   //set to check the last byte again
      }
        
    }

  #endif
}

/**********
telegraph_tags f(x)
this function shows whether or not the tags you have sent hit someone
this function takes in what light to turn on (1 or 2), along with whether or not
you are checking the lights (0). It then outputs signals to the LED's, whether it was
a good hit (green, or input of 1) or hit armor (yellow). 
inputs - integer - 0, 1 (good hit LED), 2 (Hit Armor), 3 (Automatic Aim Assist), or 4 (manual aim assist)
outputs - void - turns on lights on device
#define HIT_ENEMY_DEVICE_PIN A3
#define HIT_ENEMY_ARMOR_PIN A4
 */
void telegraph_tags(int check_variable){
  static unsigned long hit_on_time;
  static unsigned long armor_hit_on_time;
  static byte hit_on;
  static byte armor_on;
  unsigned long current_time = millis();

  switch(check_variable){
    case 0:
      //I'm checkint to see if I need to do anything
      if(hit_on > 0){
        //if I've  hit, check to see if I need to turn off
        if(current_time > hit_on_time){
          //turn off
          digitalWrite(HIT_ENEMY_DEVICE_PIN, LOW);
          hit_on = 0;
        }
      }
      if(armor_on > 0){
        //if I've hit armor, check to see if I need to turn off
        if(current_time > armor_hit_on_time){
          //turn off
          digitalWrite(HIT_ENEMY_ARMOR_PIN, LOW);
          armor_on = 0;
        }
      }
      break;
    case 1:
      //I should turn on green light
      digitalWrite(HIT_ENEMY_DEVICE_PIN, HIGH);
      hit_on_time = current_time + TELEGRAPH_ENEMY_HIT;
      hit_on = 1;
      break;
    case 2:
      //I should turn on yellow light
      digitalWrite(HIT_ENEMY_ARMOR_PIN, HIGH);
      armor_hit_on_time = current_time + TELEGRAPH_ENEMY_HIT;
      armor_on = 1;
      break;
    case 3:
      //I should turn on yellow light for automatic aim assist
      digitalWrite(HIT_ENEMY_ARMOR_PIN, HIGH);
      armor_hit_on_time = current_time + TELEGRAPH_AIM_ASSIST_HIT;
      armor_on = 1;
      break;
    case 4:
      //I should turn on green light for manual aim assist
      digitalWrite(HIT_ENEMY_DEVICE_PIN, HIGH);
      hit_on_time = current_time + TELEGRAPH_AIM_ASSIST_HIT;
      hit_on = 1;
      break;
    default:
      //not sure what to do
      break;
  }
}

/*****
calculate_ability_to_send f(x)
takes in the character class and outputs the ability that character sends
input: character class
output: tag to send (byte)
 */
byte calculate_ability_to_send( byte character_class ){
  byte ability_sent = 1;  //start with this at 1 (normal tag)
  switch(character_class){
    case 1:
      //I'm soldier. Set up variables
      ability_sent = 11;   //ability is triple damage
      break;
    case 2:
      //I'm medic. Set up variables
      ability_sent = 2;
      break;
    case 3:
      //change settings for base
      if(RUNE_SENT > 0){
        //if you are a rune, use rune
        ability_sent = RUNE_SENT;
      }
      else{
        //if you are not a rune, do revive
        ability_sent = 3;
      }
      break;
    case 4:
      //I'm pyro. Set up variables
      ability_sent = 4;
      break;
    case 5:
      //I'm Ice. Set up variables. 
      ability_sent = 5;
      break;
    case 6:
      //I'm Poison. Set up variables
      ability_sent = 6;
      break;
    case 7:
      //I'm Rock. Set up variables
      ability_sent = 7;
      break;
    case 8:
      //I'm Water. Set up variables
      ability_sent = 8;
      break;
    case 9:
      //I'm Lightning. Set up variables
      ability_sent = 9;
      break;
    case 10:
      //I'm necromancer. Set up variable
      ability_sent = 3;  //send revive as ability
      break;
    case 11:
      //I'm juggernaut. Set up variables
      ability_sent = 0;  //ability is double damage (not used)
      break;
    default:
      ability_sent = 1;
      break;
  }
  if(SERIAL_DEBUG){
    Serial.print("Calculated ability: ");
    Serial.println(ability_sent);
  }
  return ability_sent;
}

/******
setup_tag_array f(x)
This function sets up the tag array that you input into it
inputs: Tag, Team, bonus Damage
outputs: updates the array with the correct information
setup_tag_array( NORMAL_TAG_NUM, TEAM, bonus_damage_total, SENT_ARRAY_TAG);
 */
 void setup_tag_array( byte tag_to_send, byte team, byte bonus_damage, byte tag_array[]){
  tag_array[0] = (80 + team);   //add 01010000 to the team
  tag_array[1] = ((bonus_damage << 3) + (tag_to_send >> 2));
  tag_array[2] = ((tag_to_send & 0x03) << 6) + DEVICE_ID; 
  //after setting up the 3 bytes in the array, then you can calculate the checksum.
  tag_array[3] = calculate_checksum(tag_array);
  /*
  if(SERIAL_DEBUG){
    Serial.println(" ");
    Serial.println("Variables to put in array");
    Serial.print("Tag: ");
    Serial.println(tag_to_send);
    Serial.print("Team: ");
    Serial.println(team);
    Serial.print("Bonus Damage: ");
    Serial.println(bonus_damage);
    Serial.println("Calculated tag array");
    Serial.print("First byte: ");
    Serial.println(tag_array[0], BIN);
    Serial.print("Second byte: ");
    Serial.println(tag_array[1], BIN);
    Serial.print("Third byte: ");
    Serial.println(tag_array[2], BIN);
    Serial.print("Fourth byte: ");
    Serial.println(tag_array[3], BIN);
   }
   */
 }

/******
calculate_checksum f(x)
This function calculates the checksum of a data packet of 3 bytes using one's complement
addition
inputs: array of two bytes
outputs: checksum value, with bonus damage on it
 */

 byte calculate_checksum( byte tag_array[] ){
  int i = tag_array[0] + tag_array[1] + tag_array[2]; //add together the bytes
  i = (i & 0xFF) + (i >> 8);             //add together any carryover
  return byte(i & 0xFF);                 //output the resulting byte
 }

#if ATMEGA4809_TRUE  //using MPU for atmega4809

/*******
read_mpu f(x)
this function reads data from the mpu and calculates whether or not you are running, walking, or standing still
It also calculates whether or not you are turning, if that is required
inputs: none (pulls from I2C bus)
outputs: Global variables (Run/walk and turning)
******/

void read_mpu(){
  static int Z_ACCEL_PREVIOUS;
  static int Y_ACCEL_PREVIOUS;
  static int X_ACCEL_PREVIOUS;
  static int Z_GYRO_PREVIOUS;
  static int Y_GYRO_PREVIOUS;
  static int X_GYRO_PREVIOUS;
  
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers, and release the I2C bus after receiving the data (for the OLED)
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)

  //get the different in acceleration between the readings, and store that
   X_ACCEL_ARRAY[ACCEL_ARRAY_INDEX] = abs(accelerometer_x - X_ACCEL_PREVIOUS);
   Y_ACCEL_ARRAY[ACCEL_ARRAY_INDEX] = abs(accelerometer_y - Y_ACCEL_PREVIOUS);
   Z_ACCEL_ARRAY[ACCEL_ARRAY_INDEX] = abs(accelerometer_z - Z_ACCEL_PREVIOUS);
   X_GYRO_ARRAY[ACCEL_ARRAY_INDEX] = abs(gyro_x - X_GYRO_PREVIOUS);
   Y_GYRO_ARRAY[ACCEL_ARRAY_INDEX] = abs(gyro_y - Y_GYRO_PREVIOUS);
   Z_GYRO_ARRAY[ACCEL_ARRAY_INDEX] = abs(gyro_z - Z_GYRO_PREVIOUS);
   ACCEL_ARRAY_INDEX++;

   if(ACCEL_ARRAY_INDEX == ACCEL_ARRAY_LEN){
      //if I've filled up the array, process it
      check_movements(); 
     }

     //reset the previous reading to the current reading
     X_ACCEL_PREVIOUS = accelerometer_x;
     Y_ACCEL_PREVIOUS = accelerometer_y;
     Z_ACCEL_PREVIOUS = accelerometer_z;
     X_GYRO_PREVIOUS = gyro_x;
     Y_GYRO_PREVIOUS = gyro_y;
     Z_GYRO_PREVIOUS = gyro_z;
/*
   if(SERIAL_DEBUG){        
    // print out data
    Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
    Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
    Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));
    // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
    Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
    Serial.print(" | gX = "); Serial.print(convert_int16_to_str(gyro_x));
    Serial.print(" | gY = "); Serial.print(convert_int16_to_str(gyro_y));
    Serial.print(" | gZ = "); Serial.print(convert_int16_to_str(gyro_z));
    Serial.println();
   }
   */
}

/*
check_movement f(x)
this function is called when you are checking if you are walking or running.
inputs: void (all global variables)
outputs: 0 for not moving, 1 for moving slightly, 2 for running
#define RUN_ACCEL_THRESHOLD 8000      //decrease to increase sensitivity (determined empirically)
#define WALK_ACCEL_THRESHOLD 5000     //decrease to increase sensitivity (determined empirically)
#define MIN_WALKING_COUNT 3           //decrease to make it more sensitive to walking as juggernaut over time
#define MIN_RUNNING_COUNT 5           //decrease to make it more sensitive to detecting running over time
byte WIND_TOTAL_MOVEMENT = 0;          //tracking the total amount of movement you've had while hit with wind
byte WIND = 0;                         //whether or not you are hit with wind
#define WIND_MOVE_THRESHOLD 7          //the amount of movement you need to do before clearing wind
#define WIND_RUNNING_MOVEMENT 2        //running counts for twice as much as walking for the wind threshold
#define WIND_WALKING_MOVEMENT 1        //running counts for twice as much as walking for the wind threshold
 */
void check_movements( void ){
  byte above_run_threshold = 0;     //start with 0 above threshold
  byte above_walk_threshold = 0;     //start with 0 above threshold
  ACCEL_ARRAY_INDEX = 0;      //reset global variable to 0 so that the next data collection runs

  
  for(int i = 0; i < ACCEL_ARRAY_LEN; i++){
    if(X_ACCEL_ARRAY[i] >= RUN_ACCEL_THRESHOLD || Y_ACCEL_ARRAY[i] >= RUN_ACCEL_THRESHOLD || 
       Z_ACCEL_ARRAY[i] >= RUN_ACCEL_THRESHOLD) above_run_threshold++;
    if(X_ACCEL_ARRAY[i] >= WALK_ACCEL_THRESHOLD || Y_ACCEL_ARRAY[i] >= WALK_ACCEL_THRESHOLD ||
       Z_ACCEL_ARRAY[i] >= WALK_ACCEL_THRESHOLD) above_walk_threshold++;
  }
  if(above_run_threshold >= MIN_RUNNING_COUNT){
    //I'm at or above the counts for running, so I'm running
    if(WIND != 0){
      //if I'm hit with wind, add to my total movement
      WIND_TOTAL_MOVEMENT = WIND_TOTAL_MOVEMENT + WIND_RUNNING_MOVEMENT;
    }
    if(CHARACTER_CLASS == 11){
      //if you are juggernaut and running, you lose your manashield
      MANA = 0;
    }
    if(SERIAL_DEBUG) Serial.println("Running");
  }
  else if (above_walk_threshold >= MIN_WALKING_COUNT){
    //I'm at or above the counts for walking, so I'm walking
    if(WIND != 0){
      //if I'm hit with wind, add to my total movement
      WIND_TOTAL_MOVEMENT = WIND_TOTAL_MOVEMENT + WIND_WALKING_MOVEMENT;
    }
    if(CHARACTER_CLASS == 11){
      //if you are juggernaut and walking, you lose your manashield
      MANA = 0;
    }
    if(SERIAL_DEBUG) Serial.println("Walking");
  }
  else{
    //I'm not running or walking, so I'm not moving
    if(SERIAL_DEBUG) Serial.println("Not moving");
  }

  //check to see if you have passed the wind threshold, and if so, turn off wind
  if(WIND_TOTAL_MOVEMENT >= WIND_MOVE_THRESHOLD){
    //if I've moved enough, turn off wind
    WIND = 0;
  }
}
#endif

#if ATMEGA4809_TRUE                  //interrupts for button presses on ATmega4809
/*
tag_pin_ISR f(x)
If the tag pin is pressed, change the TAG_BUTTON_PRESSED flag to 1
 */
void tag_pin_ISR(){
  if(millis() > cooldown_time){
    TAG_BUTTON_PRESSED = 1;
  }
}

/*
ability_pin_ISR f(x)
If the ability pin is pressed, change the ABILTY_BUTTON_PRESSED flag to 1
 */
void ability_pin_ISR(){
  if(millis() > cooldown_time){
    ABILTY_BUTTON_PRESSED = 1;
  }
}

#endif