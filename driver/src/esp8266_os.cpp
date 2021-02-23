#include "serial.h"
#include "esp8266_os.h"
#include "cmsis_os2.h"
#include <cstring>
#include <cstdio>

/* String list definition */
typedef const struct
{
    const char *str;
} STRING_LIST_t;

/* Command codes */
typedef enum
{
    CMD_NONE = 0,
    CMD_RST,
    CMD_GMR,
    CMD_RESTORE,
    CMD_UART_CUR,

    CMD_CWMODE_CUR,
    CMD_CWJAP_CUR,
    CMD_CWLAPOPT,
    CMD_CWLAP,
    CMD_CWQAP,

    CMD_CIPMUX,
    CMD_CIPSTATUS,
    CMD_CIPSTART,
    CMD_CIPSEND,
    CMD_CIPCLOSE,

    CMD_IPD,
    CMD_ECHO = 0xFD,   /* Command Echo                 */
    CMD_TEST = 0xFE,   /* AT startup (empty command)   */
    CMD_UNKNOWN = 0xFF /* Unknown or unhandled command */
} CommandCode_t;

/* Command list (see also CommandCode_t) */
static STRING_LIST_t List_PlusResp[] = {
    {""},
    {"RST"},
    {"GMR"},
    {"RESTORE"},
    {"UART_CUR"},

    {"CWMODE_CUR"},
    {"CWJAP_CUR"},
    {"CWLAPOPT"},
    {"CWLAP"},
    {"CWQAP"},

    {"CIPMUX"},
    {"CIPSTATUS"},
    {"CIPSTART"},
    {"CIPSEND"},
    {"CIPCLOSE"},

    {"IPD"},
};

/* Rsponse codes */
typedef enum
{
    RSP_NONE = 0x00000000,
    RSP_OK,
    RSP_ERROR,
    RSP_FAIL,
    RSP_SEND,
    RSP_SEND_OK,
    RSP_SEND_FAIL,
    RSP_BUSY_P,
    RSP_BUSY_S,
    RSP_ALREADY_CONNECT,
    RSP_WIFI_CONNECT,
    RSP_WIFI_GOT_IP,
    RSP_WIFI_DISCONNECT,
    RSP_AT,
    RSP_READY,
    RSP_ERROR_CODE,
    RSP_RECEIVE,
} ResponseCode_t;

/* Generic responses (see AT_RESP_x definitions) */
static STRING_LIST_t List_ASCIIResp[] = {
    {""},
    {"OK"},
    {"ERROR"},
    {"FAIL"},
    {">"},
    {"SEND OK"},
    {"SEND FAIL"},
    {"busy p..."},
    {"busy s..."},
    {"ALREADY CONNECTED"},
    /* Generic responses redirected to AT_Notify function */
    {"WIFI CONNECTED"},
    {"WIFI GOT IP"},
    {"WIFI DISCONNECT"},
    /* Special responses */
    {"AT"},
    {"ready"},
    {"ERR CODE"},
    {"+IPD"},
};

typedef struct
{
    osEventFlagsId_t    evt;
    osEventFlagsId_t    send_evt;
    CommandCode_t       cur_cmd;
    bool                is_ipmux;
    uint8_t             link_mask;
    uint8_t             link_id;
    socket_t            socket[SUPPORT_LINKS];
    uint8_t             link_num;
    uint8_t             cmdbuf[256];
} wifi_ctl_t;

static wifi_ctl_t wifi_ctl;

static void     wifi_uart_rx_cb(uint8_t *buf, uint16_t len);
static void     wifi_send_cmd(CommandCode_t cmd, char *str1, char *str2);
static uint32_t wifi_wait_evt(uint32_t evt, uint32_t timeout);
static void     wifi_set_evt(uint32_t evt);
static uint8_t  wifi_get_link_id();
static socket_t *wifi_alloc_socket();
static void     wifi_receive_handle(uint8_t *buf, uint16_t len);

