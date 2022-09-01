// Tutorial 2a. State machine traffic light with night mode switch

// Main parts: Adafruit Metro Mini, NeoPixel Rings 16 x 5050 RGB LED,
// momentary switch

// Library required to drive RGB LEDs; use the latest version
#include "FastLED.h"

// Variables that remain constant
const byte pinSwitch = 8; // Digital input pin from momentary switch

const int timeShortPress = 250; // Maximum duration of short button press
const int timeLongPress = 350; // Minimum duration of long button press

const byte pinData1 = 10; // Digital output pin to LED ring
const byte pinData2 = 11;
const byte pinData3 = 12;

const byte numLEDs = 16; // Number of LEDs per ring

CRGB ledRings[3][numLEDs]; // Declare three FastLED arrays to store each ring's LEDs data

// A struct can bundle two or more different variables (members), and
// each can be of a different data type, quite like a shopping bag can
// contain different goods. Here, the struct is named (stateVariables),
// then six variables (members) are declared, and finally the struct is
// assigned to an array, states[5], so its data can be used
struct stateVariables
{
  byte          number;         // Stores the number of each state (not really necessary here)
  char          name[11];       // Stores the name of each state for serial printing
  unsigned long interval;       // Stores the duration for each state
  byte          ring0Colour[3]; // Stores three colour triples for the first ring (0)
  byte          ring1Colour[3];
  byte          ring2Colour[3];
};

// In a state table like the one below, we can orderly store all the data
// for each state in a single convenient place, instead of declaring lots
// of constant variables above. Also, as you will see in the switch/case
// state machine below, it's easy to access the right data programmatically
// so when texts, timings or colours change, you simply change them here
stateVariables states[5] =
{
  // State number, state name, state duration, ring0 colour triple, ring1 colour triple, ring 2 colour triple
  {0, "RED",            9000, {0, 255, 192},      {0, 0, 0},      {0, 0, 0}},
  {1, "RED_YELLOW",     2000, {0, 255, 192}, {32, 255, 255},      {0, 0, 0}},
  {2, "GREEN",          6000,     {0, 0, 0},      {0, 0, 0}, {80, 255, 168}},
  {3, "YELLOW",         2000,     {0, 0, 0}, {32, 255, 255},      {0, 0, 0}},
  {4, "YELLOW_FLASHING", 600,     {0, 0, 0}, {32, 255, 255},      {0, 0, 0}}
} ; // The state table is arranged horizontally, while the struct is arranged vertically, that's how it is

// Variables that can change
byte lastSwitchState = HIGH; // Tracks the last switch state, open (= HIGH) at start

unsigned long timePressed  = 0; // Timestamp when button is pressed
unsigned long timeReleased = 0; // Timestamp when button is released

unsigned long interval = 0; // Stores duration of each state (traffic light phase)

bool normalMode = true; // Flag to remember if in normal mode
bool flashingMode = false; // Flag to remember if in flashing mode

byte systemState = 0; // Stores the current state (traffic light phase) of the state machine

void setup()
{
  // Serial printing only necessary to understand the state transitions
  // when not connecting LED rings or strips
  Serial.begin (9600);

  // Initialise momentary switch pin with an internal pull-up resistor
  // so that the momentary switch is read as open (= HIGH) at start
  pinMode (pinSwitch, INPUT_PULLUP);

  // Initialise Metro Mini's digital pins specified above
  pinMode(pinData1, OUTPUT);
  pinMode(pinData2, OUTPUT);
  pinMode(pinData3, OUTPUT);

  // Initialise the FastLED library with the type of programmable RGB LED
  // used, the digital output pin the LED ring is wired to, the array that
  // stores each ring's LEDs data, and the number of LEDs per ring
  FastLED.addLeds<NEOPIXEL, pinData1>(ledRings[0], numLEDs);
  FastLED.addLeds<NEOPIXEL, pinData2>(ledRings[1], numLEDs);
  FastLED.addLeds<NEOPIXEL, pinData3>(ledRings[2], numLEDs);

  // Turn the three LED rings off at the start
  for (byte i = 0; i < 3; i++)
  {
    // Normally, fill_solid requires CRGB(0, 0, 0) or CHSV(0, 0 , 0), but
    // for black (off), one can use a single 0 as a shortcut
    fill_solid(ledRings[i], numLEDs, 0);
  }

  // Finally, display all LED's data (= here black, 0, off)
  FastLED.show();

  // Time to clear the serial monitor
  delay (3000);
}

