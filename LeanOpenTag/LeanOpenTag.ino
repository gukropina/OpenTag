/*gukropina 
June 7, 2015
Open Tag Developement

Booyah

Note: Are you uploading to the correct board?

Current work:

Added piezo to add sounds to the game (3 sounds, tag sent, hit, and can't tag)
Haven't added some sort of handler for interrupting sounds, so sounds are short.

Do next: 0. Make the status LED stay on? (complete)
         6. PWM from LED Handler: You need to add on and off time for LED handler
            that way, you can dim LED's, and do true PWM without really using PWM.
            Then change health to status LED brightness, and have status LED always on. (complete)
         2. Make sure piezo_handler function is working (complete)
         0.1 Add fire rate (so you can't just spam firing too quickly) (complete)
         0.2 Add cooldowns for abilities (complete)
         0.3 Make status LED dim with health depletion (complete)

         1. add the next two parts of your protocol: team and damage
            to do this, you need to change: variables, i_am_tagged, send_tag
         2. Add #define statements for magic numbers in your code (99, 999, etc.)
         3. Change your send_tag arrays to all caps for global variables
         4. Base: make it so that if you are a base, and receive a tag w/ different team,
            you take 1 damage. At health = 0, you switch teams and have 1 health
            If you take damage from your own team, you gain health (up to max).
         5. Your send_tag function will need to be more complex to handle abilities, 
            and then send regular tags afterwards. Or maybe, before tagging, rebuild
            the arrays from defaults (or at least check them all).
            Change your send_tag function to take in integers for the arguments:
            ID
            Team
            Damage
            Unique attack modifier
            and build the tag to send off of those.
         6. Can you use your Piezo for input? (that would be a cool easter egg).
         

All units are now on a 600 microsec protocol.

This is the newest iteration of the open tag project using Lean startup
principles. I will build small, testable pieces of code and do
tests on their functionality while building towards the bigger project.
I will document my work, and that will become my lesson plan.
*/

/****************
Overview
I'm going to start off by sending a signal that has one bit of information.
This is based off of a television remote.

**Protocol:**
All signals are sent on a 38 kHz carrier wave. All the signals are multiples
of the X, where X is a unit of time that ensure reliable transmission and
receiving of the IR signal.

Start: three times X
1: two times X
0: X
Space between signals: X
End: four times X         //so if the signal is too long, I'll stop receiving

**The timing of your protocol is empirically determined. Miles tag uses
600 microseconds. The maximum a signal can be timed using ints is 32,000.
I tested sending a 30,000 microsec signal, and I could see it, so the
maximum that my protocol length can be is 7,500 microseconds.

My IR receivers will go low for a 30,000 microsecond 
signal and have about a maximum of 10% error in the timing that I receive.
(aka, if I send a 600 microsec signal, I will see around a 540 microsec
signal. This was derived empirically [via testing])

Current Protocol (microseconds):
0: 600
1: 1200
Start: 1800
End: 2400
Error: +/- 300

OR

0: 750
1: 1500
Start: 2250
End: 3000
Error: +/- 375


There are Y bits sent, which corresponds to Z number of possible players

**Checksum**
I will not have a checksum. If the protocol is having trouble, then I will add
a checksum. But it's harder to build with a checksum. So I won't start with one
I could also have the person send the data twice, and if you get the signal
twice, then it's valid.

I call the data that is being sent the packet.

The receiver that I am using is active low. So receiving a signal is low.
*/


/*******
Libraries
*****/
#include <TimerOne.h>
//the Timer 1 library makes it easy to control hardware timer 1 on the arduino.

#include "pitches.h"
//the pitches library that defines musical notes' frequency

/*****************
Variables and Constants
******************/

//******** Inputs and outpus (these could be bytes to save memory...)
const int button_pin = 11;                 //button input is on pin 11. Active high.
const int ir_LED_pin = 10;                  //IR LED is on pin 10
//IR LED has to be on pin 10 or 9 for Timer1
const int ir_receiver_pin_1 = 9;             //IR receiver 1 on pin 9. Active low.
const int ir_receiver_pin_2 = 8;             //IR receiver 2 on pin 8. Active low.
const int ir_receiver_pin_3 = 7;             //IR receiver 3 on pin 7. Active low.
const int ir_receiver_pin_4 = 6;             //IR receiver 4 on pin 6. Active low.
const int ir_receiver_pin_5 = 4;             //IR receiver 5 on pin 4. Active low.
const int hit_LED_pin = 3;                   //status LED pin, turn on when hit
const int status_LED_pin = 5;                //status LED pin, for general info
const int piezo_pin = 2;                     //piezo

//WHEN ADDING LED's - add them to the handler as well

//****LED constants
const int blink_time = 1000;                 //time for one blink of LED in ms
const int blink_time_fast = 200; 
const int blink_time_base = 1;              //fastest time you will change LED
const int num_of_LEDs = 2;                             //number of LED's. For LED_handler function

//****ABILITY constants/cooldowns
const int NUM_ABILITIES = 2;                //number of abilities you have
int ability_cooldown_array[NUM_ABILITIES] = {100, 1000};  //cooldown for those abilities

