#define __DELAY_BACKWARD_COMPATIBLE__ // _delay_ms function will only accept constants without this

#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include "../avr_common/int.c"
#include "../avr_common/adc.c"
#include "../avr_common/uartINT.c"

#define CONTINUOUS_MODE 0
#define BUFFERED_MODE 1

#define DISABLED 0
#define ENABLED 1

#define CHANNELS 8

#define DEFAULT_BUF_SIZE 32

char TERMINATOR = '-';

// sending buffers and counter used for buffered sending mode
// need to be global in order to be handled by ISR
uint8_t ch_send_buf[DEFAULT_BUF_SIZE];
uint16_t val_send_buf[DEFAULT_BUF_SIZE];
uint16_t buf_cnt = 0;

void get_params(uint16_t *freq, uint16_t *samples, uint8_t *mode, uint8_t channels[])
{
    uint8_t recv_buf[DEFAULT_BUF_SIZE]; // receiving buffer

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
    char out_str[DEFAULT_BUF_SIZE];
    snprintf(out_str, DEFAULT_BUF_SIZE, "%d %d\n", channel, val);
    UART_putString((uint8_t *)out_str);
}

//! stucks on tx, wrong values are sent
// ISR(TIMER5_COMPA_vect)
//{
//     if (buf_cnt >= DEFAULT_BUF_SIZE + 1) // if buffer full
//     {
//         for (uint8_t i = 0; i < DEFAULT_BUF_SIZE - 1; ++i)
//             put_sample(ch_send_buf[i], val_send_buf[i]); // send all buffer elements
//         buf_cnt = 0;                                     // reset counter
//     }
// }

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
    UART_putChar(TERMINATOR);
}

void buffered_sampling(uint16_t freq, uint16_t samples, uint8_t channels[])
{
    // enable interrupt on timer 5
    // INT_enable();

    uint8_t i;
    buf_cnt = 0;

    for (uint16_t sample = 0; sample < samples; ++sample)
    {
        for (uint8_t ch = 0; ch < CHANNELS; ++ch)
        {
            //* enabling next commented lines will make it work without ISR
            if (buf_cnt >= DEFAULT_BUF_SIZE) // if buffer full
            {
                for (i = 0; i < DEFAULT_BUF_SIZE; ++i)
                    put_sample(ch_send_buf[i], val_send_buf[i]); // send all buffer elements
                buf_cnt = 0;                                     // reset counter
            }
            if (channels[ch] == ENABLED)
            {
                uint16_t val = ADC_read(ch);
                ch_send_buf[buf_cnt] = ch;
                val_send_buf[buf_cnt] = val;
                ++buf_cnt;
            }
            _delay_ms(1000 / freq); // ms = 1000 / Hz
        }
    }

    // disable interrupt on timer 5
    // INT_disable();

    // empty residuals in buffer
    for (i = 0; i < buf_cnt; ++i)
        put_sample(ch_send_buf[i], val_send_buf[i]);
    UART_putChar(TERMINATOR);
}

int main(int argc, char *argv[])
{
    // INT_init();
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