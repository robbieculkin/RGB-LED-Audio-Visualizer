//
//  visualizer.h
//  
//
//  Created by Robert Culkin on 1/11/17.
//
//


#ifndef visualizer_h
#define visualizer_h
#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

class neoPixelObj
{
    Adafruit_NeoPixel strip;
    double mults [4];
    bool switches [4];
    
public:
    neoPixelObj(Adafruit_NeoPixel myStrip);
    void bluetoothInput(SoftwareSerial &BT_Serial);
};



template <typename Item>
class circularArray
{
    int arrayLength;
    int first;
    Item* elts;
    
public:
    circularArray(int length);
    void addToFront(Item toAdd);
    Item operator[] (const int index);
};

template <typename Item>
circularArray<Item>::circularArray(int length)
{
    arrayLength = length;
    elts = new Item [length];
    first = length;
}

template <typename Item>
void circularArray<Item>::addToFront(Item toAdd)
{
    if(first > 0)
        first -= 1;
    else
        first = arrayLength-1;
    
    elts [first] = toAdd;
    
}
template <typename Item>
Item circularArray<Item>::operator[] (const int index)
{
    return elts[(first+index) % arrayLength];
}

#endif /* deque_h */

