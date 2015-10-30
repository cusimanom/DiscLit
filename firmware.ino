// libraries
#include <Adafruit_NeoPixel.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// hardware configuration
#define BUTTONPIN 1
#define NEOPIN 2
#define PIEZO_PIN_1 3
#define PIEZO_PIN_2 4

// standard color definitions
#define RED 255,0,0
#define ORANGE 255,75,0
#define YELLOW 255,255,0
#define GREEN 0,255,0
#define BLUE 0,0,255
#define INDIGO 100,0,255
#define VIOLET 255,0,100
#define WHITE 255,255,255
#define BLACK 0,0,0

//Neopixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, NEOPIN, NEO_GRB + NEO_KHZ800);

//Logic Flags
bool powerFlag = false;//flag for power off mode

bool buttonFlag = false;//flag for button handling

bool waitFlag = false;//flag for game start wait

bool gameFlag = false;//flag for game mode

bool customFlag = false;//flag for customization loop

bool findFlag = false;//flag for findme mode

//Global Variables
bool LEDMode = true;  //LEDs on or off
bool buzzerMode = true;   //buzzer on or off
bool lightsReady = false;  // for single-set lighting in loop structures - set light if false
int stripBrightness = 250;  // global brightness

void setup() {
  //pin configuration
  pinMode(PIEZO_PIN_1, OUTPUT);
  pinMode(PIEZO_PIN_2, OUTPUT);
  pinMode(BUTTONPIN, INPUT);
  pinMode(NEOPIN, OUTPUT);
  
  //neopixel setup
  strip.begin();
  strip.setBrightness(stripBrightness);
  strip.show();
  
  //startup show on first power connection
  startupDisplay();
  
  //go to sleep
  sleepNow();
}

//Interrupt subroutine
ISR(PCINT0_vect) {
  //wait until the button is released
  do{}while(digitalRead(BUTTONPIN) == HIGH);
  
  //do the startup display on the release of the button
  startupDisplay();
}

void loop() {  
  //check for shutdown conditions
  if(powerFlag) powerOff();
  
  //read the button state
  buttonFlag = digitalRead(BUTTONPIN);
  
  //button handling
  if(buttonFlag && !waitFlag && !gameFlag) buttonHandler();
  
  //game mode
  if(gameFlag) gameOn();
  
  //Game Start Wait 
  if(waitFlag) startWait();  
  
  //customize mode
  //if(customFlag) customize();
}



//gametime LED handling
void gameLED() {}

//settings customization
void customizeBrightness() {}

void customizeLoudness() {}


//piote note sequence (loooong-short-short-short)
// 105hZ (65/165 split) = 105 c/s = 1/105s period = 0.0105s period = 105 microsecond delay
// 3/100 minute
// (784hZ/1174hz split)
// 0.25/100 minute
// 1319hz
// 0.25/100 minute
// silent
// 0.25/100 minute
// 523hZ/784hZ split
// 2/100 minute


//flashy colorful startup
void startupDisplay() {
  do{}while(digitalRead(BUTTONPIN) == HIGH);
  
  fillStrip(RED);      EZNote(90,250);
  fillStrip(ORANGE);   EZNote(80, 250);
  fillStrip(YELLOW);   EZNote(70, 500);
  fillStrip(GREEN);    EZNote(140, 250);
  fillStrip(BLUE);     EZNote(130, 250);
  fillStrip(INDIGO);   EZNote(110, 250);
  fillStrip(VIOLET);   EZNote(100, 250);
  fillStrip(WHITE);    EZNote(124, 250);
  
  fillStrip(BLACK);
  
  if(digitalRead(BUTTONPIN) == HIGH) buttonHandler();
}


// handles button presses
void buttonHandler() {
  int counter = 0;
  long lastHeld = 0;
  
  //while the button is pressed or while the timer has not elapsed (after a sufficient length of time pressed)
  while( (digitalRead(BUTTONPIN) == HIGH) || (millis() <  (lastHeld + 3000))) {
    //do some stuff only while the button is held
    if(digitalRead(BUTTONPIN) == HIGH) {    
      counter++;
      delay(10);
      lastHeld = millis();
    }
    
    //handle the mode flags
    switch(counter) {
      case 100: fillStrip(RED);      waitFlag = true;    powerFlag = false;   LEDMode = true;   buzzerMode = true;    break;
      case 200: fillStrip(GREEN);    waitFlag = true;    powerFlag = false;   LEDMode = false;  buzzerMode = true;    break;
      case 300: fillStrip(BLUE);     waitFlag = true;    powerFlag = false;   LEDMode = true;   buzzerMode = false;   break;
      case 400: fillStrip(WHITE);    waitFlag = false;   powerFlag = true;    LEDMode = true;   buzzerMode = true;    break;
      default:                                                                                                        break;
    }
    
    //reset the counter when the button has been released
    if(digitalRead(BUTTONPIN) == LOW) counter = 0;
  }
  
  //shut off the lights after data has been collected
  fillStrip(BLACK);
}

