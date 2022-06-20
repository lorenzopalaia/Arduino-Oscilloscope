#pragma once

void UART_init(void);
void UART_putChar(uint8_t c);
uint8_t UART_getChar(void);
uint8_t UART_getString(uint8_t *buf);
void UART_putString(uint8_t *buf);