#pragma once

#define  ADC_OK            0x0000

uint32_t adc_init(void);
void     adc_isr(void *arg);
void     adc_intack(uint8_t int_type);
void     adc_run(uint32_t flags, uint32_t packets);
uint32_t adc_version(void);

