/*  $Id: misc.c 170 2005-11-30 00:49:33Z kindler $
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
#include "misc.h"
#include <stdio.h>
#include <ctype.h>
#include <avr/pgmspace.h>

/**
 * Generates a nice hexdump of a memory area.
 *
 * \param  mem     pointer to memory to dump
 * \param  length  how many bytes to dump
 */
void hexdump(void *mem, unsigned length)
{
  char  line[80];
  char *src = (char*)mem;

  printf_P(
    PSTR(
      "dumping %u bytes from %p\r\n"
      "       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F    0123456789ABCDEF\r\n"
    ), length, src
  );
  for (unsigned i=0; i<length; i+=16, src+=16) {
    char *t = line;

    t += sprintf(t, "%04x:  ", i);
    for (int j=0; j<16; j++) {
      if (i+j < length)
        t += sprintf(t, "%02X", src[j] & 0xff);
      else
        t += sprintf(t, "  ");
      t += sprintf(t, j%2 ? " " : "-");
    }

    t += sprintf(t, "  ");
    for (int j=0; j<16; j++) {
      if (i+j < length) {
        if (isprint((unsigned char)src[j]))
          t += sprintf(t, "%c", src[j]);
        else
          t += sprintf(t, ".");
      } else {
          t += sprintf(t, " ");
      }
    }

    t += sprintf(t, "\r\n");
    printf("%s", line);
  }
}
