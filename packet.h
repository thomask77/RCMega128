/*  $Id: packet.h 621 2006-05-15 23:28:33Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.
*/
#ifndef  PACKET_H
#define  PACKET_H

#include <inttypes.h>

// Error Codes
//
#define  ERR_OK               0   ///< No error
#define  ERR_CRC             -1   ///< CRC error after receiving packet
#define  ERR_OVERFLOW        -2   ///< Buffer overflow, packet too long
#define  ERR_TIMEOUT         -3   ///< Timeout during synchronous receive
#define  ERR_UNKNOWN_CMD     -4   ///< Unknown command
#define  ERR_DATA_LENGTH     -5   ///< Data length mismatch
#define  ERR_BATTERY_LOW     -6   ///< Battery low, command ignored

extern void  PKT_SendByte(uint8_t u8);
extern void  PKT_SendUInt16(uint16_t u16);
extern void  PKT_SendUInt32(uint32_t u32);
extern void  PKT_SendBlock(const void *data, int len);
extern void  PKT_EndPacket();

extern void  PKT_BeginReceive(void *data, int len);
extern int   PKT_ReceiveAsync();

#endif
