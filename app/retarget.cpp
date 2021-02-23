#include "main.h"
#include "usart.h"
#include <stdint.h>
#include <cstdio>

extern "C"
{
    int __io_putchar(int ch)
    {
        while ((huart1.Instance->SR & 1 << 7) == 0)
            ;
        huart1.Instance->DR = ch;
        return ch;
    }

    int __io_getchar(void)
    {
        int ch;
        HAL_UART_Receive(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
        if (ch == '\r')
        {
            while ((huart1.Instance->SR & 1 << 7) == 0)
                ;
            huart1.Instance->DR = ch;
            ch = '\n';
        }
        while ((huart1.Instance->SR & 1 << 7) == 0)
            ;
        huart1.Instance->DR = ch;
        return ch; //__io_putchar(ch);
    }
}