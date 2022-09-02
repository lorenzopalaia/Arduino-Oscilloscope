#include <avr/interrupt.h>

// setup interrupt on timer 5
void INT_init()
{
    const uint8_t timer_duration_ms = 1;
    TCCR5A = 0;
    TCCR5B = (1 << WGM52) | (1 << CS50) | (1 << CS52);
    uint16_t ocrval = (uint16_t)(15.62 * timer_duration_ms);
    OCR5A = ocrval;
}

void INT_enable()
{
    cli();
    TIMSK5 |= (1 << OCIE5A);
    sei();
}

void INT_disable()
{
    cli();
    TIMSK5 &= ~(1 << OCIE5A);
    sei();
}
