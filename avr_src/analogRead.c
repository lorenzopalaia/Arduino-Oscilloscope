#define __DELAY_BACKWARD_COMPATIBLE__ // _delay_ms function will only accept constants without this

#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "../avr_common/adc.c"
#include "../avr_common/uart.h"

#define CONTINUOUS_MODE 0
#define BUFFERED_MODE 1

#define DISABLED 0
#define ENABLED 1

#define CHANNELS 8

#define MAX_BUF 256

void get_params(uint16_t *freq, uint16_t *samples, uint8_t *mode, uint8_t channels[])
{
    uint8_t recv_buf[MAX_BUF]; // receiving buffer

    UART_getString(recv_buf);

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
    UART_putChar('\0');
}

void buffered_sampling(uint16_t freq, uint16_t samples, uint8_t channels[])
{

    // sending buffers
    uint8_t ch_send_buf[MAX_BUF];
    uint16_t val_send_buf[MAX_BUF];
    uint8_t buf_cnt = 0;

    // check buffer fullness
    // if full
    //     send
    //     empty buffers (unnecessary, just overwrite values)
    //     reset counter
    // sample
    // push to buffer
    // update counter

    for (uint8_t sample = 0; sample < samples; ++sample)
    {
        for (uint8_t ch = 0; ch < CHANNELS; ++ch)
            if (channels[ch] == 1)
            {
                // this will be an async IRQ routine, now it's affecting sampling freq
                // TODO: make it work
                if (buf_cnt == MAX_BUF - 1) // if buffer full
                {
                    // empty and reset
                    for (uint8_t i = 0; i < MAX_BUF; ++i)
                    {
                        put_sample(ch_send_buf[i], val_send_buf[i]); // send
                        // reset, unnecessary, we could simply overwrite
                        ch_send_buf[i] = 0;
                        val_send_buf[i] = 0;
                    }
                    buf_cnt = 0; // reset counter
                }
                // push to buffer
                uint16_t val = ADC_read(ch);
                ch_send_buf[buf_cnt] = ch;
                val_send_buf[buf_cnt] = val;
                // update counter
                ++buf_cnt;
            }
        _delay_ms(1000 / freq); // ms = 1000 / Hz
    }
    UART_putChar('\0');
}

int main(int argc, char *argv[])
{
    UART_init();
    ADC_init();

    while (1) // main loop
    {
        uint16_t freq;
        uint16_t samples;
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