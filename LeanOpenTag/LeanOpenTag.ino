/*gukropina 
March 18, 2015
Open Tag Developement

Booyah

Current work:

1. Adding a hit LED and a status LED. I need to update the function that
checks the LED's status and update the code that calls it.

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


/*
Degugging notes:
(These are old notes)
Serial data is being sent normally.
The send pulse function is working.
The send_tag function checks for new button presses correctly
*/

/*******
Libraries
*****/
#include <TimerOne.h>
//the Timer 1 library makes it easy to control hardware timer 1 on the arduino.

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

//******** protocol definitions
const long protocol_duration = 600;         //length of time a bit is send according to protocol in microseconds
const int timeout = 30;                      //timeout in milliseconds. If i don't receive aything, I'm done
const int tag_length = 2;                    //number of bits in the tag

const long protocol_start = 3*protocol_duration;           //start signal
const long protocol_one = 2*protocol_duration;             // 1
const long protocol_zero = protocol_duration;              // 0
const long protocol_end = 4*protocol_duration;             // end signal

//when I make this game with bases, my ID can be given by the base
//for now, I enter my ID in binary below
int tag_ID_array[tag_length] = {0, 1};                  // this will give me tag_length bits
int tag_received_array[tag_length];                    // array for tags I receive
int debug_array[2][tag_length + 2];                    //for debugging how long of a signal I got
//I have two methods of calculating time hit at the moment. I'm using the better one and can delete the old one.

//********** Other constants
const unsigned long blink_time = 1000;                 //time for one blink of LED in ms

//************ receiving definitions
const int receiving_delay = 5;             //this is the delay in microseconds between reading the signal
//not sure I need this delay, but I use it in my receiving a tag code

//const int receiving_error = 0;
//using digital read, I don't need this.
//when I used port manipulation instead of digital read,
//I got about a 10% error in the amount of time that it took to go through my code, so I'm adding that back in
//this is empirically derived from having my receiver send a signal, then see how long i timed for.

//******GLOBAL VARIABLES*****
int HEALTH = 5;         //this is the number of times i can be hit before you can't tag
//health is an int, so goes from 32,000 to -32,000. Hopefully i don't need to care about overflow...


/****************
DEBUGGING
*****************/
const int serial_debug = 1;            //Make this 0 for no serial debugging information, 1 for serial debugging
const int LED_debug = 1;               //Make this 0 if you want the indicator LED to act normally
                                       //otherwise it will blink if the unit receives a bad code



