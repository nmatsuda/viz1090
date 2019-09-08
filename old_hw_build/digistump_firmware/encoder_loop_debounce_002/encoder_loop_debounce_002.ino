#include "DigiKeyboard.h"

#define encoderPinA 0
#define encoderPinB 2
#define selectPin   1
#define sqlPin      0   //5 is 0 on the digistump

#define bufferPeriod 100

float encoderPinABuffer = 0;
float encoderPinBBuffer = 0;
float selectPinBuffer = 0;
float sqlPinBuffer = 0;

int encoderCounter = 0;

bool encoderPinALast = 0;
bool encoderPinBLast = 0;
bool selectPinLast = 0;
float sqlPinLast = 0;

int bufferCounter = 0;

int8_t lookup_table[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

uint8_t index = 0;

void setup() {
 
  pinMode(encoderPinA, INPUT); 
  digitalWrite(encoderPinA, HIGH);       
  pinMode(encoderPinB, INPUT); 
  digitalWrite(encoderPinB, HIGH); 

  pinMode(selectPin, INPUT);
  digitalWrite(selectPin, HIGH);

  pinMode(sqlPin, INPUT);
}

void loop() {

  //update buffers
  
  //encoderPinABuffer += (float)digitalRead(encoderPinA) / bufferPeriod;
  //encoderPinBBuffer += (float)digitalRead(encoderPinB) / bufferPeriod;
  selectPinBuffer += (float)digitalRead(selectPin) / bufferPeriod;
  sqlPinBuffer += (float)analogRead(sqlPin) / bufferPeriod;

  bufferCounter++;

  bool encoderPinAState = (bool)digitalRead(encoderPinA);
  bool encoderPinBState = (bool)digitalRead(encoderPinB);

  if((encoderPinAState != encoderPinALast) | (encoderPinBState != encoderPinBLast)) {
    index = (index << 2) & 0b1100;
    index = index  | ((encoderPinAState << 1) + encoderPinBState);
  
    encoderCounter += lookup_table[index & 0b1111];
    
    if(encoderCounter > 2) {
      DigiKeyboard.print("z");
      encoderCounter = 0;
    }

    if(encoderCounter < -2) {
      DigiKeyboard.print("x");
      encoderCounter = 0;
    }   

    encoderPinALast = encoderPinAState;
    encoderPinBLast = encoderPinBState;
  }
  
  //check pin values
  if(bufferCounter > bufferPeriod) {
    //usb keepalive
    DigiKeyboard.update();

    //bool encoderPinAState = encoderPinABuffer > .5;
    //bool encoderPinBState = encoderPinBBuffer > .5;
    bool selectPinState = selectPinBuffer > .5;

    //check encoder pins nd transmit
//    if(encoderPinAState != encoderPinALast || encoderPinBState != encoderPinBLast)  {
//
//      uint8_t index = (encoderPinALast << 4) + (encoderPinBLast << 3) + (encoderPinAState << 1) + encoderPinBState;
//
//      //encoderDetentCounter += lookup_table[index];
//
//      if(lookup_table[index] > 0) {
//        DigiKeyboard.print("z");
//      } else if (lookup_table[index] < 0) {
//        DigiKeyboard.print("x");
//      }
//      
//      encoderPinALast = encoderPinAState;
//      encoderPinBLast = encoderPinBState;
//    }
    
    //check select pin and transmit
    if(selectPinState != selectPinLast) {
      selectPinLast = selectPinState;
      if(selectPinLast) {
        DigiKeyboard.print("n");          
      } else {
        DigiKeyboard.print("m");
      }
    }

    //check squelch knob and transmit    
    if(abs(sqlPinBuffer - sqlPinLast) >= 1) {
      DigiKeyboard.print(String((int)sqlPinBuffer, HEX));
      DigiKeyboard.println(""); 
      sqlPinLast = sqlPinBuffer;
    }

    // reset buffers
    encoderPinABuffer = 0;
    encoderPinBBuffer = 0;
    selectPinBuffer = 0;
    sqlPinBuffer = 0;
    bufferCounter = 0;
  }
}