void loop()
{
  // A call to this function checks if the night mode button was pressed
  // and for how long it was pressed
  checkSwitch();

  // A call to this function runs the state machine, going through all
  // states (traffic light phases), and fetching the data for each from
  // the struct's state table above
  eventProcessing();
}

void eventProcessing()
{
  // Timestamp that updates every loop() iteration. The keyword static
  // means the variable is not created and destroyed with every function
  // call, but lives on and so retains its value between function calls
  static unsigned long timeNow = 0;

  // First we check if it is time to change the state. Every loop() iteration,
  // we subtract the timestamp from the current time taken with millis(),
  // and compare to the current state's duration interval, and the state of
  // the momentary switch
  if (millis() - timeNow <= interval && normalMode)
  {
    // If it wasn't time, we return to the loop. The classic millis() timer
    // approach to not use delay() that would block (stop) code execution.
    // Apart from the bit of code executed in the function we are in, the
    // main loop() function runs at nearly full speed, and so you can run
    // motors, check sensors or use WiFi more or less in parallel
    return;
  }

  // Then we check if the night mode was activated, if the momentary switch
  // was pressed (see what the checkSwitch() function does), and if it was
  // pressed for a short duration
  else if (!normalMode)
  {
    // The switch/case routine will jump directly to state 4
    systemState = 4;
  }

  // If that was not the case, we enter the switch/case based state machine.
  // Depending on the current value of system state (0, 1, 2, 3 or 4), the
  // code in the matching case statement is executed. This allows for the
  // convenient setting of variables, flags or calling of functions that do
  // something. If a case was matched, the break statement means that code
  // execution continues after the closing bracket of the switch/case block
  switch (systemState)
  {
    case 0:
      // The name of the state is fetched from the state table and printed
      Serial.println (states[systemState].name);
      // The duration of the state is fetched from the state table
      interval = states[systemState].interval;
      // A function is called that will illuminate the LED rings
      switchRings(systemState);
      // And the system's state value is incremented, from 0 to 1. With
      // the next loop() iteration, and when the time was up, we get to
      // the next case, and so, red is followed by red yellow, and so on.
      // You can, for other designs, jump to any case; you don't have to go
      // through the cases sequentially. Here, triggering a certain state
      // is simply sequential, but you could use a sensor's output or a
      // Tweet as a trigger to activate a certain state, anything goes!
      systemState++;
      break;

    case 1:
      Serial.println (states[systemState].name);
      interval = states[systemState].interval;
      switchRings(systemState);
      systemState++;
      break;

    case 2:
      Serial.println (states[systemState].name);
      interval = states[systemState].interval;
      switchRings(systemState);
      systemState++;
      break;

    case 3:
      Serial.println (states[systemState].name);
      interval = states[systemState].interval;
      switchRings(systemState);
      // We set the system's state back to 0, to restart from red
      systemState = 0;
      break;

    case 4:
      Serial.println (states[systemState].name);
      interval = states[systemState].interval;
      switchRings(systemState);
      // This case is the night mode, yellow flashing, and we can only get
      // here, if the systemState was 4, as per the initial check above. By
      // not specifying another state, we "trap" ourselves here on purpose,
      // until a long button press sets the systemState back to 0, so that
      // the traffic light sequence would restart from red
      break;
  }

  // And finally, for the time interval check above, we take the time of
  // when a case was matched
  timeNow = millis();
}