//the constants below make my code easier to understand. This is me telling the compiler
//what these words mean so that I can write in English tha tmakes sense to me, and the
//compiler will translate that to numbers (which the arduino functions I wrote can understand)
#define TAG_ABILITY 0                 //tag ability is set to ability 0 (this could be a #define)
#define SET 1                         //I use 1 to set cooldowns and LED blinks
#define CHECK 0                       //I use 0 to check cooldowns and LED blinks

/* Notes: Abilities:
0: firing cooldown
1: primary ability 1

*/

//******** protocol definitions
const long protocol_duration = 600;         //length of time a bit is send according to protocol in microseconds
const int timeout = 30;                      //timeout in milliseconds. If i don't receive aything, I'm done
//for this to work: tag_length = tag_ID_length + tag_UAM_length + tag_damage_length
const int tag_length = 3;                    //number of bits in the tag
const int tag_ID_length = 2;                 //number of bits in the tag that are for the tag ID (first)
//const int tag_team_length = 2;               //number of bits in the tag that are for team ID
//const int tag_damage_length = 2;             //number of bits in the tag that are for tag damage
const int tag_UAM_length = 1;                //number of bits in the tag that are for the unique attack modifier
//Note: when you add team and damage, you need to change I am tagged, send_tag, and Base_handler functions

const long protocol_start = 3*protocol_duration;           //start signal
const long protocol_one = 2*protocol_duration;             // 1
const long protocol_zero = protocol_duration;              // 0
const long protocol_end = 4*protocol_duration;             // end signal

//when I make this game with bases, my ID can be given by the base
//for now, I enter my ID in binary below
int tag_sent_array[tag_length] = {0, 1, 0};            //this sets the tag you send
int tag_ID_array[tag_ID_length] = {0, 1};              // this sets the tag ID in binary
//int tag_team_array[tag_team_length] = {0, 1};              // this sets the team number in binary (0 for base)
//int tag_damage_array[tag_damage_length] = {0, 1};          // this sets the tag damage in binary
int tag_UAM_array[tag_ID_length] = {0};                // this sets the tag unique attack modifier in binary
int tag_received_array[tag_length];                    // array for tags I receive
int debug_array[2][tag_length + 2];                    //for debugging how long of a signal I got
//I have two methods of calculating time hit at the moment. I'm using the better one and can delete the old one.

//********** Other constants


//************ receiving definitions
const int RECEIVING_DELAY = 5;             //this is the delay in microseconds between reading the signal
//not sure I need this delay, but I use it in my receiving a tag code

//const int receiving_error = 0;
//using digital read, I don't need this.
//when I used port manipulation instead of digital read,
//I got about a 10% error in the amount of time that it took to go through my code, so I'm adding that back in
//this is empirically derived from having my receiver send a signal, then see how long i timed for.

//******GLOBAL VARIABLES*****
int HEALTH = 5;         //this is the number of times i can be hit before you can't tag
int MAX_HEALTH = 5;     //this is the max health that you can have (you spawn with max health)
//health is an int, so goes from 32,000 to -32,000. Hopefully i don't need to care about overflow...
int BASE_HEALTH = 20;     //this is the health of the base
int MAX_BASE_HEALTH = 20; //this is the max health of the base
int BASE_ID = 1;          //this is the team that the base is on



/****************
DEBUGGING
*****************/
const int serial_debug = 0;            //Make this 0 for no serial debugging information, 1 for serial debugging
const int LED_debug = 0;               //Make this 0 if you want the indicator LED to act normally
                                       //otherwise it will blink if the unit receives a bad code
const int I_AM_A_BASE = 0;             //Make this 0 if you are a player, 1 if you are a base


