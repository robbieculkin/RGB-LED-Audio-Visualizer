//master.ino
//Robbie Culkin
//for the project found at https://robbieculkin.wordpress.com/2016/08/02/rgb-led-audio-visualizer/

#include <Adafruit_NeoPixel.h> //https://github.com/adafruit/Adafruit_NeoPixel

//Pins
#define LED_PIN 3
#define LED_PIN2 9
#define STROBE 4
#define RESET 6
#define DC_ONE A1
#define DC_TWO A2
#define POT A0

//globals
double oldVol = 0;
int numLoops = 0;
uint16_t j=0;
uint32_t color [100];
int frequencies[7];
int oldFreqs[7];
double vols[100];

//settings
#define TIME       8
#define MULTIPLIER 90
#define DIVISOR 1
#define MAX_VOL    600
#define UP         0.25      //(0 to 1) higher values mean livelier display
#define DOWN       0.1   
#define DIMMER     1.75
#define CONTRAST   1.3
#define LIVELINESS  2
#define LIVELINESS2 1.7

//tower info
#define LEVELS 16
//NeoPixel intitializations
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(180, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(120, LED_PIN2, NEO_GRB + NEO_KHZ800);
//uint32_t color = strip1.Color(0, 0, 0);


void setup() 
{
  Serial.begin(9600);
  
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
  strip1.setBrightness(150);
  strip1.show();
  strip2.begin();
  strip2.setBrightness(150);
  strip2.show();
}

void loop() 
{
  
  Read_Frequencies();
  double vol = GetAvgVolume(); 
  vol = SmoothVols(vol); // smooths volumes before they go to the array
  double newVol = AutoMap(vol);
  double TowerVol = AutoMap(vol);
  ShiftArrayRight();
  color[0] = GetColor(j & 255, vol/DIMMER);
  Display_FTM(newVol, strip1);
  SmoothFreqs();
  VerticalColorShift(TowerVol, strip2);
  //Display_FTM(newVol, strip2);
  delay(TIME); // slows pulses based on volume
  //Serial.println(newVol);
  int roc = map (newVol, 0, strip1.numPixels()/2.0, 0, 25);
  
  oldVol = vol;
  numLoops++;
  if(numLoops%5 == 0) //default 1
  {
    j+=roc; // increase to rotate colors faster. default 2
  }
}

void Read_Frequencies() //Read frequencies for each band
{ 
  int band;
  for (band = 0; band<7; band++)
  {                                         // + analogRead(DC_TWO)
    frequencies[band] = ((analogRead(DC_ONE)));
    // ^ take avg of the two sensors and divide by 2 using a left shift 
    if (frequencies[band] < 70) frequencies[band] = 0;
    digitalWrite(STROBE, HIGH);
    digitalWrite(STROBE, LOW);
//    Serial.print(band);
//    Serial.print(": ");
//    Serial.println(frequencies[band]);
  }
}
void Display_All(double vol, Adafruit_NeoPixel &strip)
{
  vol = pow(vol, 1.75);
  
  int i;
  for (i = 0; i < strip.numPixels()/2; i++)
  {
    double threshold = ((double)(i+1)*MULTIPLIER)/DIVISOR-3*vol+5;
    
    strip.setPixelColor(strip.numPixels()/2-1-i, color[i]);

    strip.setPixelColor(strip.numPixels()/2+i, color[i]);
  } 
  strip.show();
}

void Display_FTM (double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip1.Color(0, 0, 0);

  vol = 0.5*vol;
  vol = pow(vol, LIVELINESS);
  
  int i;
  for (i = 0; i < strip.numPixels(); i++)
  {
    double threshold = ((double)(i+1)*MULTIPLIER)/DIVISOR-3*vol+5;
    if (vol > threshold)
    {
      strip.setPixelColor(strip.numPixels()/2-1-i, color[i]);
    }
    else
    {
      strip.setPixelColor(strip.numPixels()/2-1-i, off);
    }
    
    if (vol > threshold)
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

double GetAvgVolume() //takes the average of NUM_AVG readings from the sensor
{
  int total = 0;
  int k;
  for(k=0; k < 7; k++)
  {
    total+= frequencies[k];
  }
  return ((double)total/7);
}

double SmoothVols(double newVol)
{
  if (oldVol < newVol)
      return (newVol * (UP)) + (oldVol * (1-(UP)));
  else
      return (newVol * (DOWN)) + (oldVol * (1-(DOWN)));
}

double ManualMap(double vol)
{
  int potVal = analogRead(POT);

  return vol*potVal/1024;
}

double AutoMap(double vol)
{
  double total = 0;
  static double avg;
  static int prevMillis = 0;
  
  if(millis() - prevMillis > 100)
  {
    for (int i = 100; i > 0; i--) 
    {
      total += vols[i-1];
      vols[i] = vols[i-1]; 
    }
    vols[0] = vol;
    avg = total/100.0;
    //if (avg < 10) avg = 10;
    
    prevMillis = millis();
  }
  
  if (avg < 5) return 0;
  return map (vol, 0, 3*avg+1, 0, strip1.numPixels()/1.0);
}

//TODO eliminate this function using a position variable
void ShiftArrayRight()
{
  for (int i = 99; i >=1; i--) 
  {
     color[i] = color[i-1];
  }
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
void SmoothFreqs()
{
  int i;
  for (i=0; i<7; i++)
  {
    if (oldFreqs[i] < frequencies[i])
        frequencies[i] = (frequencies[i] * (0.1)) + (oldFreqs[i] * (1-(0.1)));
    else
        frequencies[i] = (frequencies[i] * (0.1)) + (oldFreqs[i] * (1-(0.1)));
  }
}

int findLED(int level, int index)
{
  int i, total = 0;
  for(i=0; i < level; i++)
  {
    if(i%2 == 0)
    {
      total += 5;
    }
    else
    {
      total += 10;
    }
  }
  if (level%2 == 0 && index > 4)
  {
    index = 4;
  }
  total += index;
  //Serial.println(total);
  return total;
}


void VerticalColorShift(double vol, Adafruit_NeoPixel &strip)
{
  uint32_t off = strip1.Color(0, 0, 0);

  vol = 0.4*vol;
  vol = pow(vol, LIVELINESS2);
  
  int i, j;
  for (i=0; i < LEVELS; i+=2)
  {
    double threshold = (((double)(i+1)*MULTIPLIER)/DIVISOR-3*vol+5)*14;
    if(vol > threshold)
    {
      for (j=0; j < 5; j++)
      {
        strip.setPixelColor(findLED(i, j), color[2*j+i]);
      }
    }
    else
    {
      for (j=0; j < 5; j++)
      {
        strip.setPixelColor(findLED(i, j), off);
      }
    }
  }
  for (i=1; i < LEVELS; i+=2)
  {
    double threshold = (((double)(i+1)*MULTIPLIER)/DIVISOR-3*vol+5)*14;
    if(vol > threshold)
    {
      for (j=0; j < 10; j++)
      {
        strip.setPixelColor(findLED(i, j), color[j+i]);
      }
    }
    else
    {
      for (j=0; j < 10; j++)
      {
        strip.setPixelColor(findLED(i, j), off);
      }
    }
  }
  strip.show();
}
