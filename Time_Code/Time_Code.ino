
/*  Features:
    - Easy to navigate menu
    - Overview of feed times, current time, feeding comletions,
      and feeding portion on the main screen
    - Accurate time keeping with DS1307 chip
    - Can manually change set time in DS1307 chip
    - Two feedings per day
    - Manual feeding option
    - Feeding restart in case of power outtage
    - LED indication of real time clock
    - Feeding times and completions safetly store in EEPROM
    
*/

#include <JC_Button.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LiquidMenu.h>
#include <RTClib.h>
#include <EEPROM.h>




// Assigning all my input and I/O pins
#define ENTER_BUTTON_PIN 11
#define UP_BUTTON_PIN 10
#define DOWN_BUTTON_PIN 9
#define BACK_BUTTON_PIN 8
#define POWER_LED_PIN 5
#define MANUAL_BUTTON_PIN A1
#define MOTOR_RELAY 6

// Defining all the Buttons, works with the JC_Button library
Button enterBtn (ENTER_BUTTON_PIN);
Button upBtn (UP_BUTTON_PIN);
Button downBtn (DOWN_BUTTON_PIN);
Button backBtn (BACK_BUTTON_PIN);
Button manualFeedBtn (MANUAL_BUTTON_PIN);

// Defining LCD I2C and RTC
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;

//Variables used throughout the code


int Hour;
int Minute;
int portion;
int feed_time1_hour;
int feed_time1_min;
int feed_time2_hour;
int feed_time2_min;
int userInput;
int ledState = LOW;
int count;


const long interval_delay = 1000;

unsigned long rotationTime = 400;
unsigned long led_previousMillis = 0;
unsigned long delay_interval = 2000;
unsigned long blink_previousMillis = 0;
unsigned long blink_currentMillis = 0;
unsigned long blink_interval = 500;
unsigned long delay_currentMillis = 0;
unsigned long delay_previousMillis = 0;

boolean manualFeed = false;
boolean blink_state = false;
boolean feeding1_complete = false;
boolean feeding2_complete = false;
boolean feeding3_complete = false;
boolean feeding4_complete = false;
boolean feeding1_trigger = false;
boolean feeding2_trigger = false;
boolean feeding3_trigger = false;
boolean feeding4_trigger = false;

boolean relayOn = true;  /////////////////////////////////////////  DO i NEED THIS???????????
}

/* I use enums here to keep better track of what button is
    being pushed as opposed to just having each button set to
    an interger value.
*/
enum {
  btnENTER,
  btnUP,
  btnDOWN,
  btnBACK,
};

/* States of the State Machine. Same thing here with the enum
   type. It makes it easier to keep track of what menu you are
   in or want to go to instead of giving each menu an intreger value
*/
enum STATES {
  MAIN,
  MENU_EDIT_FEED1,
  MENU_EDIT_FEED2,
  MENU_EDIT_TIME,
  MENU_EDIT_PORTION,
  EDIT_FEED1_HOUR,
  EDIT_FEED1_MIN,
  EDIT_FEED2_HOUR,
  EDIT_FEED2_MIN,
  EDIT_HOUR,
  EDIT_MIN,
  EDIT_PORTION
};

// Holds state of the state machine
STATES state;

//User input variables


// Special character check mark
byte check_Char[8] = {
  B00000,
  B00000,
  B00001,
  B00011,
  B10110,
  B11100,
  B11000,
  B00000
};
//======================The Setup===========================

void setup() {
  Wire.begin();
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, check_Char);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
  }

  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //The buttons
  enterBtn.begin();
  upBtn.begin();
  downBtn.begin();
  backBtn.begin();
  manualFeedBtn.begin();

  //Setting up initial state of State Machine
  state = MAIN;

  //Setting up inputs and outputs
  pinMode(POWER_LED_PIN, OUTPUT);


  
 
  // default state of LEDs
  digitalWrite(POWER_LED_PIN, HIGH);
 

  /* These functions get the stored feed time, completed feeding,
     and portion size from EEPROM on start-up. I did this because I get random
     power outtages here where I live.
  */
  get_feed_time1();
  get_feed_time2();
  get_completed_feedings();
  get_portion();

}

