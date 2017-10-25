#include <avr/sleep.h>
//We use part of the WDT library, but have to use registers as well since library does not support interrupt mode for WDT
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "../utilities/uart.h"
#include "lowpower.h"

volatile bool iswdtsleeping = false;
volatile bool wdtflag = false;
uint16_t WDTsleep(const uint16_t seconds) {
  //Serial.print("Seconds: ");
  //Serial.println(seconds);
  serial0_println("Entering WDTsleep");
  serial0_flush();
  uint16_t left = seconds;
  iswdtsleeping = true;
  
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA &= ~(1<<ADEN); // Disable ADC
  
  while (left) {
    WDTCSR |= 0b00011000;    //Set the WDE bit and then clear it when set the prescaler, WDCE bit must be set if changing WDE bit   
    WDTCSR =  0b01000000 | WDTO_1S; //Or timer prescaler byte value with interrupt selectrion bit set

    wdt_reset();
    wdtflag = false;
    
    sleep_cpu();
    
    if (!wdtflag) {
      break;
    }

    left--;
  }

  ADCSRA |= (1<<ADEN); // Enable ADC
  sleep_disable();
  iswdtsleeping = false;
  serial0_println("WDTsleep done.");
  serial0_flush();
  return left;
}

//This is the interrupt service routine for the WDT. It is called when the WDT times out. 
//This ISR must be in your Arduino sketch or else the WDT will not work correctly
ISR(WDT_vect) 
{
  wdtflag = true;
  wdt_disable();
  MCUSR = 0; //Clear WDT flag since it is disabled, this is optional
}  // end of WDT_vect