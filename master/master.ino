//master.ino
//Robbie Culkin & Daniel Barkhorn
//for the project found at https://robbieculkin.wordpress.com/2016/08/02/rgb-led-audio-visualizer/

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel
#include "visualizer.h"

// PINS & INFO
#define BT_TX           A9
#define BT_RX           A8
#define STROBE           4
#define RESET            6
#define DC_ONE          A1
#define DC_TWO          A2
#define STRIP_PIN        3
#define N_STRIP_LEDS   180
#define TOWER_PIN        9
#define N_TOWER_LEDS   120
#define LEVELS          16

//SETTINGS
#define BRIGHTNESS  150       //(0 to 255)
#define RISE_RATE     0.13    //(0 to 1) higher values mean livelier display
#define FALL_RATE     0.04    //(0 to 1) higher values mean livelier display
#define CONTRAST      1.3     //(undefined range)
#define MAX_VOL     600       //(0 to 1023) maximum value reading expected from the shield
#define LIVELINESS    2       //(undefined range)
#define LIVELINESS_T  1.7     //(undefined range)
#define MULTIPLIER   90       //(undefined range) higher values mean fewer LEDs lit @ a given volume

//NeoPixel & Bluetooth intitialization
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(N_STRIP_LEDS, STRIP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tower  = Adafruit_NeoPixel(N_TOWER_LEDS, TOWER_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial BT_Board(BT_TX, BT_RX);    //TX, RX

// READ
void recordFrequencies(int (&frequencies) [7]);
// records frequencies into the frequencies array
double getAvg(const int (&frequencies) [7]);
// returns avg of frequencies

//FILTER
double smoothVol(double newVol);
// brings vol values closer together, resulting a smoother output (avoid strobing)
double autoMap(const double vol);
// scales volume values in order to get consisitent output at different overall volumes

//DISPLAY
void shiftColors();
// makes room for a new color value
uint32_t GetColor(byte pos, double vol);
// finds the new color value based on color rotation position and volume
void displayStrip(const double vol, Adafruit_NeoPixel &strip);
// detemines what LEDs are on/off; sends RGB values to the strip
void displayTower(const double vol, Adafruit_NeoPixel &strip);
// detemines what LEDs are on/off; sends RGB values to the strip

//HELPER FUNCTIONS
int findLED(int level, int place);
//returns the index of any LED on the tower given its level and position on the level
void bluetoothInput();

//GLOBALS (globals are normally a bad idea, but the nature of Arduino's loop()
//         function makes them necessary)
circularArray<double> vols = circularArray<double>(100);          // last 100 volumes recorded
circularArray<uint32_t> color = circularArray<uint32_t>(100);       // last 100 color values generated
int numLoops = 0;           // number of loops executed so far
double redMult = 1.0;       // changed by BT
double grnMult = 1.0;       // changed by BT
double bluMult = 1.0;       // changed by BT
double brtMult = 1.0;
bool grnOn = true;
bool bluOn = true;
bool redOn = true;
double eqMults [7];

void setup() {

  //Set default values for arrays
  for (int i = 0; i < 100; i++)
  {
    vols.addToFront(100);   //if set to 0, display would flash on. this creates a slow opening
    color.addToFront(strip1.Color(0, 0, 0));
  }

  for (int i = 0; i<7; i++)
  {
    eqMults[i] = 1.0;
  }

  Serial.begin(9600);
  BT_Board.begin(9600);

  //Spectrum Shield pin configurations
  pinMode(STROBE, OUTPUT);
  pinMode(RESET, OUTPUT);
  pinMode(DC_ONE, INPUT);
  pinMode(DC_TWO, INPUT);
  digitalWrite(STROBE, HIGH);
  digitalWrite(RESET, HIGH);

  //Initialize Spectrum Analyzers
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, HIGH);
  delay(1);
  digitalWrite(STROBE, HIGH);
  delay(1);
  digitalWrite(STROBE, LOW);
  delay(1);
  digitalWrite(RESET, LOW);

  strip1.begin();
  strip1.setBrightness(BRIGHTNESS);
  strip1.show();
  tower.begin();
  tower.setBrightness(BRIGHTNESS);
  tower.show();
}

void loop() {
  int frequencies[7];
  double volume;
  static uint16_t pos = 0;


  if (BT_Board.available())
    bluetoothInput();
    
  //
  // READ
  //
  recordFrequencies(frequencies);
  volume = getAvg(frequencies);

  //
  // FILTER
  //
  volume = smoothVol(volume);
  volume = autoMap(volume);

  //
  // DISPLAY
  //
  color.addToFront(GetColor(pos & 255, volume));

  //rotate the color assignment wheel
  int rotation = map (volume, 0, strip1.numPixels() / 2.0, 0, 25);
  numLoops++;
  if (numLoops % 5 == 0) //default 5
    pos += rotation;

  displayStrip(volume, strip1);
  displayTower(volume, tower);

}

void recordFrequencies(int (&frequencies) [7])
{
  int band;
  for (band = 0; band < 7; band++)
  {
    frequencies[band] = ((analogRead(DC_ONE) + analogRead(DC_TWO)) / 2);

    if (frequencies[band] < 70) frequencies[band] = 0;

    //account for noise
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
  }
}

double getAvg(const int (&frequencies)[7])
{
  int total = 0;

  for (int k = 0; k < 7; k++)
  {
    total += eqMults[k] * frequencies[k];
  }

  return ((double)total / 7);
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
  double myRedMult = redMult * brtMult;
  double myGrnMult = grnMult * brtMult;
  double myBluMult = bluMult * brtMult;
  
  pos = 255 - pos;

  // some last-minute transforms I dont want to stick in the main fcn
  vol = pow(vol, CONTRAST);
  vol = map(vol, 0, MAX_VOL, 0, 255);

  if (vol > 255) vol = 255;

  // All 3 colors on
  if (redOn && grnOn && bluOn)
  {
    if (pos < 85) {
      int red = 255 - pos * 3;
      int grn = 0;
      int blu = pos * 3;
      return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
    }
    if (pos < 170) {
      pos -= 85;
      int red = 0;
      int grn = pos * 3;
      int blu = 255 - pos * 3;
      return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
    }
    pos -= 170;
    int red = pos * 3;
    int grn = 255 - pos * 3;
    int blu = 0;
    return strip1.Color(vol * red * myRedMult / 255, vol * grn * myGrnMult / 255, vol * blu * myBluMult / 255);
  }

  // Two colors on
  else if ((redOn && grnOn) || (grnOn && bluOn) || (redOn && bluOn))
  {
    int color1 = 0;
    int color2 = 0;
    if (pos < 127) {
      color1 = 255 - pos * 2;
      color2 = pos * 2;
    }
    else {
      pos -= 127;
      color1 = pos * 2;
      color2 = 255 - pos * 2;
    }

    if (redOn && grnOn)
    {
      return strip1.Color(vol * color1 * myRedMult / 255, vol * color2 * myGrnMult / 255, 0);
    }
    else if (grnOn && bluOn)
    {
      return strip1.Color(0, vol * color1 * myGrnMult / 255, vol * color2 * myBluMult / 255);
    }
    else if (redOn && bluOn)
    {
      return strip1.Color(vol * color1 * myRedMult / 255, 0, vol * color2 * myBluMult / 255);
    }
  }

  // Only one color on
  else if (redOn)
  {
    return strip1.Color(vol * 255 * myRedMult / 255, 0, 0);
  }
  else if (grnOn)
  {
    return strip1.Color(0, vol * 255 * myGrnMult / 255, 0);
  }
  else if (bluOn)
  {
    return strip1.Color(0, 0, vol * 255 * myBluMult / 255);
  }

  // All colors off
  else
    return strip1.Color(0, 0, 0);
}

void displayStrip(const double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip1.Color(0, 0, 0);

  // some last-minute transforms I dont want to stick in the main fcn
  double newVol = 0.5 * vol;
  newVol = pow(newVol, LIVELINESS);

  int i;
  for (i = 0; i < strip.numPixels() / 2; i++)
  {
    // some magic numbers here that made the output better. should eventually get rid of them
    int threshold = (int)((i + 1) * MULTIPLIER - (3 * newVol) + 5);

    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 - i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 - i, off);
    }

    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 + i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels() / 2 - 1 + i, off);
    }
  }
  strip.show();
}