void switchRings(byte state)
{
  // If we are in normal mode, the three LED rings (0, 1 and 2) are filled
  // with the colours matching the current state (traffic light phase). The
  // colour values are fetched from the state table
  if (normalMode)
  {
    fill_solid(ledRings[0], numLEDs, CHSV(states[state].ring0Colour[0], states[state].ring0Colour[1], states[state].ring0Colour[2]));
    fill_solid(ledRings[1], numLEDs, CHSV(states[state].ring1Colour[0], states[state].ring1Colour[1], states[state].ring1Colour[2]));
    fill_solid(ledRings[2], numLEDs, CHSV(states[state].ring2Colour[0], states[state].ring2Colour[1], states[state].ring2Colour[2]));
  }

  // Otherwise, if we are not in the normal mode, meaning if we are in night
  // mode, and therefore want the middle LED ring (1) to flash yellow
  else if (!normalMode)
  {
    // We take a timestamp that updates every loop() iteration
    static unsigned long timeNow = 0;

    // And check if it is time to flip from illumination to off (0), with
    // the duration fetched from the state table
    if (millis() - timeNow >= states[systemState].interval)
    {
      // If a flash must be flashed
      if (flashingMode == true)
      {
        // We switch the red (0) and green (2) ring off and the yellow ring
        // on, with the colour values fetched from the state table
        fill_solid(ledRings[0], numLEDs, 0);
        fill_solid(ledRings[1], numLEDs, CHSV(states[state].ring1Colour[0], states[state].ring1Colour[1], states[state].ring1Colour[2]));
        fill_solid(ledRings[2], numLEDs, 0);

        // And we set the flashing mode to false, so in the next interval
        // the yellow ring will not be illuminated
        flashingMode = false;
      }

      // Otherwise
      else
      {
        // We switch the yellow ring (1) off
        fill_solid(ledRings[1], numLEDs, 0);

        // And we set the flashing mode to true, so in the next interval
        // the yellow ring will be illuminated
        flashingMode = true;
      }

      // Finally, we take a timestapm of when that happened
      timeNow = millis();
    }
  }

  // At last, we display all LED's data (= illuminate the LED rings as per
  // the normal or flashing mode settings above)
  FastLED.show();
}

void checkSwitch()
{
  // The momentary switch is hardware debounced with a 0.1uF capacitor; no
  // debouncing code is necessary. See http://www.gammon.com.au/switches
  // Read the voltage from the momentary switch pin to see if something
  // has changed (was the button pressed or released?)
  byte switchState = digitalRead (pinSwitch);

  // Has the momentary switch state changed since the last time it was
  // checked?
  if (switchState != lastSwitchState)
  {
    // Then, we test if the switch was closed (button pressed)
    if (switchState == LOW)
    {
      // And we take a timestamp of when that happened
      timePressed = millis();
    }

    // Otherwise, the switch must have been open (button released)
    else
    {
      // And we take a timestamp of when that happened
      timeReleased = millis();

      // How long did it take between pressing and releasing?
      unsigned long pressDuration = timeReleased - timePressed;

      // If it took less time than the short press duration
      if (pressDuration < timeShortPress)
      {
        // We set normal mode to false, so we will enter the night mode
        Serial.print ("Flashing mode; short press milliseconds "); Serial.println (pressDuration);
        normalMode = false;
      }
      else if (pressDuration > timeLongPress )
      {
        // Otherwise, with a long press, we set normal mode to true, so
        // we will enter the normal mode
        Serial.print ("Normal mode; long press milliseconds "); Serial.println (pressDuration);
        normalMode = true;

        // And after leaving the night mode, we want to begin with red
        systemState = 0;
      }
    }

    // Last, we store the current switch state for the next time around
    lastSwitchState = switchState;
  }
}