void setup(){
 Timer1.initialize(26);            //start a timer with a period of 26 microseconds, or about 38.5 kHz
 //not sure if this is necessary, but hey, why not? 
 pinMode(button_pin, INPUT); 
 pinMode(ir_receiver_pin_1, INPUT);
 pinMode(ir_receiver_pin_2, INPUT);
 pinMode(ir_receiver_pin_3, INPUT);
 pinMode(ir_receiver_pin_4, INPUT);
 pinMode(ir_receiver_pin_5, INPUT);
 pinMode(ir_LED_pin, OUTPUT);
 pinMode(hit_LED_pin,OUTPUT);
 pinMode(status_LED_pin, OUTPUT);
 pinMode(piezo_pin, OUTPUT);
 if (serial_debug) Serial.begin(9600);   
 if (serial_debug){
  delay(1000);
  Serial.print("tag to send is: ");
  int i;
  for(i=0; i < tag_length; i++){
    Serial.print(tag_sent_array[i]);
    Serial.print(", "); 
    }
   Serial.println("");
   Serial.print("Protocol duration: ");
   Serial.println(protocol_duration);
   delay(500);
   //you need to blink every LED to set the variables for them
   //delay(500);
   }
  LED_handler(hit_LED_pin, 2, blink_time_fast, blink_time_fast, SET);
  LED_handler(status_LED_pin, 97, blink_time_base, blink_time_base, SET); 
  Cooldown_Handler(TAG_ABILITY, ability_cooldown_array[TAG_ABILITY], SET);
  Cooldown_Handler(1, ability_cooldown_array[1], SET);
  if(I_AM_A_BASE) Base_Handler_Function(999);   //if I'm a base, start the base up!
 }
 
 
 /******************
 LOOP DA LOOP
 ******************/
 void loop()
 {
   //first thing I need to do is check to see if I'm getting tagged
   //I am going to call the function that checks everything for each pin I have a
   //receiver attached to
   check_if_tagged(ir_receiver_pin_1);
   check_if_tagged(ir_receiver_pin_2);
   check_if_tagged(ir_receiver_pin_3);
   check_if_tagged(ir_receiver_pin_4);
   check_if_tagged(ir_receiver_pin_5);
   
   //now that I've checked to see if I'm being tagged, I need to check to see
   //if I'm trying to tag someone else
   check_if_tagging();
   
   //now I need to check to see if I need to turn off my LED's
   LED_handler(hit_LED_pin, 0, 0, 0, CHECK);
   LED_handler(status_LED_pin, 0, 0, 0, CHECK);
   
   /*
   if (serial_debug){
    Serial.println("looping"); 
    delay(500);
   }
   */
   if(I_AM_A_BASE) Base_Handler_Function( 0 );
 }
 
 /******
 check_if_tagged
 checks to see if I am getting hit on a certain pin, calls the correct
 functions if I am, and calls the functions to see what was received,
 and what to do when I am hit.
 inputs: pin to look at
 outputs: void (it will use global arrays)
 ********/
 
 void check_if_tagged(int pin_to_check){
   int ir_receiver = digitalRead(pin_to_check);   //read in the value of the IR receiver
   int error;                                     //the error value coming from reading a tag
   if (ir_receiver == 0){                        //my receiver is active low, so it's 0 if on
     error = tag_function(pin_to_check);      //read tag, outputs the result, clears arrays and handles debug info.
     //if I want to change my output to remember who tagged me, this is where I would do it. I would add a
     //tagged_me output here from the tag function
     if(error == 0){                             //a 0 means I've been tagged                    
       //do something now that you've been hit. Like deactivate and wait until you spawn at the base.
       i_am_tagged();
       }
    }
    //return nothing. Not sure I need to do this, but hey, my function is over
   return;
 }

/*******
i_am_tagged()
This function deals with getting tagged. It is called when you are tagged.
It deals with global variables such as health. All global variables should be
in all caps. I'll get to that sometime.
inputs: void
outputs: void
********/
void i_am_tagged(void){
 //well, you got tagged. So what happens to you?
 //first, figure out who tagged you, how much damage, and what unique attack modifier
 int tagged_me = get_tag_ID( 0, tag_ID_length - 1);   
 int unique_attack_modifier = get_tag_ID(tag_ID_length, tag_ID_length + tag_UAM_length - 1);
 
 if(unique_attack_modifier == 0){
   HEALTH = HEALTH--;                       //programmer speak for health = health - 1;
   LED_handler(hit_LED_pin, 2, blink_time, 0, SET);                                   //blink LED 
   if(HEALTH > 0){
     //if I have health, dim status LED to reflect current health
     LED_handler(status_LED_pin, 99, blink_time_base*HEALTH, blink_time_base*(MAX_HEALTH - HEALTH), SET);   
     }
   else{
     digitalWrite(status_LED_pin, LOW);                          //turn status LED off
     LED_handler(status_LED_pin, 0, 0, 0, SET);
     }
 }
 else if(unique_attack_modifier == 1){
   HEALTH = MAX_HEALTH;                             //if I'm hit by the base, regain health
   LED_handler(status_LED_pin, HEALTH*2 + 1, blink_time_fast, blink_time_fast, SET);     //blink status of health
   }
   
 //make a sound now that I'm hit
 Piezo_Handler(1);   //1 is code for i was hit
   
 
 //for now, I just want to print out who tagged me
 if(serial_debug){
   Serial.print("Who tagged me: ");
   Serial.println(tagged_me);
   Serial.print("Unique Attack Modifier: ");
   Serial.println(unique_attack_modifier);
   Serial.print("Health: ");
   Serial.println(HEALTH);
   Serial.println(" ");
 }
 
 //now I need to clear out who tagged me, since I'm done using it
 int i;
 for(i=0; i < tag_length; i++){
  tag_received_array[i] = 0; 
 }
}

/*************
check_if_tagging()
This function checks to see if I am trying to tag someone, then calls the appropriate functions
inputs: none
outputs: none
***************/
void check_if_tagging( void ){
  int button_state= digitalRead(button_pin);                 //variable to keep track of button state
  int button_changed = change_state_checker( button_state ); //variable to see if button was just pressed
  //if I don't have health, then I really don't need to check anything
  if(serial_debug){
   if(button_changed){
     Serial.println("Checking if I can tag");
     delay(500);
   }
  } 
    
  if(button_changed){                              //if I'm trying to fire
     //if I have health, and tagging is off cooldown
     if (HEALTH > 0 && Cooldown_Handler(TAG_ABILITY, 0, CHECK) ){
         send_tag();                               //run tagging code
         //reset cooldown for tagging
         Cooldown_Handler(TAG_ABILITY, ability_cooldown_array[TAG_ABILITY], SET);
         
         //now I want to set my LED high, but not use a delay, so that I can have the code do other
         //stuff, so I use a function that blinks LED's and keeps track of when to turn them on/off
         LED_handler(hit_LED_pin, 2, blink_time, 0, SET);
         LED_handler(status_LED_pin, 1, blink_time_fast, blink_time_fast, SET);
         
         //make a sound
         Piezo_Handler(0);   //0 is code for I am sending a tag
       }
     else{
       //if I try to fire and don't have health, do something
       
       Piezo_Handler(2);   //2 is piezo code for can't tag
       
       }
   }  
}

