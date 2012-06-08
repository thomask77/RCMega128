/*  $Id: main.c 168 2005-11-29 02:27:57Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/

// include files -----
//
#include "servo.h"
#include <inttypes.h>
#include <stdlib.h>
#include <avr/io.h>

// Microseconds to CPU ticks
//
#define  US_TICKS(t)  ((uint16_t)(t * (F_CPU/1e6)))

// Maximum PWM on-time for servo position
//
#define  MAX_SERVO_TIME  US_TICKS(2500)


typedef struct {
  uint16_t  tick;     ///< Time of event
  uint8_t   a, b, c;  ///< I/O Port status
} ServoEvent;

static ServoEvent servoEvents[24];



/**
 * qsort compare function for uint16
 *
 * \param va  pointer to first integer
 * \param vb  pointer to second integer
 * \return  1 if a>b, -1 if a<b, 0 if equal
 */
static int compare_uint16(const void *va, const void *vb)
{
  uint16_t a = *(uint16_t*)va;
  uint16_t b = *(uint16_t*)vb;
  if (a > b)  return  1;
  if (a < b)  return -1;
  return 0;
}


/**
 * Set servo target positions.
 * 
 * \param  positions  array of 24 servo target positions.
 * \note   target positions are given as PWM pulse widths in CPU ticks.
 */
void SRV_SetPositions(unsigned *positions)
{
  // Initialize and sort event table
  //
  for (uint8_t i=0; i<24; i++) {
    servoEvents[i].tick = positions[i];
    if  (i<8)  { 
      servoEvents[i].a = 1<<i;
      servoEvents[i].b = 0;
      servoEvents[i].c = 0;
    }
    else if (i<16) {
      servoEvents[i].a = 0;
      servoEvents[i].b = 1<<(i-8);
      servoEvents[i].c = 0;
    }
    else {
      servoEvents[i].a = 0;
      servoEvents[i].b = 0;
      servoEvents[i].c = 1<<(i-16);
    }
  }
  
  qsort(servoEvents, 24, sizeof(ServoEvent), compare_uint16);

  // Merge port bits (they should sum up to 0x0fff)..
  //
  uint8_t k=0, a=0, b=0, c=0;
  for (uint8_t i=0; i<24;) {
    uint16_t  tick = servoEvents[i].tick;
 
    // Merge entries that are less than one loop iteration apart.
    // The loop will always execute for at least one iteration.
    //
    for (uint8_t j=i; j<24 && servoEvents[j].tick - tick < 30; j++) {
      a |= servoEvents[j].a;
      b |= servoEvents[j].b;
      c |= servoEvents[j].c;
      i++;
    }
    servoEvents[k].tick = tick;
    servoEvents[k].a    = a;
    servoEvents[k].b    = b;
    servoEvents[k].c    = c;
    k++;
  }

  // Reset timer and begin servo output.
  //
  TCCR1B = 0;
  TCNT1  = 0; 
  TCCR1B = _BV(CS10);
  TIFR  |= _BV(OCF1A);

  DDRA = 0x00;  DDRB = 0x00;  DDRC = 0x00;
  ServoEvent *e = servoEvents;
  for (uint8_t i=0; i<k; i++) {
    while (TCNT1 < e->tick);
    DDRA = e->a;
    DDRB = e->b;
    DDRC = e->c;
    e++;
  }
  loop_until_bit_is_set(TIFR, OCF1A);
}


/**
 * Read back current servo positions.
 *
 * \param positions  array of 24 current servo positions.
 * \note   
 *   This function will only work with special Kondo-ICS
 *   compatible servos. Target positions are given as PWM
 *   pulse widths in CPU ticks.
 */
void SRV_GetPositions(unsigned *positions)
{
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1B = _BV(CS10);

  // Send 100us pulse
  //
  OCR1A  = US_TICKS(100);
  TIFR  |= _BV(OCF1A);

  DDRA = 0x00;  DDRB = 0x00;  DDRC = 0x00;
  loop_until_bit_is_set(TIFR, OCF1A);
  DDRA = 0xff;  DDRB = 0xff;  DDRC = 0xff;

  // Servo output starts at 150us
  //
  OCR1A  = US_TICKS(150);
  TIFR  |= _BV(OCF1A);
  loop_until_bit_is_set(TIFR, OCF1A);
  DDRA = 0x00;  DDRB = 0x00;  DDRC = 0x00;

  // Skip low phase of output
  //
  OCR1A  = US_TICKS(300);
  TIFR  |= _BV(OCF1A);
  loop_until_bit_is_set(TIFR, OCF1A);
  
  // Read positions
  //
  OCR1A  = MAX_SERVO_TIME + US_TICKS(300);
  TIFR  |= _BV(OCF1A);

  ServoEvent *e  = servoEvents;
  uint8_t  pina = 0xff, pinb = 0xff, pinc = 0xff;
  uint8_t  a    = 0xff, b    = 0xff, c    = 0xff;

  while (!(TIFR & _BV(OCF1A))) {
    pina &= PINA;  pinb &= PINB;  pinc &= PINC;
    if ((pina ^ a) | (pinb ^ b) | (pinc ^ c)) {
      e->tick  = TCNT1;
      e->a = a = pina;
      e->b = b = pinb;
      e->c = c = pinc;
      e++;
    }
  }
  DDRA = 0xFF;  DDRB = 0xFF;  DDRC = 0xFF;
  e->tick = 0; e->a = 0; e->b = 0; e->c = 0;

  // Reconstruct servo positions
  //
  while (e >= servoEvents) {
    uint16_t tick = e->tick - US_TICKS(250);
    a = ~e->a;
    b = ~e->b;
    c = ~e->c;
    e--;
    if (a & 0x01)  positions[ 0] = tick;
    if (a & 0x02)  positions[ 1] = tick;
    if (a & 0x04)  positions[ 2] = tick;
    if (a & 0x08)  positions[ 3] = tick;
    if (a & 0x10)  positions[ 4] = tick;
    if (a & 0x20)  positions[ 5] = tick;
    if (a & 0x40)  positions[ 6] = tick;
    if (a & 0x80)  positions[ 7] = tick;

    if (b & 0x01)  positions[ 8] = tick;
    if (b & 0x02)  positions[ 9] = tick;
    if (b & 0x04)  positions[10] = tick;
    if (b & 0x08)  positions[11] = tick;
    if (b & 0x10)  positions[12] = tick;
    if (b & 0x20)  positions[13] = tick;
    if (b & 0x40)  positions[14] = tick;
    if (b & 0x80)  positions[15] = tick;

    if (c & 0x01)  positions[16] = tick;
    if (c & 0x02)  positions[17] = tick;
    if (c & 0x04)  positions[18] = tick;
    if (c & 0x08)  positions[19] = tick;
    if (c & 0x10)  positions[20] = tick;
    if (c & 0x20)  positions[21] = tick;
    if (c & 0x40)  positions[22] = tick;
    if (c & 0x80)  positions[23] = tick;
  }
}


/**
 * Initialize servo I/O ports. 
 * Assumes Servos on port A, B and C.
 *
 */
void SRV_Init()
{
  PORTA = 0x00;  DDRA  = 0xff;
  PORTB = 0x00;  DDRB  = 0xff;
  PORTC = 0x00;  DDRC  = 0xff;
}