static bool     wifi_prase_begin_with(uint8_t *buf, uint16_t len, ResponseCode_t keyWord);

void esp8266_os_init()
{
    for (uint8_t i = 0; i < SUPPORT_LINKS; ++i)
    {
        wifi_ctl.socket[i].link_id = i;
    }
    wifi_ctl.link_id = 0;
    wifi_ctl.link_num = 0;
    wifi_ctl.link_mask = 0xff;
    wifi_ctl.is_ipmux = false;
    wifi_ctl.evt = osEventFlagsNew(NULL);
    wifi_ctl.send_evt = osEventFlagsNew(NULL);
    serial_register(WIFI_PORT, wifi_uart_rx_cb);
}

static void wifi_uart_rx_cb(uint8_t *buf, uint16_t len)
{
    printf("wifi> %s\r\n", buf);
    if (wifi_prase_begin_with(buf, len, RSP_SEND))
    {
        wifi_set_evt(EVT_SEND_READY);
    }
    else if (wifi_prase_begin_with(buf, len, RSP_OK))
    {
        wifi_set_evt(EVT_OK);
    }
    else if (wifi_prase_begin_with(buf, len, RSP_READY))
    {
        wifi_set_evt(EVT_READY);
    }
    else if (wifi_prase_begin_with(buf, len, RSP_SEND_OK))
    {
        wifi_set_evt(EVT_SEND_DONE);
    }
    else if (wifi_prase_begin_with(buf, len, RSP_FAIL))
    {
        wifi_set_evt(EVT_ERROR);
    }
    else if (wifi_prase_begin_with(buf, len, RSP_RECEIVE))
    {
        wifi_receive_handle(buf+5, len-5);
    }
}

static bool wifi_prase_begin_with(uint8_t *buf, uint16_t len, ResponseCode_t keyWord)
{
    const char *word = List_ASCIIResp[keyWord].str;
    uint8_t wordLen = strlen(word);
    if (len >= wordLen && memcmp(buf, word, wordLen) == 0)
    {
        return true;
    }
    return false;
}

static void wifi_send_cmd(CommandCode_t cmd, const char *str1 = NULL, const char *str2 = NULL)
{
    if (cmd == CMD_NONE)
    {
        sprintf((char *)wifi_ctl.cmdbuf, "AT");
    }
    else if (str1 == NULL)
    {
        sprintf((char *)wifi_ctl.cmdbuf, "AT+%s", List_PlusResp[cmd]);
    }
    else if (str2 == NULL)
    {
        sprintf((char *)wifi_ctl.cmdbuf, "AT+%s=%s", List_PlusResp[cmd], str1);
    }
    else
    {
        sprintf((char *)wifi_ctl.cmdbuf, "AT+%s=%s,%s", List_PlusResp[cmd], str1, str2);
    }

    wifi_ctl.cur_cmd = cmd;
    serial_send_str_nl(WIFI_PORT, wifi_ctl.cmdbuf);
}

static uint32_t wifi_wait_evt(uint32_t evt, uint32_t timeout)
{
    uint32_t flag;
    do
    {
        flag = osEventFlagsWait(wifi_ctl.evt, EVT_ALL, osFlagsWaitAny, timeout);
        // printf("wait flag: 0x%x, exp: 0x%x\r\n", flag, evt);
    } while ((flag != (evt&flag)) && ((flag&0x80000000) == 0));

    return flag;
}

static void wifi_set_evt(uint32_t evt)
{
    osEventFlagsSet(wifi_ctl.evt, evt);
}

static uint8_t wifi_get_link_id()
{
    uint8_t f = 0x01;
    for (uint8_t i = 0; i < SUPPORT_LINKS; ++i)
    {
        if (wifi_ctl.link_mask & (f<<i))
        {
            wifi_ctl.link_mask &= ~(f<<i);
            return i;
        }
    }
    return 0xFF;
}