void displayTower(const double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip.Color(0, 0, 0);

  double myVol = 0.4 * vol;
  myVol = pow(myVol, LIVELINESS_T);     
  //uses power function to scale volumes,
  //making large volumes have greater
  //visual effect

  int i, j;

  double threshold;
  for (int i = 0;  i < LEVELS; i++)
  {
    threshold = (((double)(i + 1) * MULTIPLIER) - 3 * myVol + 5) * LEVELS;
    // These magic numbers work
    if (i % 2 == 1)                     // On a big 10 LED ring of tower
    {
      if (myVol > threshold)
      {
        for (j = 0; j < 10; j++)
        {
          strip.setPixelColor(findLED(i, j), color[10 * j + i]);
        }
      }
      else                              
      {
        for (j = 0; j < 10; j++)
        {
          strip.setPixelColor(findLED(i, j), off);
        }
      }
    }
    else      //(i&2 == 0) on a smaller ring with 5 LEDs
    {
      if (myVol > threshold)
      {
        for (j = 0; j < 5; j++)
        {
          strip.setPixelColor(findLED(i, j), color[(10 * 2 * j) + i]);
        }
      }
      else
      {
        for (j = 0; j < 5; j++)
        {
          strip.setPixelColor(findLED(i, j), off);
        }
      }
    }
  }
  strip.show();
}

