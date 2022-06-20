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

#define N_CHANNELS 8

#define MAX_BUF 256

void continuous_sample(uint16_t freq, uint8_t samples, uint8_t channels[])
{
    uint16_t utoa_tmp_buffer[1];
    for (uint8_t sample = 0; sample < samples; ++sample)
    {
        for (uint8_t ch = 0; ch < N_CHANNELS; ++ch)
            if (channels[ch] == 1)
            {
                uint16_t val = read_adc(ch);
                // format of string put in UART will be "sample ch val" in order to be tokenized by client
                UART_putString((uint8_t *)utoa(sample, (char *)utoa_tmp_buffer, 10)); // unsigned to string conversion (base 10)
                UART_putString((uint8_t *)" ");
                UART_putString((uint8_t *)utoa(ch, (char *)utoa_tmp_buffer, 10));
                UART_putString((uint8_t *)" ");
                UART_putString((uint8_t *)utoa(val, (char *)utoa_tmp_buffer, 10));
                UART_putString((uint8_t *)"\n");
            }
        _delay_ms(1000 / freq); // ms = 1000 / Hz
    }
}

int main(int argc, char *argv[])
{
    UART_init();
    adc_init();

    uint8_t recv_buf[MAX_BUF];  // receiving buffer
    uint16_t send_buf[MAX_BUF]; // sending buffer

    while (1)
    {
        uint16_t freq;
        uint8_t samples;
        uint8_t mode;
        uint8_t channels[N_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0}; // 0: disabled, 1:enabled
        // ask for params, TODO: delete, just for dev
        UART_putString((uint8_t *)"Ready for next sampling. Waiting for params [freq samples mode, channels: [1 2 ...]]\n");
        UART_getString(recv_buf);

        freq = atoi(strtok((char *)recv_buf, " "));
        samples = atoi(strtok(NULL, " "));
        mode = atoi(strtok(NULL, " "));
        char *channels_token = strtok(NULL, ", ");
        while (channels_token != NULL)
        {
            channels[atoi(channels_token)] = 1; // enable channel
            channels_token = strtok(NULL, " ");
        }

        if (mode == CONTINUOUS_MODE)
        {
            // continuous mode sampling + sending function
            continuous_sample(freq, samples, channels);
        }
        else if (mode == BUFFERED_MODE)
        {
            // buffered mode sampling + sending function
        }
    }
    return 0;
}