/**************
send_tag
sends the ID number and the checksum according to the protocol
This outputs the correct sequence of pulses to the pin that has the IR LED
inputs: None. this uses the global arrays:
tag_ID_array, tag_damage_array, tag_UAM_array
outputs: outputs onto ir_LED_pin the correct procol to send a tag
*****************/
void send_tag(void){
  //first, I need to set up my tag_sent_array with the correct tag to send
  
  //code you need: take the integers, and bitwise combine them to make 1 a long
  //in your for loop, pick off the bits that you want (1 at a time) and save them
  //into your tag array
  //the tag array is: ID bits, Team bits, Damage bits, UAM bits.
  //shift ID bits by team bits # + damage bits # + UAM bits
  //shift team bits by damage bits + UAM bits
  //shift damage bits by UAM bits
  //then bitwise or them all together to form a long
  //this will mean max bits sent is 32.
  //you can build the array backwards, right shifting the int evey time and picking off
  //the last bit using the AND function and doing long & 0x0001.
  
  
  for (int i=0; i < tag_length; i++){
    if(i < tag_ID_length){
      tag_sent_array[i] = tag_ID_array[i];
      }
    else if(i < tag_ID_length + tag_UAM_length){
      tag_sent_array[i] = tag_UAM_array[i - tag_ID_length];
      }
    }
  if (serial_debug){
    Serial.print("tag to send is: {");
    int i;
    for(i=0; i < tag_length; i++){
      Serial.print(tag_sent_array[i]);
      Serial.print(", "); 
      }
    Serial.println("}");
  }
  /*
  unsigned long microsec_debug_start;
  unsigned long microsec_debug;
  microsec_debug_start = micros();  //save current microseconds time
  */
  //I'm changing this to send bits out of an array
  send_pulse(protocol_start);                   //send a low signal (delay in microseconds)
  //microsec_debug = micros() - microsec_debug_start;    //save difference in microseconds
  delayMicroseconds(protocol_duration);         //protocol delay, like the protocol dictates
  for (int i=0; i < tag_length; i++){           //for each bit in the tag_ID_Array
    if (tag_sent_array[i] == 0){                // if the bit is a 0, send a 0
      send_pulse(protocol_zero);                //I'm sending a 0
    }
    else if (tag_sent_array[i] == 1){           //if the bit is a 1, send a 1
      send_pulse(protocol_one);                 //I'm sending a 1
      }
   delayMicroseconds(protocol_duration);        //after each bit, wait (as the protocol dictates)
   }
   //after the loop, I have sent start, all of my bits, and a delay. Now I need to send the end command
   send_pulse(protocol_end);
   delayMicroseconds(protocol_duration);
   //I was seeing an echo, I saw my own tag that I sent out. I'm adding a delay at the end
   //so that I won't see my own tag reflecting off of stuff and coming back to me.
   if (serial_debug){
    Serial.println("Tag sent");
    /*
    Serial.print("Protocol duration (start): ");
    Serial.println(protocol_start);
    Serial.print("Sent microseconds (start): ");
    Serial.println(microsec_debug);
    */
    }
 }
 
 /**********
send_pulse
sends an IR signal to a receiver, making the receiver low
input is the amount of time to send the signal in microseconds
The output is writing the IR_LED pin high and low for the pulse duration

Note:   The micros() function uses interrupts to update. In order to send a pulse
  and use the micros() function, I need to enable inerrupts in the send_pulse()
  function, which may screw up the timing of that function because it is then
  interrupted every 4 microseconds.
**********/
void send_pulse(long pulse_duration){
  Timer1.pwm(ir_LED_pin, 512);           //send pulse
  delayMicroseconds(pulse_duration);     //delay for the time being
  Timer1.disablePwm(ir_LED_pin);         //turn off PWM
  /*
  I am commenting this code and switching to timers
  //according to Ladyada's tutorial, I need a 38 kHz signal, which is 13 microseconds high and 13 microseconds low
  //digitalWrite takes about 3 microseconds
  cli();     //turns off interrupts. We can't have the program interrupt this part
  
  while (pulse_duration > 0){
   digitalWrite(ir_LED_pin, HIGH);
   delayMicroseconds(10);
   digitalWrite(ir_LED_pin, LOW);
   delayMicroseconds(10);
   pulse_duration -= 26;           //I have used 26 microseconds of time
  }
  
  sei();     //enable interrupts after while loop is done
  */
}

