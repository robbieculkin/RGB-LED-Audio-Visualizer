//master.ino
//Robbie Culkin
//Comissioned by the Dean of the Santa Clara University School of Engineering

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel
#include "visualizer.h"

// PINS & INFO
#define STRIP_PIN_0      3
#define STRIP_PIN_1      4
#define STRIP_PIN_2      5
#define STRIP_PIN_3      6
// 4 strips b/c the tower is divided into 4 sections, recieving its own data
#define N_STRIP_LEDS   232
#define LEVELS          32

//SETTINGS
#define BRIGHTNESS  100       //(0 to 255)
#define RISE_RATE     0.13    //(0 to 1) higher values mean upward changes are reflected faster
#define FALL_RATE     0.04    //(0 to 1) higher values mean downward changes are reflected faster
#define CONTRAST      1.3     //(undefined range)
#define MAX_VOL     300       //(0 to 1023) maximum value reading expected from the shield

//NeoPixel intitialization
Adafruit_NeoPixel strip0 = Adafruit_NeoPixel(N_STRIP_LEDS, STRIP_PIN_0, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(N_STRIP_LEDS, STRIP_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(N_STRIP_LEDS, STRIP_PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3 = Adafruit_NeoPixel(N_STRIP_LEDS, STRIP_PIN_3, NEO_GRB + NEO_KHZ800);

//FILTER
double smoothVol(double newVol);
// brings vol values closer together, resulting a smoother output (avoid strobing)
double autoMap(const double vol);
// scales volume values in order to get consisitent output at different overall volumes

//DISPLAY
uint32_t GetColor(byte pos, double vol);
// finds the new color value based on color rotation position and volume
void displayTower(const double vol, Adafruit_NeoPixel &strip);
// detemines what LEDs are on/off; sends RGB values to the strip

//HELPER FUNCTIONS
int findLED(int level, int place);
//returns the index of any LED on the tower given its level and position on the level
void customSetPixel(int level, int index, uint32_t color);

//GLOBALS (globals are normally a bad idea, but the nature of Arduino's loop()
//         function makes them necessary)
circularArray<double> vols = circularArray<double>(100);          // last 100 volumes recorded
circularArray<uint32_t> color = circularArray<uint32_t>(100);       // last 100 color values generated
int numLoops = 0;           // number of loops executed so far

void setup() {

  //Set default values for arrays
  for (int i = 0; i < 100; i++)
  {
    vols.addToFront(50);   //if set to 0, display would flash on. this creates a slow opening
    color.addToFront(strip1.Color(0, 0, 0));
  }

  Serial.begin(9600);

  strip0.begin();
  strip1.begin();
  strip2.begin();
  strip3.begin();

  strip0.setBrightness(BRIGHTNESS);
  strip1.setBrightness(BRIGHTNESS);
  strip2.setBrightness(BRIGHTNESS);
  strip3.setBrightness(BRIGHTNESS);
}

void loop() {
  double volume;
  static uint16_t pos = 0;
  for(int i = 0; i < 1000; i++)
  {
    // READ
    volume = micInput();
      
    // FILTER
    volume = smoothVol(volume);
    volume = autoMap(volume);
  
    //
    // DISPLAY
    //
    color.addToFront(GetColor(pos & 255, volume));
  
    //rotate the color assignment wheel
    int rotation = map (20*log(volume), 0, strip1.numPixels() / 2.0, 0, 25);
    numLoops++;
    if (numLoops % 7 == 0) //default 5
      pos += rotation;
  
    displayTower(volume);
  }

  crazy(); // last minute function proposed by the dean
  
  static int pos1 = 0, pos2 = 0;
  int i;
  
  randomSeed(analogRead(A0));
  for(i = 0; i < 10; i++)
  {
    if(pos1 <256) pos1 += random(215)+40;
    else pos1 = 0;
  // Robbie's Working up/down wipe
    up(Wheel(pos1), 0);
    
    down(Wheel(random(150)),0);
  }

}

double micInput(void)
{
  unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;
   unsigned int sample;

   // collect data for 50 mS
   while (millis() - startMillis < 50)
   {
      sample = analogRead(A0);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   double volts = (peakToPeak * 5.0) / 1024;  // convert to volts

   return (volts*100);
}

double smoothVol(double newVol)
{
  double oldVol = vols[0];

  if (oldVol < newVol)
  {
    newVol = (newVol * RISE_RATE) + (oldVol * (1 - RISE_RATE));
    // limit how quickly volume can rise from the last value

    return newVol;
  }

  else
  {
    newVol = (newVol * FALL_RATE) + (oldVol * (1 - FALL_RATE));
    // limit how quickly volume can fall from the last value

    return newVol;
  }
}

double autoMap(const double vol)
{
  double total = 0;
  static double avg;
  static int prevMillis = 0;

  // this if statement acts like a delay, but without putting the entire sketch on hold
  if (millis() - prevMillis > 100)
  {
    for (int i = 100; i > 0; i--)
    {
      total += vols[i - 1];
    }
    vols.addToFront(vol);

    avg = total / 100.0;

    prevMillis = millis();
  }

  if (avg < 5) return 0;
  return map (vol, 0, 3 * avg + 1, 0, strip1.numPixels() / 1.0);
}

uint32_t GetColor(byte pos, double vol) //returns color & brightness
{
  pos = 255 - pos;

  // some last-minute transforms I dont want to stick in the main fcn
  //vol = pow(vol, CONTRAST);
  vol = 100*log(vol);
  vol = map(vol, 0, MAX_VOL, 0, 255);

  if (vol > 255) vol = 255;
  
  if (pos < 85) {
    int red = 255 - pos * 3;
    int grn = 0;
    int blu = pos * 3;
    return strip1.Color(vol * red / 255, vol * grn / 255, vol * blu / 255);
  }
  if (pos < 170) {
    pos -= 85;
    int red = 0;
    int grn = pos * 3;
    int blu = 255 - pos * 3;
    return strip1.Color(vol * red / 255, vol * grn / 255, vol * blu  / 255);
  }
  pos -= 170;
  int red = pos * 3;
  int grn = 255 - pos * 3;
  int blu = 0;
  return strip1.Color(vol * red / 255, vol * grn / 255, vol * blu / 255);
  
}

void displayTower(const double vol)
{
  int i, j;

  for(i=0; i < LEVELS; i++)
  {
    for(j=0; j<38; j++)
    {
      customSetPixel(i,j, color[(j + (2*i))%100]);
    }
  }

  strip0.show();
  strip1.show();
  strip2.show();
  strip3.show();
}

int findLED(int level, int place)
{
  int i, total;

  total = (level/2)*58;

  if(level%2)
    {
      total += (int) place*(20.0/38.0);
    }
  else
    {
      total += 20 + place;
    }

  //account for hardware mistakes in layer strip length
  if(level >= 12 && level <=15)
  {
    total+=-1;
  }
  if(level >= 13 && level <=15)
  {
    total+=-2;
  }

  return total;
}

void customSetPixel(int level, int place, uint32_t color)
{
  int index = findLED(level, place);
  
  if (index < N_STRIP_LEDS)
  {
    strip0.setPixelColor(index, color);
  }
  else if( index < N_STRIP_LEDS * 2)
  {
    strip1.setPixelColor(index - N_STRIP_LEDS, color);
  }
  else if( index < N_STRIP_LEDS * 3)
  {
    strip2.setPixelColor(index - 2* N_STRIP_LEDS, color);
  }
  else if( index < N_STRIP_LEDS * 4)
  {
    strip3.setPixelColor(index - 3*N_STRIP_LEDS, color);
  }
}

void up(uint32_t c, uint8_t wait) {
  for(int i=1; i<32; i+=2) {
    for(int j=0; j < 38; j++)
    {
      customSetPixel(i, j, c);
    }
    strip0.show();
    strip1.show();
    strip2.show();
    strip3.show();
    delay(150);
    
  }
}
void down(uint32_t c, uint8_t wait)
{
  for(int i=32; i>=0; i-=2) {
    for(int j=0; j < 38; j++)
    {
      customSetPixel(i, j, c);
    }
    strip0.show();
    strip1.show();
    strip2.show();
    strip3.show();
    delay(120);
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip0.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip0.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip0.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void crazy(void)
{
  randomSeed(analogRead(A0));
  for(int i = 0; i < 120; i++)
  {
    int level = random(32);
    uint32_t c = Wheel(random(255));
    for(int j = 0; j < 38; j++)
    {
      customSetPixel(level, j, c);
      strip0.show();
    strip1.show();
    strip2.show();
    strip3.show();
    }
    
    delay(20);
  }
}

