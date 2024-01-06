#ifndef _OBP60FUNCTIONS_H
#define _OBP60FUNCTIONS_H

#include <Arduino.h>
#include "OBP60Hardware.h"
  
// Global vars

// Touch keypad over ESP32 touch sensor inputs  

int keypad[9];          // Raw data array from TTP229
int key;                // Value of key [0|1], 0 = touched, 1 = not touched
int keycode = 0;        // Keycode of pressed key [0...8], 0 = nothing touched
int keycode2 = 0;       // Keycode of very short pressed key [0...8], 0 = nothing touched
int keycodeold = 0;     // Old keycode
int keycodeold2 = 0;    // Old keycode for short pressed key
bool keyoff = false;    // Disable all keys
int keydelay = 250;     // Delay after key pressed in  [ms]
bool keylock = false;   // Key lock after pressed key is valid (repeat protection by conginous pressing)
long starttime = 0;     // Start time point for pressed key


int readKeypad() {
  int keystatus = 0;      // Status of key [0...11], 0 = processed, 1...8 = key 1..8, 9 = right swipe , 10 = left swipe, 11 keys disabled
  keycode = 0;

  // Read key code
  if(touchRead(14) > TOUCHTHRESHOLD){ // Touch pad 1
    keypad[1] = 1;
  }
  else{
    keypad[1] = 0;
  }
  if(touchRead(13) > TOUCHTHRESHOLD){ // Touch pad 2
    keypad[2] = 1;
  }
  else{
    keypad[2] = 0;
  }
  if(touchRead(12) > TOUCHTHRESHOLD){ // Touch pad 3
    keypad[3] = 1;
  }
  else{
    keypad[3] = 0;
  }
  if(touchRead(11) > TOUCHTHRESHOLD){ // Touch pad 4
    keypad[4] = 1;
  }
  else{
    keypad[4] = 0;
  }
  if(touchRead(10) > TOUCHTHRESHOLD){ // Touch pad 5
    keypad[5] = 1;
  }
  else{
    keypad[5] = 0;
  }
  if(touchRead(9) > TOUCHTHRESHOLD){  // Touch pad 6
    keypad[6] = 1;
  }
  else{
    keypad[6] = 0;
  }
  // Nothing touched
  if(keypad[1] == 0 && keypad[2] == 0 && keypad[3] == 0 &&  keypad[4] == 0 && keypad[5] == 0 && keypad[6] == 0){
    keypad[0] = 1;
  }
  else{
    keypad[0] = 0;
  } 

  for (int i = 0; i < 9; i++) {
      if(i > 0){
          // Convert keypad to keycode
          if(keypad[i] == 1){
            key = 1;
          }
          else{
            key = 0;
          }
          keycode += key * i;
      }
  }

  // Detect short keynumber
  if (keycode > 0 ){ 
    if(keylock == false){
      starttime = millis();
      keylock = true;
    }
    if (keycode != keycodeold){
      keylock = false;
    }
    // Detect a very short keynumber (10ms)
    if (millis() > starttime + 10 && keycode == keycodeold && keylock == true) {
      // Process only valid keys
      if(keycode == 1 || keycode == 6){
        keycode2 = keycode;
      }
      // Clear by unvalid keys
      else{
        keycode2 = 0;
        keycodeold2 = 0;
      }
    }
    // Timeout for very short pressed key
    if(millis() > starttime + 200){
      keycode2 = 0;
    }
    // Detect a short keynumber (200ms)
    if (keyoff == false && millis() > starttime + 200 && keycode == keycodeold && keylock == true) {
      keystatus = keycode;
      keycode = 0;
      keycodeold = 0;
      keycode2 = 0;
      keycodeold2 = 0;
      buzzer(TONE4, 100);
      keylock = false;
      delay(keydelay);
    }
  }

  // Key lock with key 1 and 6 or 6 and 1 in fast series
  if((keycode2 == 1 && keycodeold2 == 6) || (keycode2 == 6 && keycodeold2 == 1)) {
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    buzzer(TONE4, 1000);
    keylock = false;
    delay(keydelay);
    keyoff = !keyoff;
    keystatus = 11;
  }

  // Detect swipe right
  if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode > keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
  //if (keycode > 0 && keycodeold > 0 && keycode > keycodeold){
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    keystatus = 9;
    buzzer(TONE3, 150);
    buzzer(TONE4, 150);
  }

  // Detect swipe left
  if (keyoff == false && keycode > 0 && keycodeold > 0 && keycode < keycodeold && !((keycode == 1 && keycodeold == 6) || (keycode == 6 && keycodeold == 1))){
  //if (keycode > 0 && keycodeold > 0 && keycode < keycodeold){  
    keycode = 0;
    keycodeold = 0;
    keycode2 = 0;
    keycodeold2 = 0;
    keystatus = 10;
    buzzer(TONE4, 150);
    buzzer(TONE3, 150);
  }

  // Reset keylock after release
  if (keycode == 0){
    keylock = false;
  }

  // Copy keycode
  keycodeold = keycode;
  keycodeold2 = keycode2;

  return keystatus;
}

#endif