//======================The Loop===========================
void loop() {
  changing_states();

  check_buttons();

  check_feedtime ();

  check_rtc();

  manual_feed_check ();
}

//=============== The Functions =======================

/* Uses the JC_Button Library to continually check if a button
   was pressed. Depending on what button is pressed, it sets the
   variable userInput to be used in the fucntion menu_transitions
*/
void check_buttons () {
  enterBtn.read();
  upBtn.read();
  downBtn.read();
  backBtn.read();
  manualFeedBtn.read();

  if (enterBtn.wasPressed()) {
    Serial.println ("You Pressed Enter!");
    userInput = btnENTER;
    menu_transitions(userInput);
  }
  if (upBtn.wasPressed()) {
    Serial.println ("You Pressed Up!");
    userInput = btnUP;
    menu_transitions(userInput);
  }
  if (downBtn.wasPressed()) {
    Serial.println ("You Pressed Down!");
    userInput = btnDOWN;
    menu_transitions(userInput);
  }
  if (backBtn.wasPressed()) {
    Serial.println ("You Pressed Back!");
    userInput = btnBACK;
    menu_transitions(userInput);
  }
  if (manualFeedBtn.wasPressed()) {
    Serial.println ("You Are Manually Feeding!");
    manualFeed = true;
  }
}
//=====================================================

