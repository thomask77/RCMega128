/*  $Id: uart.c 237 2006-01-05 20:48:52Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/

// include files -----
//
#include "uart.h"
#include <avr/wdt.h>
#include <avr/signal.h>

#define  UART_RX_BUFFER_MASK  (UART_RX_BUFFER_SIZE - 1)
#define  UART_TX_BUFFER_MASK  (UART_TX_BUFFER_SIZE - 1)

#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
  #error RX buffer size is not a power of 2
#endif

#if (UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
  #error TX buffer size is not a power of 2
#endif

static volatile char    UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile char    UART_TxBuf[UART_TX_BUFFER_SIZE];
static          uint8_t UART_RxHead, UART_TxTail;
static volatile uint8_t UART_RxTail, UART_TxHead;


SIGNAL(SIG_UART0_RECV)
{
  char c = UDR0;
  uint8_t tail = (UART_RxTail+1) & UART_RX_BUFFER_MASK;
  if (tail != UART_RxHead) {
    UART_RxTail = tail;
    UART_RxBuf[tail] = c;
  }
}


SIGNAL(SIG_UART1_RECV)
{
  char c = UDR1;
  uint8_t tail = (UART_RxTail+1) & UART_RX_BUFFER_MASK;
  if (tail != UART_RxHead) {
    UART_RxTail = tail;
    UART_RxBuf[tail] = c;
  }
}


SIGNAL(SIG_UART0_DATA)
{
  uint8_t head = UART_TxHead;
  if (UART_TxTail != head) {
    head = (head+1) & UART_TX_BUFFER_MASK;
    UART_TxHead = head;
    UDR0 = UART_TxBuf[head];
    UDR1 = UART_TxBuf[head];
  }
  else {
    UCSR0B &= ~_BV(UDRIE0);
  }
}


int UART_GetChar()
{    
  while (UART_RxTail == UART_RxHead)
    wdt_reset();

  uint8_t head = (UART_RxHead+1) & UART_RX_BUFFER_MASK;
  uint8_t data = UART_RxBuf[head];
  UART_RxHead  = head; 

  return data;
}


int UART_PutChar(char data)
{
  uint8_t tail = (UART_TxTail+1) & UART_TX_BUFFER_MASK;
  
  while (tail == UART_TxHead)
    wdt_reset();
  
  UART_TxBuf[tail] = data;
  UART_TxTail = tail;

  UCSR0B |= _BV(UDRIE);
  return 0;
}


void UART_PutString(const char *s)
{
  while (*s) 
    UART_PutChar(*s++);
}


void UART_PutString_P(PGM_P s)
{
  register char c;
  while ((c = pgm_read_byte(s++))) 
    UART_PutChar(c);
}


int UART_CharsAvail()
{
  return (UART_RxTail - UART_RxHead) & UART_RX_BUFFER_MASK;
}


void UART_Init(unsigned divider)
{
  UART_TxHead = 0; UART_TxTail = 0;
  UART_RxHead = 0; UART_RxTail = 0;

  // Enable receiver and transmitter on both UARTs
  // (remember to add pullup-resistors to prevent
  //  garbage characters when device is disconncted)
  //
  UBRR0H = ((uint8_t)(divider>>8)) & ~0x80;
  UBRR1H = ((uint8_t)(divider>>8)) & ~0x80;
  UBRR0L = (uint8_t)divider;
  UBRR1L = (uint8_t)divider;

  UCSR0A = divider & 0x8000 ? _BV(U2X) : 0;
  UCSR1A = divider & 0x8000 ? _BV(U2X) : 0;
  UCSR0B = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
  UCSR1B = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
}
