#include "serial.h"
#include "cmsis_os2.h"
#include <string.h>
#include <cstdio>

#define MAX_BUFFER_SIZE 256
#define MSGQUEUE_OBJECTS 8

typedef struct
{
    UART_HandleTypeDef *huart;
    uint8_t type;
    uint8_t rxbuf[MAX_BUFFER_SIZE + 1];
    uint16_t rxlen;
} serial_data_t;

static serial_data_t uart6_data;

typedef struct
{
    rx_cb_t uart6_rx_cb;
    osMessageQueueId_t msgque;
} serial_ctl_t;

serial_ctl_t serial_ctl;

static void serial_receive_handle(serial_data_t *data);
static void serial_data_handle(serial_data_t *data);

const osThreadAttr_t serial_thread_attr = {
    .stack_size = 2048,
    // .priority = osPriorityAboveNormal,
};

__NO_RETURN static void serial_rx_thread(void *argument)
{
    (void)argument;
    serial_data_t data;
    while (1)
    {
        // printf("serial thread> wait\r\n");
        osMessageQueueGet(serial_ctl.msgque, &data, NULL, osWaitForever);
        // printf("serial thread> get\r\n");
        // printf("serial thread> uart:%p, type:%d, len:%d\r\n", data.huart, data.type, data.rxlen);
        // printf("serial thread> %s\r\n", data.rxbuf);
        serial_data_handle(&data);
        // printf("serial thread> done\r\n");
    }
}

extern "C"
{
    void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
    {
        if (huart == &huart6)
        {
            serial_receive_handle(&uart6_data);
        }
    }
}

void serial_receive_init()
{
    uart6_data.huart = &huart6;
    uart6_data.rxlen = 0;
    serial_ctl.uart6_rx_cb = NULL;
    serial_ctl.msgque = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(serial_data_t), NULL);
    osThreadNew(serial_rx_thread, NULL, &serial_thread_attr);
    HAL_UART_Receive_IT(uart6_data.huart, uart6_data.rxbuf + uart6_data.rxlen, 1);
}

void serial_send(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        while ((huart->Instance->SR & 1 << 7) == 0)
            ;
        huart->Instance->DR = buf[i];
    }
}

void serial_send_str(UART_HandleTypeDef *huart, uint8_t *str)
{
    for (uint16_t i = 0; i < strlen((char *)str); i++)
    {
        while ((huart->Instance->SR & 1 << 7) == 0)
            ;
        huart->Instance->DR = str[i];
    }
}

void serial_send_str_nl(UART_HandleTypeDef *huart, uint8_t *str)
{
    for (uint16_t i = 0; i < strlen((char *)str); i++)
    {
        while ((huart->Instance->SR & 1 << 7) == 0)
            ;
        huart->Instance->DR = str[i];
    }
    while ((huart->Instance->SR & 1 << 7) == 0)
        ;
    huart->Instance->DR = '\r';
    while ((huart->Instance->SR & 1 << 7) == 0)
        ;
    huart->Instance->DR = '\n';
}

void serial_register(UART_HandleTypeDef *huart, rx_cb_t rx_func)
{
    if (huart == &huart6)
    {
        serial_ctl.uart6_rx_cb = rx_func;
    }
}

#define DATA_HEADER     "+IPD"

static void serial_receive_handle(serial_data_t *data)
{
    static bool is_data = false;
    static bool get_len = false;
    static uint16_t data_len = 0;

    if (data->rxlen == strlen(DATA_HEADER))
    {
        if (memcmp(data->rxbuf, DATA_HEADER, strlen(DATA_HEADER)) == 0)
        {
            is_data = true;
            data_len = 0;
        }
    }

    if (is_data)
    {
        uint16_t pos = data->rxlen;
        if (!get_len && data->rxbuf[pos] == ':')
        {
            uint16_t tmp = 1;
            while (data->rxbuf[pos-1] != ',')
            {
                data_len = data_len + (data->rxbuf[pos-1]-'0')*tmp;
                pos --;
                tmp *= 10;
            }
            data_len += data->rxlen ;
            get_len = true;
        }
        if (get_len && data_len == data->rxlen)
        {
            data->type = SERIAL_GET_DATA;
            data->rxlen++;
            data->rxbuf[data->rxlen] = '\0';
            osMessageQueuePut(serial_ctl.msgque, data, 0, 0);
            is_data = false;
            get_len = false;
            data->rxlen = 0;
        }
        else
        {
            data->rxlen++;
        }
    }
    else
    {
        if ((data->rxbuf[data->rxlen] == '\n') || (data->rxlen==0 && data->rxbuf[0]=='>'))
        {
            data->type = SERIAL_GET_LINE;
            if (data->rxbuf[data->rxlen] != '\n')
            {
                data->rxlen++;
            }
            data->rxbuf[data->rxlen] = '\0';
            osMessageQueuePut(serial_ctl.msgque, data, 0, 0);
            data->rxlen = 0;
        }
        else
        {
            data->rxlen++;
            if (data->rxlen == MAX_BUFFER_SIZE)
            {
                data->type = SERIAL_GET_FULL;
                data->rxbuf[data->rxlen] = '\0';
                osMessageQueuePut(serial_ctl.msgque, data, 0, 0);
                data->rxlen = 0;
            }
        }
    }
    HAL_UART_Receive_IT(data->huart, data->rxbuf + data->rxlen, 1);
}

static void serial_data_handle(serial_data_t *data)
{
    if (data->huart == &huart6)
    {
        if (serial_ctl.uart6_rx_cb != NULL)
        {
            serial_ctl.uart6_rx_cb(data->rxbuf, data->rxlen);
        }
    }
}