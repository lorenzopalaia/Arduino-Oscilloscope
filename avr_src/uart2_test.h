#pragma once

void UART_init(void);
uint8_t UART_getString(uint8_t *buf);
void UART_putString(uint8_t *buf);