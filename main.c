/*  $Id: main.c 621 2006-05-15 23:28:33Z kindler $
    Copyright (c)2005 by Thomas Kindler, thomas.kindler@gmx.de

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version. Read the
    full License at http://www.gnu.org/copyleft for more details.

    NOTE:
      * Remember to set the correct fuse values:
        - Extended: 0xFF, High: 0xC8, Low: 0xFF, Lock: 0xEF

    TODO:
      * Use Timer0 as millisecond tick
      * Use Timer1 output for beeper
        - enable beep while moving servos for battery low warning..
      * Use Timer3 for Servo In/Output

    NICE-TO-HAVE:
      * Servo modes without readback
*/

// include files -----
//
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/crc16.h>
#include "misc.h"
#include "adc.h"
#include "uart.h"
#include "packet.h"
#include "beeper.h"
#include "servo.h"

// I/O Port definitions
//
#define   LED_PORT           PORTD   ///< LED port
#define   LED_DDR            DDRD    ///< LED data direction
#define   LED1_BIT           PD4     ///< LED1 (busy) signal
#define   LED2_BIT           PD5     ///< LED2 (online) signal

#define   GSEL_PORT          PORTE   ///< Accelerometer g select port
#define   GSEL_DDR           DDRE    ///< Accelerometer g select data direction
#define   GSEL_GS1_BIT       PE3     ///< Accelerometer GS1 signal
#define   GSEL_GS2_BIT       PE4     ///< Accelerometer GS2 signal

// Serial commands
//
#define   CMD_NOP            0x00    ///< Do nothing
#define   CMD_GET_BOARD_INFO 0x01    ///< Software revision, F_CPU, etc.
#define   CMD_BEEP           0x02    ///< Beep
#define   CMD_READ_EEPROM    0x03    ///< Read  EEPROM
#define   CMD_WRITE_EEPROM   0x04    ///< Write EEPROM
#define   CMD_READ_SERVOS    0x05    ///< Get current servo positions
#define   CMD_WRITE_SERVOS   0x06    ///< Set servo target positions
#define   CMD_READ_SENSORS   0x07    ///< Read analog inputs
#define   CMD_WRITE_CONFIG   0x08    ///< Write configuration to EEPROM
#define   CMD_SET_MIN_BATT   0x09    ///< Set minimum battery level

// Board configuration
//
#define   PROTOCOL_VERSION   0x0130  ///< Protocol version
#define   ZOMBIE_TIMEOUT     100     ///< Force a servo update after 100ms
#define   ZOMBIE_MAXUPDATES  10      ///< Maximum number of zombie cycles

// EEPROM config area
// (allocated from the top)
//
typedef struct {
  unsigned  minBattery;
  unsigned  crc;
} ConfigArea;


ConfigArea  configArea;
char        packet[128];
unsigned    targetPositions[24];
int         zombieUpdates;
uint8_t     batteryLow;


void InitMCU()
{
  // Enable watchdog timer
  //
  wdt_enable(WDTO_60MS);

  // Set up IO ports
  // 
  LED_DDR   |= _BV(LED1_BIT) | _BV(LED2_BIT);
  LED_PORT  |= _BV(LED2_BIT);
  GSEL_DDR  |= _BV(GSEL_GS1_BIT) | _BV(GSEL_GS2_BIT);

  if (MCUCSR & (_BV(WDRF) | _BV(BORF))) {
    // Watchdog or brownout reset detected
    //
    RTTTL_Play_P(PSTR(":d=4,b=160:16a,16g,2a,16g,16f,16d,16e,4c#,d."));
  }

  MCUCSR = 0;
  ADC_Init();
  SRV_Init();

  // Set up zombie timeout
  //
  TCCR3B = _BV(CS32) | _BV(CS30) | _BV(WGM32);
  OCR3A  = (ZOMBIE_TIMEOUT * (F_CPU/1024)) / 1000;

  // Initialize serial ports
  //
  UART_Init(UART_DIVIDER_U2X(115200));
  fdevopen(UART_PutChar, UART_GetChar, 0);

  // Load configArea from EEPROM
  //
  unsigned crc = 0xffff;
  for (int i=4096-sizeof(configArea); i<4096; i++) {
    ((char*)&configArea)[i] = eeprom_read_byte((void*)i);
    crc = _crc_ccitt_update(crc, ((char*)&configArea)[i]);
  }
  if (crc != 0) {
    // loading failed, fill with default values.
    //
    memset(&configArea, 0, sizeof(configArea));
  }

  RTTTL_Play_P(PSTR(":d=16,b=160:c,c6."));

  LED_PORT |=  _BV(LED1_BIT);
  LED_PORT &= ~_BV(LED2_BIT);
  
  // Enable interrupts.
  //
  sei();
}


/**
 * Dispatch command and send response data.
 *
 */
