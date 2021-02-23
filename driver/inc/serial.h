#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "main.h"
#include "usart.h"
#include <stdint.h>

typedef void (*rx_cb_t)(uint8_t *, uint16_t);

#define SERIAL_GET_LINE 0
#define SERIAL_GET_FULL 1
#define SERIAL_GET_DATA 2

extern void serial_receive_init();
extern void serial_send(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t len);
extern void serial_send_str(UART_HandleTypeDef *huart, uint8_t *str);
extern void serial_send_str_nl(UART_HandleTypeDef *huart, uint8_t *str);
extern void serial_register(UART_HandleTypeDef *huart, rx_cb_t rx_func);

#endif