/*******
tag_function()
This function is called when I think I'm getting tagged.
It goes through all serial debugging, outputs, etc. for checking and reacting
to being tagged.
inputs: IR_pin receiving tag (int)
outputs: error code,
         any serial debugging information
         any LED debugging information
********/
int tag_function(int IR_Pin){
  //I got some weird outputs, so I'm using an internal error code instead
  //of using the same name as global error. I'm not sure I should use this
  //global variable thing.
  
  int error = read_tag(IR_Pin);               //read tag outputs an int that lets me know if I got a good tag
  if (serial_debug){                          //if I'm doing a serial debug, send info out
         Serial.println(" ");
         Serial.print("tag received on pin ");
         Serial.println(IR_Pin);
         Serial.print("Error: ");
         Serial.println(error);
         int i;
         for(i=0; i < tag_length + 2; i++){
           /*
           Serial.print("Microseconds of pulse (my timing): ");
           Serial.print(debug_array[0][i]);
           */
           Serial.print(" with micros() ");
           Serial.println(debug_array[1][i]);
           Serial.print("Maps to: ");
           /*
           Serial.print(my_map(debug_array[0][i]));
           Serial.print(", ");
           */
           Serial.println(my_map(debug_array[1][i]));
           delay(50);
         }
         Serial.print("Tag received: ");
         for(i=0; i < tag_length; i++){
          Serial.print( tag_received_array[i] );
          Serial.print(", ");
         }
         Serial.println(" ");
     
  }
  if(LED_debug){
       //if I'm in this If, I want to know what error I got.
       //So far, my errors are: (see tag function below)
       /*
        0: valid tag
        5: Didn't receive a start bit first
        6: received too many bits
        7: didn't receive enough bits
        8: timout"
        */
        switch (error){
         case 0:
           break;
         case 5:
           blink_LED(hit_LED_pin, 500, 2);
           //this will blink the hit_LED_pin with a delay of 500 twice
           break;
         case 6:
           blink_LED(hit_LED_pin, 500, 3);
           break;
         case 7:
           blink_LED(hit_LED_pin, 500, 4);
           break;
         case 8:
           blink_LED(hit_LED_pin, 500, 5);
           break;
         default:
          break; 
          
        }
     }
  
       //now I need to clear any data that I may have written into my global arrays
       //since I am done using them, and want to be ready for the next tag
       int i;
       for(i=0; i < tag_length + 2; i++){
        debug_array[0][i] = 0;
        debug_array[1][i] = 0; 
       }
  
  return error;     
}

/********************
read_tag()
uses the protocol for messages to decode the tagger's ID number
This function is called when I start receiving a tag
inputs: pin to read (integer)
outputs: an integer, which is one of the following:
  0: valid tag
  5: Didn't receive a start bit first
  6: received too many bits
  7: didn't receive enough bits
  8: timout
It also changes the tag_received_array with the tag i've got
This function uses the read_protocol helper function to read what was sent
********************/
int read_tag(int IR_Pin){
  int ir_receiver;                          //this is used to keep track of what the IR receiver state is
  int debug_index = 0;                      //index to track of which protocol signal I'm on
  int ir_receiver_got = 0;                  //local variable to track of the last communication I got
  ir_receiver_got = read_protocol(IR_Pin, debug_index);   //see what the IR sent
  debug_index++;                                 //the read_protocol function stores the time it
                                                 //saw something in the debug aray at [debug_index]
                                                 //I increment the index so I can store the next piece
  if(ir_receiver_got == 2) {
    //if I got the begin signal, then I can begin reading in data 
    //I will read data until I'm past my array, I get an end signal
    int index = 0;                      //keeps track of my idex in my tag ID array
    unsigned long time = millis();      //to keep track of the time in milliseconds (millis() is 32 bit)
                                        //this is used to see if I should time out
    while (true){
     //first, I need to get some digital input
     ir_receiver = digitalRead(IR_Pin);
     //next, see if it's 0 (active low)
     if(ir_receiver == 0){
       //if I'm starting to get a piece of information, get it
       ir_receiver_got = read_protocol(IR_Pin, debug_index);
       debug_index++;
       //now I got to see what I've got
       if(ir_receiver_got == 0 || ir_receiver_got == 1){
        //If I got a 1 or a 0, I need to store it to my array
        //but only if I'm not past my index
        if(index != tag_length) {
          tag_received_array[index] = ir_receiver_got;
          index++;                                          //add one to index
        }
        else{
          //if index == tag_length, I'm passed my array. What's going on here. Abort!!!
          return 6;           //alert the code! I've got an error
        }
       }
       else if (ir_receiver_got == 3){ 
         //I'm being told to stop
        if(index == tag_length){
         //I've filled my array with data, so it should be a valid tag
        return 0;
        }
        else{
         return 7;    //I did not fill up my array for a received tag. That's an error (too few bits)
        }
       }
     }
     //If my IR receiver is not 0, then I'm going to wait for it...forever? no. I need a timeout
     if((millis() - time) > timeout){
       return 8;         //timed out
     }
    }
  }
  //So, if I get here, then I didn't get a 2, so I didn't see a valid tag.
  return 5;
  //Note: If I get a 4, the my_map function failed. See below.
}

