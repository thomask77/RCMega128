/*  $Id: packet.c 258 2006-01-11 18:30:52Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/

// include files -----
//
#include "packet.h"
#include "uart.h"
#include <avr/io.h>
#include <avr/crc16.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <stdio.h>

#define  PKT_END      0xC0     ///< End-of-packet symbol
#define  PKT_ESC      0xDB     ///< Escape symbol
#define  PKT_ESC_END  0xDC     ///< Escaped C0 byte
#define  PKT_ESC_ESC  0xDD     ///< Escaped DB byte

static   uint16_t     txCrc;   ///< CRC checksum of current packet
static   int          txLen;   ///< Length of current packet

static   char        *rxBuf;   ///< Pointer to receive buffer
static   uint16_t     rxBufSize;   ///< Length of receive buffer
static   uint16_t     rxPos;   ///< Current position in receive buffer
static   bool         rxEsc;   ///< Last byte was an escape symbol


/**
 * Send a single byte.
 *
 * \param  u8  uint8_t to send
 */
inline void PKT_SendByte(uint8_t u8)
{
  if (txLen == 0) {
    UART_PutChar(PKT_END);
    txCrc = 0xffff;
  }
  switch (u8) {
    case PKT_END:
      UART_PutChar(PKT_ESC);
      UART_PutChar(PKT_ESC_END);
      break;
    case PKT_ESC:
      UART_PutChar(PKT_ESC);
      UART_PutChar(PKT_ESC_ESC);
      break;
    default:
      UART_PutChar(u8);
      break;
  }
  txCrc = _crc_ccitt_update(txCrc, u8);
  txLen++;
}


/**
 * Send a 2 byte integer.
 *
 * \param  u16  uint16_t to send
 */
inline void PKT_SendUInt16(uint16_t u16)
{
  PKT_SendByte(u16);
  PKT_SendByte(u16 >>  8);
}

/**
 * Send a 4 byte integer.
 *
 * \param  u32  uint32_t to send
 */
inline void PKT_SendUInt32(uint32_t u32)
{
  PKT_SendByte(u32);
  PKT_SendByte(u32 >>  8);
  PKT_SendByte(u32 >> 16);
  PKT_SendByte(u32 >> 24);
}

/**
 * Send a block of data.
 *
 * \param  data  pointer to data
 * \param  len   length of data
 */
inline void PKT_SendBlock(const void *data, int len)
{
  const char *c = data;
  while (len--)
    PKT_SendByte(*c++);
}


/**
 * Send end-of-packet.
 * 
 */
inline void PKT_EndPacket()
{
  PKT_SendUInt16(txCrc);
  UART_PutChar(PKT_END);
  txLen = 0;
}


/**
 * Begin asynchronous packet receive.
 * 
 * \note   Packets are always received including the 16bit CRC, so
 *         the buffer needs to be 2 bytes bigger than the user data.
 *
 * \param  data   pointer to packet receive buffer
 * \param  len    length of packet receive buffer
 */
inline void PKT_BeginReceive(void *data, int len)
{
  rxBuf = data;
  rxBufSize = len;
  rxPos = 0;
  rxEsc = false;
}



/**
 * Receive a packet asynchronously.
 * \see  PKT_BeginReceive()
 * \return 
 *    0  No packet received
 *   >0  Length of received packet (including 2 bytes of CRC)
 *   <0  Error code
 */
inline int PKT_ReceiveAsync()
{
  int length;
  while (UART_CharsAvail()) {
    char   c = UART_GetChar();

    switch (c) {
      case PKT_ESC:
        rxEsc = true;
        break;
      
      case PKT_END:
        length = rxPos;
        rxPos  = 0;
        rxEsc  = false;

        if (length >= 2) {
          unsigned crc = 0xffff;
          for (int i=0; i<length; i++)
            crc = _crc_ccitt_update(crc, rxBuf[i]);
          if (crc == 0)
            return length-2;
          else
            return ERR_CRC;
        }
        break;

      default:
        if (rxPos < rxBufSize) {
          if (rxEsc && c==PKT_ESC_END) 
            c = PKT_END;
          if (rxEsc && c==PKT_ESC_ESC)
            c = PKT_ESC;
          rxBuf[rxPos++] = c;
          rxEsc = false;
        }
        else {
          rxPos = 0;
          rxEsc = false;
          return ERR_OVERFLOW;
        }
        break;
    }
  }  
  return 0;
}