void displayTower2(const double vol, Adafruit_NeoPixel &strip)
{
  for(int i = 0; i<LEVELS; i++)
  {
    for(int j = 0; j < 10; i++)
    {
      if (i%2 == 0) //smaller rings
      {
        strip.setPixelColor(findLED(i, j), color[(10* 2 * j) + i]);
      }
      else
      {
        strip.setPixelColor(findLED(i, j), color[10 * j + i]);
      }
    }
  }
  strip.show();
}

int findLED(int level, int place)
{
  int i, total = 0;
  for (i = 0; i < level; i++)
  {
    if (i % 2 == 0)
    {
      total += 5;
    }
    else
    {
      total += 10;
    }
  }
  if (level % 2 == 0 && place > 4)
  {
    place = 4;
  }
  total += place;

  return total;
}

// Function to interpret Bluetooth input
// Preconditions: Bluetooth signal has been successfully recieved as string
// Postconditions: Appropriate variables have been changed (?)
void bluetoothInput()
{
  char label = BT_Board.read();
  char in0;
  char in1;
  char in2;
  double value = 0;
  uint8_t index;

  if (isalpha(label))
  {
    in0 = BT_Board.read();
    in1 = BT_Board.read();
    in2 = BT_Board.read();
    index = in0 - '0';
    uint8_t tens = in1 - '0';
    uint8_t ones = in2 - '0';
    
    if (ones > 9)
    {
      ones = tens;
      tens = 0;
    }

    value += 10 * tens;
    value += ones;

    if(value > 99)
      return;
  }

  // Color on/off switches
  if(label == 'r')
  {
    redOn = (value == 1);
  }
  if(label == 'g')
  {
    grnOn = (value == 1);
  }
  if(label == 'b')
  {
    bluOn = (value == 1);
  }
  if(label == 'c')
  {
    redOn = (value == 1);
    grnOn = (value == 1);
    bluOn = (value == 1);
  }
 
  // Color scaling
  if(label == 'R' || label == 'B' || label == 'G' || label == 'C')
  {
    value = value/100.0;

    switch (label)
    {
      case 'R' : redMult = value;
                 break;
      case 'G' : grnMult = value;
                 break;
      case 'B' : bluMult = value;
                 break;
      case 'C' : brtMult = value;
                 break;
    }
  }

  if(label == 'E')
  {
    eqMults[index] = value/50.0;
  }
  
  Serial.write(label);
  Serial.write(in0);
  Serial.write(in1);
  Serial.write(in2);
}



