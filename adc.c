/*  $Id: adc.c 171 2005-11-30 14:43:53Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/

// include files -----
//
#include "adc.h"
#include <stdbool.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/signal.h>

static volatile bool adc_ready;

// interrupt handlers -----
//
SIGNAL(SIG_ADC)
{
  adc_ready = true;  
}


void ADC_Init()
{
  // Port F is used for analog inputs
  //
  DDRF  = 0;
  PORTF = 0x00;

  // Enable ADC, ~8kSamples/s, interrupt enabled
  // 
  ADCSRA = _BV(ADEN)  | _BV(ADIE)  | 
           _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}


unsigned ADC_Read()
{
  // Don't use SLEEP_MODE_ADC, because it would stop the UARTs.
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  adc_ready = false;
  sleep_mode();
  while (!adc_ready);
  return ADC; 
}

