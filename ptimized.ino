//master.ino
//Robbie Culkin
//for the project found at https://robbieculkin.wordpress.com/2016/08/02/rgb-led-audio-visualizer/

#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel

// PINS
#define STROBE 4
#define RESET 6
#define DC_ONE A1
#define DC_TWO A2
#define LED_PIN 3
#define LED_PIN2 9

//SETTINGS
#define RISE_RATE   0.25      //(0 to 1) higher values mean livelier display
#define FALL_RATE   0.1 
#define CONTRAST   1.3
#define MAX_VOL    600
#define LIVELINESS  2
#define MULTIPLIER 90

//tower info
#define LEVELS 16

//NeoPixel intitialization
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(180, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel tower = Adafruit_NeoPixel(120, LED_PIN2, NEO_GRB + NEO_KHZ800);

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

// ASSIGN COLOR/BRIGHTNESS
    void shiftColors();
    // makes room for a new color value
    uint32_t GetColor(byte pos, double vol);
    // finds the new color value based on color rotation position and volume
    
//DISPLAY
    void displayStrip(const double vol, Adafruit_NeoPixel &strip);
    // sends RGB values to the strip
    void disiplayTower(const double vol);
    // sends RGB values to the tower


//GLOBALS (globals are normally a bad idea, but the nature of Arduino's loop() 
//         function makes them necessary)
double vols[100];
uint32_t color [100];
uint32_t colorCursor = 0;
int numLoops = 0;


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  int frequencies[7];
  double volume;
  static uint16_t pos = 0;
  
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
  // ASSIGN COLOR/BRIGHTNESS
  //
  shiftColors();
  color[colorCursor] = GetColor(pos & 255, volume);

  //rotate the color assignment wheel
  int rotation = map (volume, 0, strip1.numPixels()/2.0, 0, 25);
  numLoops++;
  if(numLoops%5 == 0) //default 5
    pos+=rotation;

  //
  // DISPLAY
  //  
  displayStrip(volume, strip1);
  
}

void recordFrequencies(int (&frequencies) [7])
{
    int band;
    for (band = 0; band<7; band++)
    {
        frequencies[band] = ((analogRead(DC_ONE) + analogRead(DC_TWO))/2);
        
        if (frequencies[band] < 70) frequencies[band] = 0;
        
        //account for noise
        digitalWrite(STROBE, HIGH);
        digitalWrite(STROBE, LOW);
    }
}

double getAvg(const int (&frequencies)[7])
{
  int total = 0;
  
  for(int k=0; k < 7; k++)
  {
    total+= frequencies[k];
  }
  
  return ((double)total/7);
}

double smoothVol(double newVol)
{
  double oldVol = vols[0];
  
  if (oldVol < newVol)
  {
      newVol = (newVol * RISE_RATE) + (oldVol * (1-RISE_RATE));
      // limit how quickly volume can rise from the last value
      
      oldVol = newVol;
      return newVol;
  }
      
  else
  {
      newVol = (newVol * FALL_RATE) + (oldVol * (1-FALL_RATE));
      // limit how quickly volume can fall from the last value
      
      oldVol = newVol;
      return newVol;
  }
      
}

double autoMap(const double vol)
{
  double total = 0;
  static double avg;
  static int prevMillis = 0;

  // this if statement acts like a delay, but without putting the entire sketch on hold
  if(millis() - prevMillis > 100)
  {
    for (int i = 100; i > 0; i--) 
    {
      total += vols[i-1];
      vols[i] = vols[i-1]; 
    }
    vols[0] = vol;
    
    avg = total/100.0;
    
    prevMillis = millis();
  }
  
  if (avg < 5) return 0;
  return map (vol, 0, 3*avg+1, 0, strip1.numPixels()/1.0);
}

void shiftColors()
{
  if (colorCursor == 99)
    colorCursor = 0;
  else
    colorCursor++;
}

uint32_t GetColor(byte pos, double vol) //returns color & brightness
{
  pos = 255 - pos;
  vol = pow(vol, CONTRAST);
  vol = map(vol, 0, MAX_VOL, 0, 255);

if(vol >255) vol = 255;
  
  if(pos < 85) {
    int red = 255 - pos * 3;
    int grn = 0;
    int blu = pos * 3;
    return strip1.Color(vol*red/255, vol*grn/255, vol*blu/255);
  }
  if(pos < 170) {
    pos -= 85;
    int red = 0;
    int grn = pos * 3;
    int blu = 255 - pos * 3;
    return strip1.Color(vol*red/255, vol*grn/255, vol*blu/255);
  }
  pos -= 170;
  int red = pos * 3;
  int grn = 255 - pos * 3;
  int blu = 0;
  return strip1.Color(vol*red/255, vol*grn/255, vol*blu/255);
}

void displayStrip(const double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip1.Color(0, 0, 0);

  // some last-minute transforms
  double newVol = 0.5*vol;
  newVol = pow(newVol, LIVELINESS);
  
  int i;
  for (i = 0; i < strip.numPixels(); i++)
  {
    // some magic numbers here that made the output better. should eventually get rid of them
    double threshold = ((double)(i+1) * MULTIPLIER - (3*newVol)+5);
     
    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels()/2-1-i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels()/2-1-i, off);
    }
    
    if (newVol > threshold)
    {
      strip.setPixelColor(strip.numPixels()/2-1+i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels()/2-1+i, off);
    }
  }
  strip.show();
}

