/*  $Id: adc.c 171 2005-11-30 14:43:53Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/
#ifndef SERVO_H
#define SERVO_H

extern void SRV_SetPositions(unsigned *target);
extern void SRV_GetPositions(unsigned *current);
extern void SRV_Init();

#endif
