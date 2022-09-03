#define __DELAY_BACKWARD_COMPATIBLE__ // _delay_ms function will only accept constants without this

#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "../avr_common/int.c"
#include "../avr_common/adc.c"
#include "../avr_common/uartINT.c" //? uartINT.c currently not working (but uartINT_test.c works ...), switch with uart.c

#define CONTINUOUS_MODE 0
#define BUFFERED_MODE 1

#define DISABLED 0
#define ENABLED 1

#define CHANNELS 8

#define MAX_BUF 256

// sending buffers and counter used for buffered sending mode
// need to be global in order to be handled by ISR
uint8_t ch_send_buf[MAX_BUF];
uint16_t val_send_buf[MAX_BUF];
uint16_t buf_cnt = 0;

void get_params(uint16_t *freq, uint16_t *samples, uint8_t *mode, uint8_t channels[])
{
    uint8_t recv_buf[MAX_BUF]; // receiving buffer

    UART_getString(recv_buf);

    // strtok is better than sscanf in this case because lenght of enabled channels string is variable
    *freq = atoi(strtok((char *)recv_buf, " "));
    *samples = atoi(strtok(NULL, " "));
    *mode = atoi(strtok(NULL, " "));
    char *channels_token = strtok(NULL, ", ");
    while (channels_token != NULL)
    {
        channels[atoi(channels_token)] = ENABLED; // enable channel
        channels_token = strtok(NULL, " ");
    }
}

void put_sample(uint8_t channel, uint16_t val)
{
    char out_str[MAX_BUF];
    snprintf(out_str, MAX_BUF, "%d %d\n", channel, val);
    UART_putString((uint8_t *)out_str);
}

//! this ISR is not compromising uartINT comunication but data is not beign transfered
ISR(TIMER5_COMPA_vect)
{
    if (buf_cnt >= MAX_BUF - 1) // if buffer full
    {
        for (uint8_t i = 0; i < MAX_BUF - 1; ++i)
            put_sample(ch_send_buf[i], val_send_buf[i]); // send all buffer elements
        buf_cnt = 0;                                     // reset counter
    }
}

void continuous_sampling(uint16_t freq, uint16_t samples, uint8_t channels[])
{
    for (uint8_t sample = 0; sample < samples; ++sample)
    {
        for (uint8_t ch = 0; ch < CHANNELS; ++ch)
            if (channels[ch] == 1)
            {
                uint16_t val = ADC_read(ch);
                put_sample(ch, val);
            }
        _delay_ms(1000 / freq); // ms = 1000 / Hz
    }
    // UART_putChar('\0');// from uart.c
    UART_putString((uint8_t *)"-"); // from uart2.c: not working
}

void buffered_sampling(uint16_t freq, uint16_t samples, uint8_t channels[])
{
    // enable interrupt on timer 5
    INT_enable();

    for (uint8_t sample = 0; sample < samples; ++sample)
    {
        for (uint8_t ch = 0; ch < CHANNELS; ++ch)
            if (channels[ch] == ENABLED)
            {
                // make reading and push to buffers
                // this cycle will be interrupted by ISR then will resume
                // TODO: busy waiting in order to wait until the empty interrupt occurs instead of simple interrupt
                uint16_t val = ADC_read(ch);
                ch_send_buf[buf_cnt] = ch;
                val_send_buf[buf_cnt] = val;
                ++buf_cnt;
            }
        _delay_ms(1000 / freq); // ms = 1000 / Hz
    }

    // disable interrupt on timer 5
    INT_disable();

    // empty residuals in buffer
    for (uint8_t i = 0; i < buf_cnt; ++i)
        put_sample(ch_send_buf[i], val_send_buf[i]);
    // UART_putChar('\0'); // from uart.c
    UART_putString((uint8_t *)"-"); // from uart2.c: not working
}

int main(int argc, char *argv[])
{
    INT_init();
    UART_init();
    ADC_init();

    while (1) // main loop
    {
        uint16_t freq, samples;
        uint8_t mode;
        uint8_t channels[CHANNELS] = {DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED, DISABLED};

        get_params(&freq, &samples, &mode, channels);

        if (mode == CONTINUOUS_MODE)
            continuous_sampling(freq, samples, channels);
        else if (mode == BUFFERED_MODE)
            buffered_sampling(freq, samples, channels);
    }
    return 0;
}