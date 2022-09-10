#define STRLEN 256
#define BAUD 19600
#define MYUBRR (F_CPU / 16 / BAUD - 1)

#define UDRIE_on() UCSR0B |= (1 << UDRIE0);
#define UDRIE_off() UCSR0B &= ~(1 << UDRIE0);

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdint.h>
#include <util/delay.h>

volatile uint8_t tx, rx;            // vars to check if i'm currently receiving or transmitting
volatile uint8_t in_char, out_char; // tmp vars for receiving or sending

void UART_init()
{
  // Set baud rate
  UBRR0H = (uint8_t)(MYUBRR >> 8);
  UBRR0L = (uint8_t)MYUBRR;

  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);                               // 8-bit data
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0); // Enable RX and TX and RX + TX interrupts

  sei();
}

uint8_t UART_getChar()
{
  rx = 1;
  while (rx == 1)
    ;
  return in_char;
}

uint8_t UART_getString(uint8_t *buf)
{
  uint8_t *b0 = buf;
  while (1)
  {
    uint8_t c = UART_getChar();
    *buf = c;
    ++buf;
    if (c == 0)
      return buf - b0;
    if (c == '\n' || c == '\r')
    {
      *buf = 0;
      ++buf;
      return buf - b0;
    }
  }
}

ISR(USART0_RX_vect)
{
  in_char = UDR0;
  rx = 0;
}

void UART_putChar(uint8_t c)
{
  out_char = c;
  tx = 1;
  while (tx == 1)
    ;
}

void UART_putString(uint8_t *buf)
{
  UDRIE_on();
  while (*buf)
  {
    UART_putChar(*buf);
    ++buf;
  }
  UDRIE_off();
}

ISR(USART0_UDRE_vect)
{
  // UCSR0B &= ~(1 << UDRIE0);
  UDR0 = out_char;
  // UCSR0B |= (1 << UDRIE0);
  tx = 0;
}