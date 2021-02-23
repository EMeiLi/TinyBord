#include "main.h"
#include "gpio.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "oled_u8g2.h"
#include "serial.h"
#include "esp8266_os.h"
#include "esp8266_test.h"
#include "cmsis_os2.h"
#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

const osThreadAttr_t thread1_attr = {
    .stack_size = 4096
};

__NO_RETURN static void app_main(void *argument)
{
    (void)argument;
    MX_GPIO_Init();
    // MX_I2C1_Init();
    MX_SPI2_Init();
    MX_USART1_UART_Init();
    MX_USART6_UART_Init();
    osDelay(20);
    u8g2_t u8g2;
    u8g2Init(&u8g2);
    u8g2_SetFont(&u8g2, u8g_font_10x20r);
    u8g2_DrawStr(&u8g2, 0, 25, "Happy niu");
    u8g2_DrawStr(&u8g2, 0, 50, "  Year [2021]");
    u8g2_SendBuffer(&u8g2);
    string str;
    str.resize(128);
    serial_receive_init();
    esp8266_os_init();
    while (1)
    {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        cout << "console> please input a line: ";
        getline(cin, str);
        cout << "console get> " << str << endl;
        // esp8266_test();
        // esp8266_socket_test();
        esp8266_udp_test(str.at(0));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    osKernelInitialize();
    osThreadNew(app_main, NULL, &thread1_attr);
    osKernelStart();
    for (;;)
    {
    }
    return 0;
}