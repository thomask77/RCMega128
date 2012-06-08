/*  $Id: uart.h 621 2006-05-15 23:28:33Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/
#ifndef UART_H
#define UART_H

#include <avr/pgmspace.h>

#define UART_DIVIDER(baudRate)      ((F_CPU)/((baudRate)*16l)-1)
#define UART_DIVIDER_U2X(baudRate)  (((F_CPU)/((baudRate)*8l)-1) | 0x8000)

#define UART_RX_BUFFER_SIZE  256     ///< Size of receive buffer
#define UART_TX_BUFFER_SIZE  128     ///< Size of transmit buffer

extern  void UART_Init(unsigned divider);
extern  int  UART_GetChar();
extern  int  UART_CharsAvail();
extern  int  UART_PutChar(char c);
extern  void UART_PutString(const char *s);
extern  void UART_PutString_P(PGM_P  s);

#endif