//waits for the game to start after the mode has been selected
void startWait() {
  //delay 
  delay(100);
  
  // check the button status and update the game mode if pressed
  if(digitalRead(BUTTONPIN) == HIGH) {
    //No longer waiting, start playing
    waitFlag = false;
    gameFlag = true;
    
    //reset local lighting
    lightsReady = false;
  }
  
  //if not, set lights to red. Only do it once, and only if LED mode is activated.
  if(LEDMode && !lightsReady && !gameFlag) {
    fillStrip(RED);
    lightsReady = true;
  } 
  
  //beep the beeper if beeper mode is activated
  if(buzzerMode && !gameFlag) EZNote(28, 100);  
}

//game mode  
void gameOn() {
  long timer = 0;
  
  //run the normal game mode 
  while(!findFlag && gameFlag) {
    //iterate the counter
    timer++;
    delay(10);
    
    //run the LED pattern if LEDMode is selected and the button is not pressed (yielding LED control to buttonHandler while pressed so that the UI works)  
    if(LEDMode) {
      strip.setPixelColor(0, wheel(timer));
      strip.setPixelColor(1, wheel(timer + 75));
      strip.setPixelColor(2, wheel(timer + 150));
      strip.setPixelColor(3, wheel(timer + 225));
      strip.show();
    }
    else fillStrip(BLACK);
    
    //break when the timer elapses
    if(timer > 10000) { //5000 = 45 seconds
      findFlag = true;
      timer = 0;
      fillStrip(BLACK);
      break;
    }
    
    //handle power off conditions
    if((digitalRead(BUTTONPIN) == HIGH) && (timer > 50)) EZPower();
    if(!gameFlag) break;
  }
  
  //initiate findMe mode
  if(findFlag) findMe();  
}

//locator mode
void findMe() {
  //if the findMe flag has been activated
  while(findFlag) {
    
    //one iteration for every spoke on the wheel
    for(int x = 0; x < 255; x ++) {
      
      //set the strip to carious spokes on the wheel on even iterations
      if((LEDMode) && ((x%2) == 0) ) {
        strip.setPixelColor(0, wheel(x));
        strip.setPixelColor(1, wheel(x));
        strip.setPixelColor(2, wheel(x));
        strip.setPixelColor(3, wheel(x));
        strip.show();
      }
      
      //fill the strip white on odd iterations
      else if(LEDMode) fillStrip(WHITE);
      
      //beep on even flashes when the beeper mode is activated
      if(((x % 2) == 0) && buzzerMode) EZNote(124,200);
      
      delay(100);
      
      //
      if(digitalRead(BUTTONPIN) == HIGH) {
        findFlag = false;
        break;
      }
      
      if(!findFlag) break;
    }
  }
}

//shutdown mode
void powerOff() {
  //turn the lights red
  fillStrip(RED);
  
  //play a tune
  EZNote(500,1000);
  
  //housekeeping
  buttonFlag = false;
  waitFlag = false;
  gameFlag = false;
  powerFlag = false;
  customFlag = false;
  findFlag = false;

  
  //blink three times
  for(int x = 0; x < 3; x++) {
    fillStrip(BLACK);
    delay(500);
    fillStrip(RED);
    delay(500);
  }
  fillStrip(BLACK);
  
  //go to sleep
  sleepNow();
}

//Easily check for power off conditions outside of buttonHandler. Call from within buttonFlag equivalent conditions.
void EZPower() {
  long long powerCounter = 0; 
  fillStrip(RED);
  do {
    powerCounter++;
    if(powerCounter > 400000) {
      powerCounter = 0;
      powerOff();
    }
  } while(digitalRead(BUTTONPIN) == HIGH);    
}

//set fuses and go to sleep
void sleepNow() {
  GIMSK |= _BV(PCIE);                      // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT1);                    // Use PB1 as interrupt pin
  ADCSRA &= ~_BV(ADEN);                    // ADC off
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);     // replaces above 

  sleep_enable();                          // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                   // Enable interrupts
  sleep_cpu();                             // sleep

  cli();                                   // Disable interrupts
  PCMSK &= ~_BV(PCINT1);                   // Turn off PB1 as interrupt pin
  sleep_disable();                         // Clear SE bit
  ADCSRA |= _BV(ADEN);                     // ADC on

  sei();                                   // Enable interrupts
} 

//Illuminate the strip a single color
void fillStrip(int red, int green, int blue) {
  //one iteration for each LED
  for(int x = 0; x < 4; x++) {
    strip.setPixelColor(x, red, green, blue);
  }
  strip.show();
}

//compatible color wheel
uint32_t wheel(byte WheelPos) {
  if(WheelPos < 85) return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//Simple call for softPWM
void EZNote(int freq, int duration) {
  softPWM(PIEZO_PIN_1, PIEZO_PIN_2, freq, duration);
}

//Software PWM (Square Wave, 1/2 Duty Cycle)
void softPWM(int pin_1, int pin_2, int freq, int duration) { // software PWM function that fakes analog output
  //Note the start time for the timer
  long startTime = millis();
  
  //square wave generator
  do{
    digitalWrite(pin_1,HIGH);
    digitalWrite(pin_2,LOW);
    delayMicroseconds(freq);
    digitalWrite(pin_1,LOW);
    digitalWrite(pin_2,HIGH);
    delayMicroseconds(freq);
  } while ( millis() < (startTime + duration) );
}