/* This funcion determines what is displayed, depending on what menu
    or "state" you are in. Each menu has a function that displays the
    respective menu
*/
void changing_states() {

  switch (state) {
    case MAIN:
      display_current_time();
      display_feeding_times();
      display_portion();
      break;

    case MENU_EDIT_FEED1:
      display_set_feed_time1_menu();
      break;

    case MENU_EDIT_FEED2:
      display_set_feed_time2_menu();
      break;

    case MENU_EDIT_FEED3:
      display_set_feed_time3_menu();
      break;

    case MENU_EDIT_FEED4:
      display_set_feed_time4_menu();
      break;

    case MENU_EDIT_TIME:
      display_set_time_menu();
      break;

    case MENU_EDIT_PORTION:
      display_set_portion_menu ();
      break;

    case EDIT_FEED1_HOUR:
      set_feed_time1();
      break;

    case EDIT_FEED1_MIN:
      set_feed_time1();
      break;

    case EDIT_FEED2_HOUR:
      set_feed_time2();
      break;

    case EDIT_FEED2_MIN:
      set_feed_time2();
      break;

    case EDIT_FEED3_HOUR:
      set_feed_time3();
      break;

    case EDIT_FEED3_MIN:
      set_feed_time3();
      break;

    case EDIT_FEED4_HOUR:
      set_feed_time4();
      break;

    case EDIT_FEED4_MIN:
      set_feed_time4();
      break;

    case EDIT_HOUR:
      set_the_time();
      break;

    case EDIT_MIN:
      set_the_time();
      break;

  }
}
//=====================================================
/* This is the transitional part of the state machine. This is
   what allows you to go from one menu to another and change values
*/
void menu_transitions(int input) {

  switch (state) {
    case MAIN:
      if (input == btnENTER) {
        lcd.clear();
        state = MENU_EDIT_FEED1;
      }
      if (input == btnBACK)
     
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_FEED1:
      if (input == btnBACK) {
        lcd.clear();
        state = MAIN;
      }
      else if (input == btnENTER) {
        lcd.clear();
        state = EDIT_FEED1_HOUR;
      }
      else if (input == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_FEED2;
      }
      
      break;
    //----------------------------------------------------
    case EDIT_FEED1_HOUR:
      // Need this to prevent motor going off while setting time
      relayOn = false;
      if (input == btnUP) {
        feed_time1_hour++;
        if (feed_time1_hour > 23) {
          feed_time1_hour = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time1_hour--;
        if (feed_time1_hour < 0) {
          feed_time1_hour = 23;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        relayOn = true;
        state = MENU_EDIT_FEED1;
      }
      else if (input == btnENTER) {
        state = EDIT_FEED1_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED1_MIN:
      if (input == btnUP) {
        feed_time1_min++;
        if (feed_time1_min > 59) {
          feed_time1_min = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time1_min--;
        if (feed_time1_min < 0) {
          feed_time1_min = 59;
        }
      }
      else if (input == btnBACK) {
        state = EDIT_FEED1_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        relayOn = true;
        write_feeding_time1 ();
        state = MAIN;
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_FEED2:
      if (input == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEED1;
      }
      else if (input == btnENTER) {
        lcd.clear();
        state = EDIT_FEED2_HOUR;
      }
      else if (input == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED2_HOUR:
      relayOn = false;
      if (input == btnUP) {
        feed_time2_hour++;
        if (feed_time2_hour > 23) {
          feed_time2_hour = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time2_hour--;
        if (feed_time2_hour < 0) {
          feed_time2_hour = 23;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        relayOn = true;
        state = MENU_EDIT_FEED2;
      }
      else if (input == btnENTER) {
        state = EDIT_FEED2_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_FEED2_MIN:
      if (input == btnUP) {
        feed_time2_min++;
        if (feed_time2_min > 59) {
          feed_time2_min = 0;
        }
      }
      else if (input == btnDOWN) {
        feed_time2_min--;
        if (feed_time2_min < 0) {
          feed_time2_min = 59;
        }
      }
      else if (input == btnBACK) {
        state = EDIT_FEED2_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        relayOn = true;
        write_feeding_time2 ();
        state = MAIN;
      }
      break;
    //----------------------------------------------------
    case MENU_EDIT_TIME:
      if (input == btnUP) {
        lcd.clear();
        state = MENU_EDIT_FEED2;
      }
      else if (input == btnENTER) {
        lcd.clear();
        state = EDIT_HOUR;
      }
      else if (input == btnDOWN) {
        lcd.clear();
        state = MENU_EDIT_PORTION;
      }
      break;
    //----------------------------------------------------
    case EDIT_HOUR:
      if (input == btnUP) {
        Hour++;
        if (Hour > 23) {
          Hour = 0;
        }
      }
      else if (input == btnDOWN) {
        Hour--;
        if (Hour < 0) {
          Hour = 23;
        }
      }
      else if (input == btnBACK) {
        lcd.clear();
        state = MENU_EDIT_TIME;
      }
      else if (input == btnENTER) {
        state = EDIT_MIN;
      }
      break;
    //----------------------------------------------------
    case EDIT_MIN:
      if (input == btnUP) {
        Minute++;
        if (Minute > 59) {
          Minute = 0;
        }
      }
      else if (input == btnDOWN) {
        Minute--;
        if (Minute < 0) {
          Minute = 59;
        }
      }
      else if (input == btnBACK) {
        state = EDIT_HOUR;
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        rtc.adjust(DateTime(0, 0, 0, Hour, Minute, 0));
        state = MAIN;
      }
      break;
    //----------------------------------------------------
  
      }
      else if (input == btnENTER) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print( "*Settings Saved*");
        delay(1000);
        lcd.clear();
        write_portion();
        state = MAIN;
      }
      break;
  }
}
//=====================================================
// This function checks the feed time against the current time

void check_feedtime ()
{
  DateTime now = rtc.now();
  if (now.second() == 0) {
    if ((now.hour() == feed_time1_hour) && (now.minute() == feed_time1_min))
    {
      feeding1_trigger = true;
      if (relayOn)
      {
        if (feeding1_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print ("Dispensing");
          lcd.setCursor(1, 1);
          lcd.print("First Feeding");
          startFeeding();
        }
      }
    }
    else if ((now.hour() == feed_time2_hour) && (now.minute () == feed_time2_min))
    {
      feeding2_trigger = true;
      if (relayOn)
      {
        if ( feeding2_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print ("Dispensing");
          lcd.setCursor(0, 1);
          lcd.print("Second Feeding");
          startFeeding();
        }
      }
    }

    else if ((now.hour() == feed_time3_hour) && (now.minute () == feed_time3_min))
    {
      feeding3_trigger = true;
      if (relayOn)
      {
        if ( feeding3_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print ("Dispensing");
          lcd.setCursor(0, 1);
          lcd.print("Third Feeding");
          startFeeding();
        }
      }
    }
     else if ((now.hour() == feed_time4_hour) && (now.minute () == feed_time4_min))
    {
      feeding4_trigger = true;
      if (relayOn)
      {
        if ( feeding4_complete == false)
        {
          lcd.clear();
          lcd.setCursor(3, 0);
          lcd.print ("Dispensing");
          lcd.setCursor(0, 1);
          lcd.print("Fourth Feeding");
          startFeeding();
        }
      }
    }
  }
  
  // Midnight Reset
  if ( (now.hour() == 0) && (now.minute() == 0))
  {
    feeding1_complete = false;
    feeding2_complete = false;
    feeding3_complete = false;
    feeding4_complete = false;
    EEPROM.write(8, feeding1_complete);
    EEPROM.write(9, feeding2_complete);
    EEPROM.write(10, feeding3_complete);
    EEPROM.write(11, feeding4_complete);
  }
  /*If power outtage happens during a feed time, this checks to see if the
     feed time has passed and if the feeding occurred. If not, it feeds.
  */
  if ( (now.hour() >= feed_time1_hour) && (now.minute() > feed_time1_min))
  {
    if ((feeding1_complete == 0) && (feeding1_trigger == 0))
    {
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print ("Uh-Oh!");
      lcd.setCursor(2, 1);
      lcd.print("Power Outage");
      startFeeding();
    }
  }
  if ( (now.hour() >= feed_time2_hour) && (now.minute() > feed_time2_min))
  {
    if ((feeding2_complete == 0) && (feeding2_trigger == 0))
    {
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print ("Uh-Oh!");
      lcd.setCursor(2, 1);
      lcd.print("Power Outage");
      startFeeding();
    }
  }
    if ( (now.hour() >= feed_time3_hour) && (now.minute() > feed_time3_min))
    {
      if ((feeding3_complete == 0) && (feeding3_trigger == 0))
      {
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print ("Uh-Oh!");
        lcd.setCursor(2, 1);
        lcd.print("Power Outage");
        startFeeding();
      }
    }
     if ( (now.hour() >= feed_time4_hour) && (now.minute() > feed_time4_min))
  {
    if ((feeding4_complete == 0) && (feeding4_trigger == 0))
    {
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print ("Uh-Oh!");
      lcd.setCursor(2, 1);
      lcd.print("Power Outage");
      startFeeding();
    }
  }
}

//=====================================================
// Displays the set portion menu option

//=====================================================
// Displays the menu where you change the current time
void set_the_time ()
{
  lcd.setCursor(2, 0);
  lcd.print("Set the Time");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(Hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(Minute);
      break;
    //----------------------------------------------------
    case EDIT_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(Hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(Minute);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}
//=====================================================

//=====================================================
//Displays the menu option for setting the time
void display_set_time_menu () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(2, 1);
  lcd.print("Set the Time");
}
//=====================================================
// Displays the menu where you change the second feeding time
void set_feed_time1 ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 1");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_FEED1_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(feed_time1_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time1_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED1_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time1_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(feed_time1_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}


void set_feed_time2 ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 2");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_FEED2_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(feed_time2_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time2_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED2_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time2_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(feed_time2_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}

void set_feed_time3 ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 3");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_FEED3_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(feed_time3_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time3_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED3_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time3_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(feed_time3_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}

void set_feed_time4 ()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Feed Time 4");
  switch (state)
  {
    //----------------------------------------------------
    case EDIT_FEED4_HOUR:

      if (blink_state == 0)
      {
        lcd.setCursor(5, 1);
        add_leading_zero(feed_time4_hour);
      }
      else
      {
        lcd.setCursor(5, 1);
        lcd.print("  ");
      }
      lcd.print(":");
      add_leading_zero(feed_time4_min);
      break;
    //----------------------------------------------------
    case EDIT_FEED2_MIN:
      lcd.setCursor(5, 1);
      add_leading_zero(feed_time4_hour);
      lcd.print(":");
      if (blink_state == 0)
      {
        lcd.setCursor(8, 1);
        add_leading_zero(feed_time4_min);
      }
      else
      {
        lcd.setCursor(8, 1);
        lcd.print("  ");
      }
      break;
  }
  blinkFunction();
}
//=====================================================
// Displays the menu where you change the first feeding time

//=====================================================
// Adds a leading zero to single digit numbers
void add_leading_zero (int num) {
  if (num < 10) {
    lcd.print("0");
  }
  lcd.print(num);
}
//=====================================================
/* Displays the feeding time on the main menu as well as the
   check mark for visual comfirmation of a completed feeding
*/
void display_feeding_times () {
  //Displaying first feed time
  lcd.setCursor(0, 0);
  lcd.print ("F1:");
  add_leading_zero(feed_time1_hour);
  lcd.print(":");
  add_leading_zero(feed_time1_min);
  lcd.print(" ");
  if (feeding1_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }
  //Displaying second feed time
  lcd.setCursor(0, 1);
  lcd.print("F2:");
  add_leading_zero(feed_time2_hour);
  lcd.print(":");
  add_leading_zero(feed_time2_min);
  lcd.print(" ");
  if (feeding2_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }

   //Displaying second feed time
  lcd.setCursor(0, 1);
  lcd.print("F3:");
  add_leading_zero(feed_time3_hour);
  lcd.print(":");
  add_leading_zero(feed_time3_min);
  lcd.print(" ");
  if (feeding3_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }

   //Displaying second feed time
  lcd.setCursor(0, 1);
  lcd.print("F4:");
  add_leading_zero(feed_time4_hour);
  lcd.print(":");
  add_leading_zero(feed_time4_min);
  lcd.print(" ");
  if (feeding4_complete == true)
  {
    lcd.write(0);
  }
  else
  {
    lcd.print(" ");
  }
}
//=====================================================
// Displays the current time in the main menu
void display_current_time () {
  DateTime now = rtc.now();
  lcd.setCursor(11, 0);
  add_leading_zero(now.hour());
  lcd.print(":");
  add_leading_zero(now.minute());
}
//=====================================================
// Displays the menu option for setting the first feed time
void display_set_feed_time1_menu () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0, 1);
  lcd.print("Set Feed Time 1");
}
//=====================================================
// Displays the meny option for setting the second feed time
void display_set_feed_time2_menu () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0, 1);
  lcd.print("Set Feed Time 2");
}

void display_set_feed_time3_menu () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0, 1);
  lcd.print("Set Feed Time 3");
}

void display_set_feed_time4_menu () {
  lcd.setCursor(2, 0);
  lcd.print("Menu Options");
  lcd.setCursor(0, 1);
  lcd.print("Set Feed Time 4");
}
//=====================================================
// Displays the feeding portion in the main menu

//=====================================================
// Starts the feeding process.

void startFeeding()
{
  digitalWrite(motorrelay, HIGH);
   delay(3000);
   
    
  // Keeps track of which feeding just happened and writes it to EEPROM
  if ((feeding1_complete == false) && (feeding2_complete == false)&& (feeding3_complete == false)&& (feeding4_complete == false))
  {
    feeding1_complete = true;
    EEPROM.write(8, feeding1_complete);
  }
  else if ((feeding1_complete == true) && (feeding2_complete == false)&&(feeding3_complete == false)&& (feeding4_complete == false))
  {
    feeding2_complete = true;
    EEPROM.write(9, feeding2_complete);
  }
   else if ((feeding1_complete == true) && (feeding2_complete == true)&&(feeding3_complete == false)&& (feeding4_complete == false))
  {
    feeding3_complete = true;
    EEPROM.write(10, feeding3_complete);
  }
   else if ((feeding1_complete == true) && (feeding2_complete == true)&&(feeding3_complete == true)&& (feeding4_complete == false))
  {
    feeding4_complete = true;
    EEPROM.write(11, feeding4_complete);
  }
  digitalWrite(motorrelay,LOW);
 
  
  lcd.clear();
  delay_currentMillis = millis();
  while (millis() - delay_currentMillis <= delay_interval)
  {
    lcd.setCursor(2, 0);
    lcd.print ("Feeding Done");
  }
  lcd.clear();
}


//=====================================================
// Writes the hour and minute valies set for 1st feeding to the EEPROM

void write_feeding_time1 ()
{
  EEPROM.write(0, feed_time1_hour);
  EEPROM.write(1, feed_time1_min);
}
//=====================================================
// Writes the hour and minute values set for 2nd feeding to the EEPROM

void write_feeding_time2 () 
{
  EEPROM.write(2, feed_time2_hour);
  EEPROM.write(3, feed_time2_min);
}

void write_feeding_time3 () 
{
  EEPROM.write(4, feed_time3_hour);
  EEPROM.write(5, feed_time3_min);
}

void write_feeding_time2 () 
{
  EEPROM.write(6, feed_time4_hour);
  EEPROM.write(7, feed_time4_min);
}
//=====================================================
// Writes portion value set to the EEPROM


//=====================================================
// Reads the hour and minute values from 1st feed time from EEPROM

void get_feed_time1 ()
{
  feed_time1_hour = EEPROM.read(0);
  if (feed_time1_hour > 23) feed_time1_hour = 0;
  feed_time1_min = EEPROM.read(1);
  if (feed_time1_min > 59) feed_time1_min = 0;

}
//=====================================================
// Reads the hour and minute values from 2nd feed time from EEPROM

void get_feed_time2 ()
{
  feed_time2_hour = EEPROM.read(2);
  if (feed_time2_hour > 23) feed_time2_hour = 0;
  feed_time2_min = EEPROM.read(3);
  if (feed_time2_min > 59) feed_time2_min = 0;
}

void get_feed_time3 ()
{
  feed_time3_hour = EEPROM.read(4);
  if (feed_time3_hour > 23) feed_time3_hour = 0;
  feed_time3_min = EEPROM.read(5);
  if (feed_time3_min > 59) feed_time3_min = 0;
}
void get_feed_time4 ()
{
  feed_time4_hour = EEPROM.read(6);
  if (feed_time4_hour > 23) feed_time4_hour = 0;
  feed_time4_min = EEPROM.read(7);
  if (feed_time4_min > 59) feed_time4_min = 0;
}
//=====================================================
// Reads portion set value from EEPROM

// Reads boolean value of whether or not feedings have occured from EEPROM

void get_completed_feedings()
{
  feeding1_complete = EEPROM.read(8);
  feeding2_complete = EEPROM.read(9);
  feeding3_complete = EEPROM.read(10);
  feeding4_complete = EEPROM.read(11);
}

//=====================================================
// Checks if you push the manual feed button

void manual_feed_check () {
  if (manualFeed == true)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Manual Feeding");
    startFeeding();
    manualFeed = false;
  }
}
//=====================================================
// checks to see if RTC is running

void check_rtc () {
  if (!rtc.isrunning())
  {
    led_blink();
  }
}
//=====================================================
/* Blinks the red led when RTC has failed. 
   sensor fails
*/

void led_blink()
{
  unsigned long led_currentMillis = millis();
  if (led_currentMillis - led_previousMillis >= interval_delay)
  {
    led_previousMillis = led_currentMillis;
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }
   
  }
}
//=====================================================
// Creates the blinking effect when changing values

void blinkFunction()
{
  blink_currentMillis = millis();

  if (blink_currentMillis - blink_previousMillis > blink_interval)
  {
    blink_previousMillis = blink_currentMillis;
    blink_state = !blink_state;
  }
}
//=====================================================