/***************
read_protocol
This function is called to determine what was sent, either a 0, 1, start or end signal over IR.
Inputs: Pin to read, the index for the debug array that I write to
Outputs: 0,1,2,3,4 for a 0, 1, start, end signal, or error.
If I wait for longer than an end signal, I will return an end signal
****************/
int read_protocol(int Pin_Read, int i) {
 int ir_receiver = 0;                          //keeps track of what state ir_receiver is (starts = 0 since
                                               //I'm getting tagged when I call this function
 int count = 0;                                //this counts how many times I will have my delay
 int unsigned long microsec_start = micros();  //this counts microseconds using a timer
 while (ir_receiver == 0){
   //count++;
   //while the receiver is still receiving a signal 
   delayMicroseconds(RECEIVING_DELAY);         //delay for a bit. you know. you can't check constantly
   ir_receiver = digitalRead(Pin_Read);        //see if the IR receiver pin has changed
   //digital read takes about 3 microseconds
 }
   //now that I have the time that I waited for, I need to return which protocol singal it was
   //I use my_map function for that
   
   //I also need some debugging here to see what's going on
   //debug_array[0][i] = count*(receiving_delay + 3 + receiving_error);
   debug_array[1][i] = micros() - microsec_start;
   //using micros is more consistent than my timing function, so I will use
   //that instead.
   return my_map( debug_array[1][i] );
 //this uses my map function which allows a signal to be half a protocol duration in length away from what it is
 //supposed to be and still calls it that value.  
}


/****************
get_tag_ID
This is only called after I have received a tag and stored it into tag_received_array.
tag_received_array is the ID of my tagger (no start or stop codes)
This outputs an integer with the ID (or value) of a specific part of the tag
It builds the integer from the back of the tag (starts with the 1's position)
and moves towards the 2's, 4's, etc. position to build an integer.
inputs: start and stop parts of debug_array to build an int from (int)
output: integer build from the bits in debug array specified
*****************/
int get_tag_ID(int array_start, int array_stop){
  int output = 0;
  int i;
  for (i = array_start; i <= array_stop; i++){
    output = output << 1;                        //bitshift left to multiply by 2 and shift all bits
    output = output + tag_received_array[i];     //add new bit
    /* don't need serial debug anymore
    if(serial_debug){
      Serial.print("i: ");
      Serial.println(i);
      Serial.print("tag received array: ");
      Serial.println(tag_received_array[i]);
      Serial.print("output: ");
      Serial.println(output);
      delay(500);
    }
    */
  }
 return output;
}

/**********
change_state_checker
checks to see if the button has already been pressed
Outputs 1 if button changes from 0 to 1
outputs 0 otherwise
***********/
int change_state_checker(int current_state){
  static int last_state;
  
  if (current_state != last_state){     //if it has changed
     if (last_state == 0){
       if (serial_debug){
       Serial.println("button pressed");
       delay(500);                          //I need to add a delay to let Serial keep up
       }
      last_state = current_state;          //save the change in the current state
      return 1;     //if it changed from 0 to 1, output 1
     }
     
     if (last_state == 1){
       //if it changed from 1 to 0, output is already 0
       if (serial_debug){
        Serial.println("button released");
        //delay(500);
       }
     }
  }
  /* I don't need the debugging below anymore
  if(serial_debug){
   Serial.print("last_state: ");
   Serial.println(last_state);
   Serial.print("current_state: ");
   Serial.println(current_state); 
   delay(2000);
  }
  */
  last_state = current_state;          //reset last_state to current_state
  return 0;                            //return 0 if I didn't get a change from 0 to 1
 }

/**********
my_map
maps the time in microseconds to one of my signals using the amount of error I want.
it uses less than, so its [ ), or inclusive on the lower end of the range.
input: the time in microseconds
output: the protocol singal, either 0, 1, 2 or 3. 
        If there is an error, i will return 4
**********/
int my_map( int time_in ){
  int i;
  for(i = 1; i < 5; i++){
    //I start with i=1 so I can have 1 protocol duration
   if( (i*protocol_duration - (protocol_duration/2) ) < time_in && 
       time_in < (i*protocol_duration + (protocol_duration/2) ) ){
        //if my time is greather than my protocol minus half and it is
       // less than my protocol plus half, then I'm going to say I'm
       // at that protocol
       return i - 1;
       }
  }
  return 4;
}

/********
blink_LED
This allows me to more easily write code to blink an LED a certain number of times.
input: Pin LED is on (int),
       delay time (int),
       number of blinks (int),
outputs: Will blink the LED using standard delays. Void output to program
*******/
void blink_LED(int blink_LED_pin, int delay_time, int num_blinks){
  int i;
  for(i = 0; i < num_blinks; i++){
    digitalWrite(blink_LED_pin, HIGH);
    delay(delay_time);
    digitalWrite(blink_LED_pin, LOW);
    delay(delay_time);
  }
}

