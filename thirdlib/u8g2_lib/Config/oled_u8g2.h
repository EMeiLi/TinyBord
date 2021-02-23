#ifndef __OLED_U8G2_H__
#define __OLED_U8G2_H__

#include "u8g2.h"
#include "main.h"
#include "i2c.h"

#define OLED_ADDRESS 0x78

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
extern void u8g2Init(u8g2_t *u8g2);

#ifdef __cplusplus
}
#endif

#endif