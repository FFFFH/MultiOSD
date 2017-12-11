/*************************************************************************
Title: Interrupt UART library with receive/transmit circular buffers
Author: Peter Fleury <pfleury@gmx.ch> http://jump.to/fleury,
        UncleRus <unclerus@gmail.com>
File: $Id: uart.c,v 1.6.2.1 2007/07/01 11:14:38 peter Exp $

DESCRIPTION:
    An interrupt is generated when the UART has finished transmitting or
    receiving a byte. The interrupt handling routines use circular buffers
    for buffering received and transmitted data.

    The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE macros define
    the buffer size in bytes. Note that these values must be a
    power of 2.

NOTES:
    Based on original library by Peter Fluery, Tim Sharpe, Nicholas Zambetti.
    Based on Atmel Application Note AVR306

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include "../../config.h"

#ifdef UART_STDIO
#   include <stdio.h>
#endif

#ifndef UART_RX_BUFFER_SIZE
    #define UART_RX_BUFFER_SIZE 64
#endif

#ifndef UART_TX_BUFFER_SIZE
    #define UART_TX_BUFFER_SIZE 64
#endif

#define UART_BAUD_SELECT(baud_rate)  (((F_CPU) + 8UL * (baud_rate)) / (16UL * (baud_rate)) - 1UL)
#define UART_BAUD_SELECT_DOUBLE_SPEED(baud_rate) ( ((((F_CPU) + 4UL * (baud_rate)) / (8UL * (baud_rate)) - 1UL)) | 0x8000)

#define UART_BR_9600 0
#define UART_BR_19200 1
#define UART_BR_38400 2
#define UART_BR_57600 3
#define UART_BR_115200 4

#if ((UART_RX_BUFFER_SIZE + UART_TX_BUFFER_SIZE) >= (RAMEND - 0x60))
    #error "size of UART_RX_BUFFER_SIZE + UART_TX_BUFFER_SIZE larger than size of SRAM"
#endif

#define UART_FRAME_ERROR      0x1000              /* Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0800              /* Overrun condition by UART   */
#define UART_PARITY_ERROR     0x0400              /* Parity Error by UART        */
#define UART_BUFFER_OVERFLOW  0x0200              /* receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /* no receive data available   */

#include "platform.h"

namespace uart_utils
{

extern const uint16_t bitrates[];
extern const uint16_t get_bitrate(uint8_t br_type, uint8_t def = 0);

}  // namespace uart_utils

namespace uart0
{

void init(uint16_t baud_rate);
uint16_t receive();
void send(uint8_t data);
void send_string(const char *s);
void send_string_p(const char *progmem_s);

#ifdef UART_STDIO
extern FILE stream;
#endif

}

#ifdef ATMEGA_USART1
namespace uart1
{

void init(unsigned int baud_rate);
uint16_t receive();
void send(unsigned char data);
void send_string(const char *s);
void send_string_p(const char *s);

#ifdef UART_STDIO
extern FILE stream;
#endif

}
#endif

#endif /* UART_H_ */
