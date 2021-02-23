#ifndef __ESP8266_OS_H__
#define __ESP8266_OS_H__

#include "usart.h"
#include <stdint.h>

#define WIFI_MODE_STA 1
#define WIFI_MODE_AP 2
#define WIFI_MODE_AP_STA 3

#define WIFI_PORT (&huart6)

#define RSP_AT_ONCE         500
#define RSP_AWHILE          10000
#define RSP_LONG            20000

#define EVT_OK              0x00000001
#define EVT_READY           0x00000002
#define EVT_ERROR           0x00000004
#define EVT_SEND_READY      0x00000008
#define EVT_SEND_DONE       0x00000010
#define EVT_SEND_FAIL       0x00000020
#define EVT_RECEIVE         0x00000040
#define EVT_ALREAD_CONNECT  0x00000080
#define EVT_ALL             0x000000FF

#define SOCKET_TCP_CLIENT   1
#define SOCKET_TCP_SERVER   2
#define SOCKET_UDP          3

#define SUPPORT_LINKS       6

typedef uint32_t wifi_status_t;

typedef void (*net_revieve_cb_t)(uint8_t *buf, uint16_t len);

typedef struct {
    uint8_t             link_id;
    uint8_t             type;
    net_revieve_cb_t    receive_calllback;
} socket_t;

extern void esp8266_os_init();

/* AT Command */
extern wifi_status_t wifi_cmd_test();
extern wifi_status_t wifi_cmd_reset();
extern wifi_status_t wifi_cmd_get_gmr();
extern wifi_status_t wifi_cmd_set_echo(bool enable);
extern wifi_status_t wifi_cmd_restore();
extern wifi_status_t wifi_cmd_uart_cur(uint32_t baud);

extern wifi_status_t wifi_cmd_cwmode_cur(uint32_t mode);
extern wifi_status_t wifi_cmd_cwjap_cur(const char *ssid, const char *pwr);
extern wifi_status_t wifi_cmd_cwlapopt(uint16_t sort_enable, uint16_t sort_mask);
extern wifi_status_t wifi_cmd_cwlap();
extern wifi_status_t wifi_cmd_cwqap();

extern wifi_status_t wifi_cmd_set_ipmux(bool enable_mux);
extern wifi_status_t wifi_cmd_cipstatus();
extern wifi_status_t wifi_cmd_tcp_connect(const char *remote_ip, uint32_t remote_port, uint8_t link_id=0xFF);
extern wifi_status_t wifi_cmd_udp_connect(const char *remote_ip, uint32_t remote_port, uint32_t local_port, uint8_t link_id=0xFF);
extern wifi_status_t wifi_cmd_cipsend(uint8_t *buf, uint16_t len, uint8_t link_id=0xFF);
extern wifi_status_t wifi_cmd_cipclose(uint8_t link_id=0xFF);

/* tcp/ip apis */
extern socket_t *tcp_client_init(net_revieve_cb_t func);
extern socket_t *tcp_server_init(net_revieve_cb_t func);
extern socket_t *udp_socket_init(net_revieve_cb_t func);
extern bool socket_open(socket_t *socket, const char *remote_ip, uint32_t remote_port, uint32_t local_port=5277);
extern void socket_close(socket_t *socket);
extern void socket_send(socket_t *socket, uint8_t *buf, uint16_t len);

/* https */

#endif