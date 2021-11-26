#pragma once

int  xlprint(const char *format, ...);
int  xlprints(char *buf, const char *format, ...);
void xlprint_open(UART_HandleTypeDef *huart);
void xlprint_isr(void);
void xlprint_ack(void);
