/*  $Id: beeper.c 246 2006-01-08 21:28:50Z kindler $
    loosely based on Craig Peacock's PIC16F87x sourcecode
    http://www.beyondlogic.org/pic/ringtones.htm

    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/

// include files -----
//
#include "beeper.h"
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <avr/delay.h>
#include <avr/wdt.h>

/**
 * Note frequency table, based on the 6th MIDI Octave
 * \see  http://www.google.com/search?q=note+frequency
 */
static const unsigned freqtable[] = { 
  // A    A#   B     -  C    C#   D    D#   E     -   F    F#   G    G#
     440, 466, 494,  0, 262, 277, 294, 311, 330,   0, 349, 370, 392, 415
};


typedef struct {
  int   defaultOctave;
  int   defaultDuration;
  int   bpm;

  int   note;
  int   octave;
  int   duration;
  bool  dotted;
  int  *number;
  
  enum {
    TITLE, PARAMS, SONG
  } state;

} RTTTL_State;



/**
 * Beep.
 *
 * \note   This function is not accurate in any way ;)
 * \param  freq    Frequency in Hz (use 0 to generate a silent pause)
 * \param  duration  Length in ms
 *
 * \todo   Calibrate this with a scope or AVR Simulator
 */
void Beep(unsigned freq, unsigned duration)
{
  if (freq) {
    BEEPER_DDR |= _BV(BEEPER_BIT1) | _BV(BEEPER_BIT2);
  } else {
    freq = 1000;
    BEEPER_DDR &= ~(_BV(BEEPER_BIT1) | _BV(BEEPER_BIT2));
  }
  
  BEEPER_PORT |=  _BV(BEEPER_BIT1);
  BEEPER_PORT &= ~_BV(BEEPER_BIT2);
  unsigned delay = (F_CPU/8)/freq;
  unsigned count = (duration*(F_CPU/4000))/delay;

  for (unsigned i=0; i<count; i++) {
    BEEPER_PORT ^= _BV(BEEPER_BIT1) | _BV(BEEPER_BIT2);
    _delay_loop_2(delay);
    wdt_reset();
  }
}


static void RTTTL_PlayByte(RTTTL_State *state, char c)
{
  c = toupper(c);
     
  if (state->number && c >= '0' && c <= '9') {
    *state->number *= 10;
    *state->number += c - '0';
    return;
  }
  
  switch (state->state) {
    case TITLE:
      // Skip song title
      //
      if (c == ':')
        state->state = PARAMS;
      break;

    case PARAMS:
      // Modify default parameters
      //
      if (c == 'O') {
        state->number = &state->defaultOctave;
        state->defaultOctave = 0;
      }
      else if (c == 'D') {
        state->number = &state->defaultDuration;
        state->defaultDuration = 0;
      }
      else if (c == 'B') {
        state->number = &state->bpm;
        state->bpm = 0;
      }
      else if (c == ':') {
        state->number = &state->duration;
        state->duration = 0;
        state->state    = SONG;
      }
      break;

    case SONG:
      if (c >= 'A' && c <= 'G') {
        state->note   = (c-'A') * 2;
        state->number = &state->octave;
        state->octave = 0;
      }
      if (c == 'P')
        state->note = -1;
      if (c == '#')
        state->note++;
      if (c == '.')
        state->dotted = true;

      if (c==',' || c==0) {
        if (!state->octave)   state->octave = state->defaultOctave;
        if (!state->duration) state->duration = state->defaultDuration;
        if (state->duration <  1)  state->duration = 1;
        if (state->duration > 32)  state->duration = 32;
        if (state->octave   <  3)  state->octave   = 3;
        if (state->octave   >  8)  state->octave   = 8;

        state->duration = (60000/(state->bpm/4))/state->duration;
        if (state->dotted)
          state->duration += state->duration/2;

        if (state->note >= 0 && state->note <= 13) {
          int frequency = freqtable[state->note];
          if (state->octave > 4)
            frequency <<= state->octave - 4;
          if (state->octave < 4)
            frequency >>= 4 - state->octave;
          Beep(frequency, state->duration - 10);
          Beep(0, 10);
        }
        else {
          Beep(0, state->duration);
        }

        state->note     = -1;
        state->octave   = 0;
        state->duration = 0; 
        state->dotted   = false;
        state->number   = &state->duration;
      }
      break;
  }
}


void RTTTL_Init(RTTTL_State *state)
{
  state->defaultOctave = 5;
  state->defaultDuration = 4;
  state->bpm = 125;
  state->note = -1;
  state->octave = 0;
  state->duration = 0;
  state->dotted = false;
  state->number = NULL;
  state->state = TITLE;
}


/**
 * Play a RTTTL tune from program memory.
 *
 * \param  melody  pointer to RTTTL string
 */
void RTTTL_Play(const char *melody)
{
  RTTTL_State state;
  RTTTL_Init(&state);

  char c;
  do {
    c = *melody++;
    RTTTL_PlayByte(&state, c);
  } while (c != 0);
}


/**
 * Play a RTTTL tune from program memory.
 *
 * \param  melody  pointer to RTTTL string
 */
void RTTTL_Play_P(const PGM_P melody)
{
  RTTTL_State state;
  RTTTL_Init(&state);

  char c;
  do  {
    c = pgm_read_byte_near(melody++);
    RTTTL_PlayByte(&state, c);
  } while (c != 0);
}

