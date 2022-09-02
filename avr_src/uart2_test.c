#define STRLEN 256
#define BAUD 19600
#define MYUBRR (F_CPU / 16 / BAUD - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

void USART_putString(char *);

volatile char out[STRLEN]; // out buffer
volatile char in[STRLEN];
volatile unsigned int outp; // out buffer position
volatile unsigned int inp;
volatile unsigned char tx; // var to check if i'm currently transmitting
volatile unsigned char rx;

void USART_Init()
{
    // Set baud rate
    UBRR0H = (uint8_t)(MYUBRR >> 8);
    UBRR0L = (uint8_t)MYUBRR;

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               /* 8-bit data */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); /* Enable RX and TX */

    sei();

    tx = 0;
    rx = 0;
}

void USART_putString(char *buf)
{
    strcpy(out, buf);
    outp = 0;
    tx = 1;
    UCSR0B |= (1 << UDRIE0);
    while (tx == 1)
        ; // wait until msg delivered
}

void USART_getString(char *buf)
{
    memset(in, 0, sizeof(in));
    inp = 0;
    rx = 1;
    // UCSR0A |= (1 << UDRIE0);
    while (rx == 1)
        ;                // wait until msg received
    strcpy(buf, in + 1); // why +1 ?
}

ISR(USART0_RX_vect)
{
    // UCSR0A &= ~(1 << UDRIE0);
    if (UDR0 == '\n')
        rx = 0;
    else
    {
        in[inp] = UDR0;
        ++inp;
        // UCSR0A |= (1 << UDRIE0);
    }
}

ISR(USART0_UDRE_vect)
{
    // UCSR0B &= ~(1 << UDRIE0);
    UDR0 = out[outp];
    if (out[outp] == '\0')
        tx = 0;
    else
    {
        ++outp;
        UCSR0B |= (1 << UDRIE0);
    }
}

// test main
int main(void)
{
    USART_Init();

    // USART_putString("Hello ");
    // USART_putString("world\n");

    char buf[STRLEN];

    while (1)
    {
        USART_getString(buf);
        USART_putString(buf);
    }

    return 0;
}