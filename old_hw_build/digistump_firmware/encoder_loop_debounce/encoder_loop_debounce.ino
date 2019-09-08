#include "DigiKeyboard.h"

#define encoder0PinA 0
#define encoder0PinB 2

#define aIn  //5 is 0 on the digistump

#define avgPeriod 100

static boolean rotating=false;

float avgRead = 0;
float lastRead = 0;
unsigned char avgCount = 0;

float buttonState =  0;

bool lastButtonState = 0;

int rotIncrement = 0;

void setup() {
 
  pinMode(encoder0PinA, INPUT); 
  digitalWrite(encoder0PinA, HIGH);       
  pinMode(encoder0PinB, INPUT); 
  digitalWrite(encoder0PinB, HIGH); 

  pinMode(1, INPUT);
  digitalWrite(1, HIGH);

  attachInterrupt(0, rotEncoder, CHANGE);  
}

void rotEncoder(){
  rotating=true; 
}

void loop() {
    DigiKeyboard.update();

    avgRead += (float)analogRead(0) / avgPeriod;
    avgCount++;

    buttonState += (float)digitalRead(1) / avgPeriod;

    if(avgCount >= avgPeriod) {
      if(abs(avgRead - lastRead) >= 1) {
        DigiKeyboard.print(String((int)avgRead, HEX));
        DigiKeyboard.println(""); 
        lastRead = avgRead;
      }

      if((buttonState > .5) != lastButtonState) {
        lastButtonState = (buttonState > .5);

        if(lastButtonState) {
          DigiKeyboard.print("n");          
        } else {
          DigiKeyboard.print("m");
        }
         
      }
      
      avgRead = 0;
      avgCount = 0;
      buttonState = 0;
      
    }
  
    while(rotating) {
      delay(2);
  
      if (digitalRead(encoder0PinA) == digitalRead(encoder0PinB)) {  
        rotIncrement++;
      } 
      else {          
        rotIncrement--;                        
      }
      rotating=false; // Reset the flag back to false
    }

    if(rotIncrement >= 2) {
      DigiKeyboard.print("z");
      rotIncrement = 0;
    }

    if(rotIncrement <= -2) {
      DigiKeyboard.print("x");
      rotIncrement = 0;
    }
}