/*******
set_LED_status
This function will turn off the status LED in a set period of time, using the millis()
function. This will check to see whether or not I need to turn on/off an LED
intput: set_LED_pin is the pin the LED is on (int)
        time_set is the time to turn off LED from now in milliseconds (int)
        blinks is the number of times to cycle LED (2 = on and then off)
           blinks = 0 if you are checking to see if you should blink
output: direct control of the LED pin
*******
void set_LED( int set_LED_pin, unsigned long time_set, int blinks){
 //so, I need an internal variable that keeps track of the current time for each LED
 static unsigned long LED_off_time;
 static int num_blinks;
 static int blink_time;
 
 //if I'm setting the time, set it.
 if (blinks > 0){
  LED_off_time = millis() + time_set;    //set the time to turn off
  change_LED(set_LED_pin);               //change state of LED
  num_blinks = blinks - 1;               //save blinks to go (minus first)
  blink_time = time_set;                 //save time in between blinks
  if(serial_debug){
    Serial.println("set blinking");
    }
 }
 else{
   //if I'm not setting time, I'm checking time
   if(num_blinks > 0){
    if(millis() > LED_off_time){
      //I'm checking time, and it is time to switch the input
      change_LED(set_LED_pin);               //change state of LED
      //now that I've changed the pin state, I need to record that in my variables
      num_blinks -= num_blinks;        //decrement number of blinks
      //if number of blinks is greater than 0, then set up to blink again
      if(num_blinks > 0){
        LED_off_time = millis() + blink_time;
        if(serial_debug) Serial.println("more blinks");
        }
     else{
       if(serial_debug) Serial.println("no more blinks");
       }   
      }
   }
     //i'm done checking if I"m supposed to switch time at these parenthesis 
  }
 }
 */
 

/******
LED_handler
This function will keep track of the following:
Time that every LED needs to switch state
Number of times the LED needs to switch state
Duration that each LED needs to switch state for
It will use this information to call the appropriate function to check each LED.
Those functions will turn LED's on and off
It will be called every loop, or to set data for certain pins
intpus: handler_LED_pin - (int) the LED pin that I am manipulating
        handler_blinks - (int) the number of times that pin should blink (if = 99, blinks infinitely)
        handler_blink_time_on - (int) time in milliseconds that the LED should blink on for
        handler_blink_time_off - (int) time in milliseconds that the LED should blink off for
        set_LED - (int) whether to set variables or check (1 to set variables, 0 to check LED)
outputs: changes output of LED's
*****/

void LED_handler( int handler_LED_pin, int handler_blinks, int handler_blink_time_on, int handler_blink_time_off,
                  int set_LED){
 static unsigned long LED_change_time[num_of_LEDs]; //keep track of time to turn all LED's on
 static int LED_blinks[num_of_LEDs];                //keeps track of the number of blinks
 static int LED_blink_time_on[num_of_LEDs];         //keeps track of time to blink LED's
 static int LED_blink_time_off[num_of_LEDs];        //keeps track of time to blink LED's
 int i;                                             //internal variable for each LED
 int changed_LED;                                   //variable to see if I changed LED, 1 if changed, 0 if not
 
 //figure out which pin I'm working with, and use that one
 switch(handler_LED_pin){
  case hit_LED_pin:
    i = 0;
    break;
  case status_LED_pin:
    i = 1;
    break;
  default:
    i = 1;
    break;
 }
 
 //if I'm setting variables, then set them
 if(set_LED){
   LED_change_time[i] = millis();                        //set time to change LED
   LED_blinks[i] = handler_blinks;                       //set number of blinks
   LED_blink_time_on[i] = handler_blink_time_on;         //set time between blinks
   LED_blink_time_off[i] = handler_blink_time_off;       //set time between blinks
   
   if(serial_debug){
     Serial.print("set LED ");
     Serial.println(i);
     delay(50);
   }
   if(LED_blinks[i] != -1){
     //if I am not blinking infinitely, start at LED off
     digitalWrite(handler_LED_pin, LOW);                   //reset LED to beginning
     //turn on LED after appropriate time
     LED_change_time[i] = millis() + LED_blink_time_off[i];
     }
 }
 else{
 //if I'm not setting, then I'm checking
 //first, see if I should change the state
 if(LED_blinks[i] > 0){
   //first, call a function to check to see if I need to switch the LED state (on or off)
   changed_LED = check_LED( handler_LED_pin, LED_change_time[i]);
   //this function checks, and changes, the LED state if it is time to
     
   //if I changed the LED, I need to update the times
   if(changed_LED == 1){
      //if changed_LED ==1, I went from low to high, so stay on for change_time_on
      LED_change_time[i] = millis() + LED_blink_time_on[i];
      if(LED_blinks[i] != 99) LED_blinks[i] = LED_blinks[i] - 1;      //decrement number of blinks
      //I'm adding an unsigned long and an int...is arduino ok with that? Yes.
      }
    else if(changed_LED == 2){
      //changed_LED == 2, so I went from high to low, so stay off for change_time_off
      LED_change_time[i] = millis() + LED_blink_time_off[i];
      if(LED_blinks[i] != 99) LED_blinks[i] = LED_blinks[i] - 1;      //decrement number of blinks
    }
    
    
    if(serial_debug){
     Serial.println("I switched LED states");
     Serial.print("Number of blinks left ");
     Serial.println(LED_blinks[i]); 
     delay(50);
     }
    
    }
  }
 }
 //I'm done hanglind LED's  
 //when millis() overflows, this code will keep a light on until you reset it. but hey, 
 //that'll happen almost never.
 //unless you don't turn the unit off for over a month (50 days ish)