static socket_t *wifi_alloc_socket()
{
    if (wifi_ctl.is_ipmux)
    {
        uint8_t id = wifi_get_link_id();
        printf("wifi> get id=%d\r\n", id);
        if (id != 0xFF)
        {
            wifi_ctl.link_num ++;
            return &wifi_ctl.socket[id];
        }
    }
    else
    {
        if (wifi_ctl.link_num == 0)
        {
            return &wifi_ctl.socket[0];
        }
    }
    return NULL;
}

static void wifi_receive_handle(uint8_t *buf, uint16_t len)
{
    uint8_t link_id = buf[0] - '0';
    if (link_id < SUPPORT_LINKS)
    {
        for (uint8_t i = 0; i < len; i++)
        {
            if (buf[i] == ':')
            {
                i++;
                if (wifi_ctl.socket[link_id].receive_calllback != NULL)
                {
                    wifi_ctl.socket[link_id].receive_calllback(buf+i, len-i);
                    return ;
                }
                printf("wifi> socket rx callback is NULL.\r\n");
            }
        }
        printf("wifi> link_id %d : not find.\r\n", link_id);
    }
    else
    {
        printf("wifi> link_id %d is valid.\r\n", link_id);
    }
}

wifi_status_t wifi_cmd_test()
{
    wifi_send_cmd(CMD_NONE);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_reset()
{
    wifi_send_cmd(CMD_RST);
    return wifi_wait_evt(EVT_READY, RSP_AWHILE);
}

wifi_status_t wifi_cmd_get_gmr()
{
    wifi_send_cmd(CMD_GMR);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_set_echo(bool enable)
{
    const char *pstr = enable? "ATE1": "ATE0";
    serial_send_str_nl(WIFI_PORT, (uint8_t *)pstr);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_restore()
{
    wifi_send_cmd(CMD_RESTORE);
    return wifi_wait_evt(EVT_READY, RSP_AWHILE);
}

wifi_status_t wifi_cmd_uart_cur(uint32_t baud)
{
    char str[20];
    sprintf(str, "%u,8,1,0,0", baud);
    wifi_send_cmd(CMD_UART_CUR, str);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_cwmode_cur(uint32_t mode)
{
    char str[20];
    sprintf(str, "%u", mode);
    wifi_send_cmd(CMD_CWMODE_CUR, str);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_cwjap_cur(const char *ssid, const char *pwr)
{
    char str[40];
    sprintf(str, "\"%s\",\"%s\"", ssid, pwr);
    wifi_send_cmd(CMD_CWJAP_CUR, str);
    return wifi_wait_evt(EVT_OK, RSP_LONG);
}

wifi_status_t wifi_cmd_cwlapopt(uint16_t sort_enable, uint16_t sort_mask)
{
    char str[10];
    sprintf(str, "%d,%d", sort_enable, sort_mask);
    wifi_send_cmd(CMD_CWLAPOPT, str);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_cwlap()
{
    wifi_send_cmd(CMD_CWLAP);
    return wifi_wait_evt(EVT_OK, RSP_AWHILE);
}

wifi_status_t wifi_cmd_cwqap()
{
    wifi_send_cmd(CMD_CWQAP);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

/* tcp/ip*/

wifi_status_t wifi_cmd_set_ipmux(bool enable_mux)
{
    wifi_ctl.is_ipmux = enable_mux;
    wifi_send_cmd(CMD_CIPMUX, enable_mux? "1": "0");
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_cipstatus()
{
    wifi_send_cmd(CMD_CIPSTATUS);
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

wifi_status_t wifi_cmd_tcp_connect(const char *remote_ip, uint32_t remote_port, uint8_t link_id)
{
    char str[30];
    if (wifi_ctl.is_ipmux)
    {
        sprintf(str, "%d,\"TCP\",\"%s\",%u", link_id, remote_ip, remote_port);
    }
    else
    {
        sprintf(str, "\"TCP\",\"%s\",%u", remote_ip, remote_port);
    }
    wifi_send_cmd(CMD_CIPSTART, str);
    return wifi_wait_evt(EVT_OK, RSP_AWHILE);
}

wifi_status_t wifi_cmd_udp_connect(const char *remote_ip, uint32_t remote_port, uint32_t local_port, uint8_t link_id)
{
    char str[30];
    if (wifi_ctl.is_ipmux)
    {
        sprintf(str, "%d,\"UDP\",\"%s\",%u,%u", link_id, remote_ip, remote_port,local_port);
    }
    else
    {
        sprintf(str, "\"UDP\",\"%s\",%u,%u", remote_ip, remote_port,local_port);
    }
    wifi_send_cmd(CMD_CIPSTART, str);
    return wifi_wait_evt(EVT_OK, RSP_AWHILE);
}

wifi_status_t wifi_cmd_cipsend(uint8_t *buf, uint16_t len, uint8_t link_id)
{
    char str[10];
    if (wifi_ctl.is_ipmux)
    {
        if (link_id == 0xFF)
        {
            printf("%s: para error.\r\n", __func__);
            return 0xFFFFFFFCU;
        }
        sprintf(str, "%d,%d", link_id, len);
    }
    else
    {
        sprintf(str, "%d", len);
    }
    wifi_send_cmd(CMD_CIPSEND, str);
    wifi_wait_evt(EVT_SEND_READY, RSP_AT_ONCE);
    serial_send(WIFI_PORT, buf, len);
    return wifi_wait_evt(EVT_SEND_DONE, RSP_AWHILE);
}

wifi_status_t wifi_cmd_cipclose(uint8_t link_id)
{
    char str[3];
    if (wifi_ctl.is_ipmux)
    {
        if (link_id == 0xFF)
        {
            printf("%s: para error.\r\n", __func__);
            return 0xFFFFFFFCU;
        }
        sprintf(str, "%d", link_id);
        wifi_send_cmd(CMD_CIPCLOSE, str);
    }
    else
    {
        wifi_send_cmd(CMD_CIPCLOSE);
    }
    
    return wifi_wait_evt(EVT_OK, RSP_AT_ONCE);
}

socket_t *tcp_client_init(net_revieve_cb_t func)
{
    socket_t *ps = wifi_alloc_socket();
    if (ps != NULL)
    {
        ps->type     = SOCKET_TCP_CLIENT;
        ps->receive_calllback = func;
    }
    return ps;
}

socket_t *tcp_server_init(net_revieve_cb_t func)
{
    socket_t *ps = wifi_alloc_socket();
    if (ps != NULL)
    {
        ps->type     = SOCKET_TCP_SERVER;
        ps->receive_calllback = func;
    }
    return ps;
}

socket_t *udp_socket_init(net_revieve_cb_t func)
{
    socket_t *ps = wifi_alloc_socket();
    if (ps != NULL)
    {
        printf(">>> get socket id=%d\r\n", ps->link_id);
        ps->type     = SOCKET_UDP;
        ps->receive_calllback = func;
    }
    return ps;
}

bool socket_open(socket_t *socket, const char *remote_ip, uint32_t remote_port, uint32_t local_port)
{
    switch (socket->type)
    {
    case SOCKET_TCP_CLIENT:
        wifi_cmd_tcp_connect(remote_ip, remote_port, socket->link_id);
        /* code */
        break;
    case SOCKET_TCP_SERVER:
        /* code */
        break;
    case SOCKET_UDP:
        /* code */
        wifi_cmd_udp_connect(remote_ip, remote_port, local_port, socket->link_id);
        break;
    default:
        break;
    }
    return true;
}

void socket_close(socket_t *socket)
{
    wifi_cmd_cipclose(socket->link_id);
}

void socket_send(socket_t *socket, uint8_t *buf, uint16_t len)
{
    wifi_cmd_cipsend(buf, len, socket->link_id);
}