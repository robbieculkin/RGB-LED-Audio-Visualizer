//
//  visualizer.cpp
//  
//
//  Created by Robert Culkin on 1/11/17.
//
//

#include "visualizer.h"

neoPixelObj::neoPixelObj(Adafruit_NeoPixel myStrip)
{
    strip = myStrip;
    
    for(int i = 0; i < 4; i++)
    {
        mults[i] = 1.0;
        switches[i] = true;
    }
}

void neoPixelObj::bluetoothInput(SoftwareSerial &BT_Serial)
{
    char label = BT_Serial.read();
    char in1;
    char in2;
    double value = 0;
    
    if (isalpha(label))
    {
        in1 = BT_Serial.read();
        in2 = BT_Serial.read();
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
        switches[0] = (value == 1);
    }
    if(label == 'g')
    {
        switches[1] = (value == 1);
    }
    if(label == 'b')
    {
        switches[2] = (value == 1);
    }
    
    // Color scaling
    if(label == 'R' || label == 'B' || label == 'G' || label == 'C')
    {
        value = value/100.0;
        
        switch (label)
        {
            case 'R' : mults[0] = value;
                break;
            case 'G' : mults[1] = value;
                break;
            case 'B' : mults[2] = value;
                break;
            case 'C' : mults[3] = value;
                break;
        }
    }
    
    BT_Serial.write(label);
    BT_Serial.write(in1);
    BT_Serial.write(in2);
}