/*******
check_LED
This will check to see whether or not I need to turn on/off an LED and do so if need be
intput: set_LED_pin - (int) is the pin the LED is on (int)
        time_switch - (unsigned long) is the time to turn off LED in milliseconds
output: 0 if I did not change the pin
        1 if I did change the pin
*******/
int check_LED( int set_LED_pin, unsigned long time_switch){
  //see if I have passed the time
  if(millis() > time_switch){
    return change_LED(set_LED_pin);               //change state of LED;
  }
 else{
   return 0;
   }   
 }

 /*****
 change_LED
 changes the state of an LED on a pin
 inputs: pin to manipulate
 outputs: 1 if changed pin from Low to High
          2 if changed pin from high to low
******/
int change_LED(int LED_pin){
 //get the current state
 int state = digitalRead(LED_pin); 
 if(state){
   //if the LED is HIGH
   digitalWrite(LED_pin, LOW);
   return 2;
 }
 else{
  //otherwise, turn LED HIGH
  digitalWrite(LED_pin, HIGH);
  return 1; 
 }
}

/******
Cooldown_Handler
This function handles cooldowns for abilities. 
It keeps track of when you can use an ability/tag
intpus: handler_ability - (int) the ability you are checking/setting
        handler_cooldown - (int) cooldown for ability
        set_cooldown - (int) reset cooldown (1 to reset, 0 to check cooldown)
outputs: 0 - (int) can't use ability
         1 - (int) ability off cooldown and can use.
Notes: Abilities:
0: tagging
1: primary ability
******/

int Cooldown_Handler(int handler_ability, int handler_cooldown, int set_cooldown){
 static unsigned long cooldown_time[NUM_ABILITIES];   //keep track of time cooldown is off for each ability
 
 if(handler_ability > NUM_ABILITIES - 1){
   if(serial_debug){
     Serial.print("invalid ability");
     delay(50);
     }
   return 0;      //if I'm not checking an actual ability, stop
   }
 
 //if I'm setting variables, then set them
 if(set_cooldown){
   cooldown_time[handler_ability] = millis() + handler_cooldown;    //set cooldown time for ability
   }
 else{
 //if I'm not setting, then I'm checking
 if(millis() > cooldown_time[handler_ability]){
   return 1;
   }
 else{
   return 0;
   }
 }
} 


/*******
Piezo_Handler
This function controls all of the piezo sounds
input: sound to play (integer)
output: sound on the Piezo
Sounds:
  0: send tag
  1: received tag
  2: cannot tag
*******/

void Piezo_Handler(int sound){
  switch(sound){
  case 0:    //send tag sound
      for (int i = 4000; i < 5000; i++){
         tone(piezo_pin, i);
         delay(5);
         noTone(piezo_pin);
         i = i + 250;
       }
       
       delay(10);
       tone(piezo_pin, 5000);
       delay(25);
       noTone(piezo_pin);
       
       for (int i = 5000; i > 2000; i--){
         tone(piezo_pin, i);
         delay(5);
         noTone(piezo_pin);
         i = i - 250;
       }
      break;
  case 1:    //received tag sound
     tone(piezo_pin, NOTE_D6);
     delay(25);
     noTone(piezo_pin);
     
     delay(10);
     tone(piezo_pin, NOTE_D4);
     delay(25);
     noTone(piezo_pin);
     
     delay(10);
     tone(piezo_pin, NOTE_D8);
     delay(25);
     noTone(piezo_pin);
     
     delay(10);
     tone(piezo_pin, NOTE_D4);
     delay(25);
     noTone(piezo_pin);
     break;
  case 2:    //cannot tag sound
     tone(piezo_pin, NOTE_A3);
     delay(25);
     noTone(piezo_pin);
     
     delay(10);
     tone(piezo_pin, NOTE_A2);
     delay(75);
     noTone(piezo_pin);   
     break;
  default:
    break;
 }
}

/****
Base_Handler_Function()
This function handles all of my base interactions. For now, it is going to send out a heal tag
every 250 milliseconds. If team = 999, then I start sending out signals.
inputs: team person who tagged base is on (int)
outputs: heal tag out of the IR LED.
****/
void Base_Handler_Function( int team){
  static unsigned long send_ack_tag;
  static unsigned long base_heal;
  
  //if I receive team = 999, then I start sending signals
  if (team == 999){
    send_ack_tag = 0;
    base_heal = 0;
  }
  
  //now, I need to see if I need to send a signal out, which I do every 250 ms
  if(millis() > send_ack_tag){
    //to send a tag as a base, my ID is 0
    tag_ID_array[0] = 0;
    tag_ID_array[1] = 0;
    //my unique attack modifier is 1
    tag_UAM_array[0] = 0;
    //now I need to send the tag
    send_tag();
    //the send tag function will build a tag out of the ID and UAM arrays
    
    //now, I need to reset my timer to send this tag in another 250 ms
    send_ack_tag = millis() + 250;
  }
  
  //my base passively heals every 10 seconds
  if(millis() > base_heal){
    if(BASE_HEALTH != MAX_BASE_HEALTH) BASE_HEALTH++;
    base_heal = millis() + 10000;
  }
  
}

//insert coded stuff here, you know :)
