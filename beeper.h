/*  $Id: beeper.h 246 2006-01-08 21:28:50Z kindler $
    loosely based on Craig Peacock's PIC16F87x sourcecode
    http://www.beyondlogic.org/pic/ringtones.htm

    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/
#ifndef BEEPER_H
#define BEEPER_H

#include <avr/pgmspace.h>
#include <avr/io.h>

// Port bits for the beeper..
//
#define BEEPER_PORT  PORTD
#define BEEPER_DDR   DDRD
#define BEEPER_BIT1  PD6
#define BEEPER_BIT2  PD7


extern void Beep(unsigned freq, unsigned length);

extern void RTTTL_Play  (const char  *melody);
extern void RTTTL_Play_P(const PGM_P  melody);

#endif
