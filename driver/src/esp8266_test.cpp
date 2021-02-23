#include "main.h"
#include "esp8266_os.h"
#include "cmsis_os2.h"
#include <cstdio>

uint8_t buf[2048];

void esp8266_test()
{
    // wifi_cmd_reset();
    printf("esp8266 test> step1: send AT test\r\n");
    wifi_cmd_test();
    printf("esp8266 test> step2: enable echo\n");
    wifi_cmd_set_echo(true);
    printf("esp8266 test> step3: get gmr\r\n");
    wifi_cmd_get_gmr();
    printf("esp8266 test> step4: boost uart baud\r\n");
    wifi_cmd_uart_cur(115200 * 9);
    HAL_UART_Set_Baud(WIFI_PORT, 115200 * 9);
    printf("esp8266 test> step5: set station mode\r\n");
    wifi_cmd_cwmode_cur(WIFI_MODE_STA);
    printf("esp8266 test> step6: set list aps opt\r\n");
    wifi_cmd_cwlapopt(1, 2047);
    printf("esp8266 test> step7: list aps\r\n");
    wifi_cmd_cwlap();
    printf("esp8266 test> step8: join ap\r\n");
    wifi_cmd_cwjap_cur("Hotel M_1118", "seoul0900");
    printf("esp8266 test> step9: disable ipmux\r\n");
    wifi_cmd_set_ipmux(false);
    printf("esp8266 test> step10: check ipstatus\r\n");
    wifi_cmd_cipstatus();
    // wifi_cmd_tcp_connect("192.168.0.5", 6666);
    printf("esp8266 test> step11: connect udp\r\n");
    wifi_cmd_udp_connect("192.168.0.5", 5555, 7777);
    printf("esp8266 test> step12: udp send\r\n");
    int cnt = 25;
    while (cnt--)
    {
        wifi_cmd_cipsend(buf, 2048);
    }
    cnt = 25;
    osDelay(2000);
    while (cnt--)
    {
        wifi_cmd_cipsend(buf, 2048);
    }
    printf("esp8266 test> step13: check ipstatus\r\n");
    wifi_cmd_cipstatus();
    printf("esp8266 test> step14: disconnect\r\n");
    wifi_cmd_cipclose();
    printf("esp8266 test> step15: quit ap\r\n");
    wifi_cmd_cwqap();
    osDelay(500);
    printf("esp8266 test> step16: set baud back to 115200\r\n");
    wifi_cmd_uart_cur(115200);
    HAL_UART_Set_Baud(WIFI_PORT, 115200);
    printf("esp8266 test> step17: AT test\r\n");
    wifi_cmd_test();
}

void esp8266_socket_test()
{
    printf("esp8266 test> step1: wifi reset\r\n");
    wifi_cmd_reset();
    printf("esp8266 test> step2: boost uart baud\r\n");
    wifi_cmd_uart_cur(115200 * 9);
    HAL_UART_Set_Baud(WIFI_PORT, 115200 * 9);
    printf("esp8266 test> step3: set station mode\r\n");
    wifi_cmd_cwmode_cur(WIFI_MODE_STA);
    printf("esp8266 test> step4: join ap\r\n");
    wifi_cmd_cwjap_cur("Hotel M_1118", "seoul0900");
    printf("esp8266 test> step5: enable ipmux\r\n");
    wifi_cmd_set_ipmux(true);
    socket_t *udp = udp_socket_init(NULL);
    socket_t *udp2 = udp_socket_init(NULL);
    socket_open(udp, "192.168.0.5", 5555, 7777);
    socket_open(udp2, "192.168.0.5", 5277, 7788);
    socket_send(udp, (uint8_t*)"hello\r\nu", 8);
    socket_send(udp2, (uint8_t*)"2hello\r\nu", 9);
    socket_close(udp);
    socket_close(udp2);

    printf("esp8266 test> step10: check ipstatus\r\n");
    wifi_cmd_cipstatus();
    printf("esp8266 test> step15: set baud back to 115200\r\n");
    wifi_cmd_uart_cur(115200);
    HAL_UART_Set_Baud(WIFI_PORT, 115200);
}

void udp_receive_handler(uint8_t *buf, uint16_t len)
{
    printf("udp> len: %d\r\n", len);
    printf("udp> %s\r\n", buf);
}
void tcp_receive_handler(uint8_t *buf, uint16_t len)
{
    printf("tcp> len: %d\r\n", len);
    printf("tcp> %s\r\n", buf);
}
socket_t *udp_socket;
socket_t *tcp_socket;
void esp8266_udp_test(char cmd)
{
    if (cmd == 'W')
    {
        printf("esp8266 test> step1: wifi reset\r\n");
        wifi_cmd_reset();
        printf("esp8266 test> step2: boost uart baud\r\n");
        wifi_cmd_uart_cur(115200 * 9);
        HAL_UART_Set_Baud(WIFI_PORT, 115200 * 9);
        printf("esp8266 test> step3: set station mode\r\n");
        wifi_cmd_cwmode_cur(WIFI_MODE_STA);
        printf("esp8266 test> step4: join ap\r\n");
        wifi_cmd_cwjap_cur("Hotel M_1118", "seoul0900");
        printf("esp8266 test> step5: enable ipmux\r\n");
        wifi_cmd_set_ipmux(true);
    }
    else if (cmd == 'U')
    {
        udp_socket = udp_socket_init(udp_receive_handler);
        socket_open(udp_socket, "192.168.0.5", 5555, 7777);
    }
    else if (cmd == 'T')
    {
        tcp_socket = tcp_client_init(tcp_receive_handler);
        // socket_open(tcp_socket, "192.168.0.5", 6666);
        socket_open(tcp_socket, "www.baidu.com", 80);
    }
    else if (cmd == 'S')
    {
        socket_send(udp_socket, (uint8_t*)"test\r\n", 6);
        socket_send(tcp_socket, (uint8_t*)"GET ", 4);
    }
    else if (cmd == 'E')
    {
        socket_close(udp_socket);
        wifi_cmd_cwqap();
        osDelay(500);
        wifi_cmd_uart_cur(115200);
        HAL_UART_Set_Baud(WIFI_PORT, 115200);
    }
}