void Dispatch(char *data, uint16_t length)
{
  if (length < 2) {
    PKT_SendByte(0);
    PKT_SendByte(0);
    PKT_SendByte(ERR_DATA_LENGTH);
    PKT_EndPacket();
    return;
  }
  
  int seqnum  = data[0];
  int command = data[1];
  PKT_SendByte(seqnum);
  PKT_SendByte(command);

  data += 2; length -= 2;
  switch (command) {
    case CMD_NOP: {
      PKT_SendByte(ERR_OK);
      break;
    }

    case CMD_BEEP: {
      PKT_SendByte(ERR_OK);
      data[length-1] = 0;
      RTTTL_Play(data);
      break;
    }

    case CMD_READ_SERVOS: {
      PKT_SendByte(ERR_OK);
      unsigned tmp[24];
      SRV_GetPositions(tmp);
      PKT_SendBlock(tmp, sizeof(tmp));
      break;
    }

    case CMD_WRITE_SERVOS: {
      if (length < sizeof(targetPositions)) {
        PKT_SendByte(ERR_DATA_LENGTH);
        break;
      }
      if (batteryLow) {
        PKT_SendByte(ERR_BATTERY_LOW);
        break;
      }

      PKT_SendByte(ERR_OK);
      memcpy(targetPositions, data, sizeof(targetPositions));
      SRV_SetPositions(targetPositions);

      // reset zombie timeout
      //
      TCNT3         = 0;
      zombieUpdates = 0;
      break;
    }

    case CMD_READ_SENSORS: {
      if (length < 1) {
        PKT_SendByte(ERR_DATA_LENGTH);
        break;
      }
      PKT_SendByte(ERR_OK);

      // Set accelerometer sensitivity
      //
      uint8_t tmp = GSEL_PORT & ~(_BV(GSEL_GS1_BIT) | _BV(GSEL_GS2_BIT));
      if (data[0] & 1)  tmp |= _BV(GSEL_GS1_BIT);
      if (data[0] & 2)  tmp |= _BV(GSEL_GS2_BIT);
      GSEL_PORT = tmp;

      // Read gyroscope in differential mode
      //
      ADMUX = _BV(REFS0) | 0x10;        // 1x (ADC0 - ADC1)
      ADC_Read();                       // throw away first conversion
      PKT_SendUInt16(ADC_Read());

      ADMUX = _BV(REFS0) | 0x09;        // 10x (ADC1 - ADC0)
      ADC_Read();                       // throw away first conversion
      PKT_SendUInt16(ADC_Read());

      // Read accelerometer and battery voltage
      //
      ADMUX = _BV(REFS0);               // single ended, AVcc
      ADC_Read();                       // throw away first conversion
      for (int i=2; i<6; i++) {
        ADMUX = _BV(REFS0) | i;
        PKT_SendUInt16(ADC_Read());
      }

      // Read PSD Sensors against 2.56V bandgap
      //
      ADMUX = _BV(REFS0) | _BV(REFS1);  // 2.56V bandgap
      ADC_Read();                       // throw away first conversion

      ADMUX = _BV(REFS0) | _BV(REFS1) | 6;
      PKT_SendUInt16(ADC_Read());
      ADMUX = _BV(REFS0) | _BV(REFS1) | 7;
      PKT_SendUInt16(ADC_Read());

      // Read CPU voltage
      //
      ADMUX = _BV(REFS0) | 0x1E;        // 1.23V bandgap against AVcc
      ADC_Read();
      PKT_SendUInt16(ADC_Read());

      break;
    }

    case CMD_GET_BOARD_INFO: {
      PKT_SendByte(ERR_OK);
      PKT_SendUInt16(PROTOCOL_VERSION);
      PKT_SendUInt32(F_CPU);
      
      // Read 1.23V bandgap voltage reference against AVcc
      //
      ADMUX = _BV(REFS0) | 0x1E;
      ADC_Read();
      PKT_SendUInt16(ADC_Read());
      break;
    }

    case CMD_WRITE_EEPROM: {
      if (length < 2) {
        PKT_SendByte(ERR_DATA_LENGTH);
        break;
      }
      PKT_SendByte(ERR_OK);
      uint8_t *addr = (uint8_t *)(*(uint16_t*)&data[0]);
      for (int i=2; i<length; i++) {
        eeprom_write_byte(addr++, data[i]);
        wdt_reset();
      }
      break;
    }
 
    case CMD_READ_EEPROM: {
      if (length < 4) {
        PKT_SendByte(ERR_DATA_LENGTH);
        break;
      }

      PKT_SendByte(ERR_OK);
      uint8_t  *addr   = (uint8_t *)(*(uint16_t*)&data[0]);
      uint16_t  length = *(uint16_t*)&data[2];
      while (length--)
        PKT_SendByte(eeprom_read_byte(addr++));
      break;
    }

    case CMD_WRITE_CONFIG: {
      configArea.crc = 0xffff;
      for (int i=0; i<sizeof(configArea)-2; i++)
        configArea.crc = _crc_ccitt_update(configArea.crc, ((char*)&configArea)[i]);
      for (int i=4096-sizeof(configArea); i<4096; i++) {
        eeprom_write_byte((void*)i, ((char*)&configArea)[i]);
        wdt_reset();
      }
    }

    case CMD_SET_MIN_BATT: {
      if (length < 2) {
        PKT_SendByte(ERR_DATA_LENGTH);
        break;
      }

      PKT_SendByte(ERR_OK);
      configArea.minBattery = *(uint16_t*)&data[0];
    }

    default:
      PKT_SendByte(ERR_UNKNOWN_CMD);
      break;
  }

  PKT_EndPacket();
}


int main()
{
  InitMCU();

  PKT_BeginReceive(packet, sizeof(packet));
  for (;;) {
    wdt_reset();

    // Check battery
    //
    ADMUX = _BV(REFS0) | 5;
    batteryLow = ADC_Read() < configArea.minBattery;
    
    if (batteryLow)
      RTTTL_Play_P(PSTR("::c6"));

    // Receive command packet
    //
    int length = PKT_ReceiveAsync();
    if (length > 0) {
      LED_PORT &= ~_BV(LED1_BIT);
      Dispatch(packet, length);
      LED_PORT |=  _BV(LED1_BIT);
    }

    // Check for zombie timeout
    //
    if ((ETIFR & _BV(OCF3A)) && zombieUpdates < ZOMBIE_MAXUPDATES) {
      SRV_SetPositions(targetPositions);
      ETIFR |= _BV(OCF3A);
      zombieUpdates++;
    }
  }
}