void setup(){
 Timer1.initialize(26);            //start a timer with a period of 26 microseconds, or about 38.5 kHz
 //not sure if this is necessary, but hey, why not? 
 pinMode(button_pin, INPUT); 
 pinMode(ir_LED_pin, OUTPUT);
 pinMode(ir_receiver_pin_1, INPUT);
 pinMode(ir_receiver_pin_2, INPUT);
 pinMode(ir_receiver_pin_3, INPUT);
 pinMode(ir_receiver_pin_4, INPUT);
 pinMode(ir_receiver_pin_5, INPUT);
 pinMode(hit_LED_pin,OUTPUT);
 pinMode(status_LED_pin, OUTPUT);
 if (serial_debug) Serial.begin(9600);   
 if (serial_debug){
   delay(1000);
  Serial.print("tag to send is: ");
  int i;
  for(i=0; i < tag_length; i++){
   Serial.print(tag_ID_array[i]);
   Serial.print(", "); 
  }
   Serial.println("");
   Serial.print("Protocol duration: ");
   Serial.println(protocol_duration);
  delay(500);
  } 
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
   set_LED(hit_LED_pin, 0, 0);
   set_LED(status_LED_pin, 0, 0);
   /*
   if (serial_debug){
    Serial.println("looping"); 
    delay(500);
   }
   */
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
 int tagged_me = get_tag_ID( 0, tag_length - 1);   // if I got a good tag, get an int of who tagged me?
 HEALTH = HEALTH--;                       //programmer speak for health = health - 1;
 set_LED(hit_LED_pin, blink_time, 1);     //blink LED 
 
 //for now, I just want to print out who tagged me
 if(serial_debug){
   Serial.print("Who tagged me: ");
   Serial.println(tagged_me);
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
    
  if(HEALTH > 0){
     if (button_changed){          //see if button changed state
     //if so, I am pushing the button for the first time, do something
      send_tag();                       //run tagging code
      //now I want to set my LED high, but not use a delay, so that I can have the code do other
      //stuff
      set_LED(hit_LED_pin, blink_time, 1);
      set_LED(status_LED_pin, blink_time, 1);
     }
   }
}

/**************
send_tag
sends the ID number and the checksum according to the protocol
This outputs the correct sequence of pulses to the pin that has the IR LED
*****************/
void send_tag(void){
  /*
  The micros() function uses interrupts to update. In order to send a pulse
  and use the micros() function, I need to enable inerrupts in the send_pulse()
  function, which may screw up the timing of that function because it is then
  interrupted every 4 microseconds.
  unsigned long microsec_debug_start;
  unsigned long microsec_debug;
  microsec_debug_start = micros();  //save current microseconds time
  */
  //I'm changing this to send bits out of an array
  send_pulse(protocol_start);                   //send a low signal (delay in microseconds)
  //microsec_debug = micros() - microsec_debug_start;    //save difference in microseconds
  delayMicroseconds(protocol_duration);         //protocol delay, like the protocol dictates
  for (int i=0; i < tag_length; i++){           //for each bit in the tag_ID_Array
    if (tag_ID_array[i] == 0){                  // if the bit is a 0, send a 0
      //I'm sending a 0
      send_pulse(protocol_zero);
    }
    else if (tag_ID_array[i] == 1){             //if the bit is a 1, send a 1
      //I'm sending a 1
      send_pulse(protocol_one);
      }
   delayMicroseconds(protocol_duration);               //after each bit, wait (as the protocol dictates)
   }
   //at the end, I have sent start, all of my bits, and a space. Now I need to send the end command
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
   count++;
  //while the receiver is still receiving a signal 
   delayMicroseconds(receiving_delay);         //delay for a bit. you know. you can't check constantly
   ir_receiver = digitalRead(Pin_Read); //see if the IR receiver pin has changed
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
set_LED
This function will turn off an LED in a set period of time, using the millis()
function. This will check to see whether or not I need to turn on/off an LED
intput: set_LED_pin is the pin the LED is on (int)
        time_set is the time to turn off LED from now in milliseconds (int)
        start_time is whether or not to start timing from here or not (1 for starting, 0 for not)
output: direct control of the LED pin
*******/
void set_LED( int set_LED_pin, unsigned long time_set, int start_time){
 //so, I need an internal variable that keeps track of the current time for each LED
 static unsigned long LED_off_time_status;
 static unsigned long LED_off_time_hit;
 unsigned long LED_off_time;
 
 
 //if I'm setting the time, set it.
 if (start_time){
  LED_off_time = millis() + time_set;
  digitalWrite(set_LED_pin, HIGH);
  //save the time into the correct variable
    switch (set_LED_pin){
             case 0:
               break;
             case hit_LED_pin:
               LED_off_time_hit = LED_off_time;
               break;
             case status_LED_pin:
               LED_off_time_status = LED_off_time;
               break;
             default:
              LED_off_time_status = LED_off_time;
              break; 
            }
 }
 else{
   //if I'm not setting time, I'm checking time
   //pick the correct time to use
     switch (set_LED_pin){
         case 0:
           break;
         case hit_LED_pin:
           LED_off_time = LED_off_time_hit;
           break;
         case status_LED_pin:
           LED_off_time = LED_off_time_status;
           break;
         default:
          LED_off_time = LED_off_time_status;
          break; 
        } 

  if(millis() > LED_off_time){
   digitalWrite(set_LED_pin, LOW); 
  }
 }
 //when millis() overflows, this code will keep a light on until you reset it. but hey, 
 //that'll happen almost never.
 //unless you don't turn the unit off for over a month (50 days ish)
 return;
}


//insert coded stuff here, you know :)
