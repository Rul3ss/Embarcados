#include "udp_cliente.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define UDP_SERVER_IP    "192.168.1.102"
#define UDP_SERVER_PORT  5555

static const char *TAG = "SYS_UDP";

static int s_udp_sock = -1;
static int s_player_id = -1;

static int json_get_int(const char *json, const char *key, int fallback)
{
    char pattern[32];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *p = strstr(json, pattern);
    if (!p) return fallback;
    p += strlen(pattern);
    while (*p == ' ') p++;
    return atoi(p);
}

bool udp_init_and_handshake(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_port        = htons(UDP_SERVER_PORT);

    s_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s_udp_sock < 0) return false;

    if (connect(s_udp_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        close(s_udp_sock);
        s_udp_sock = -1;
        return false;
    }

    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(s_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    const char *hello = "{\"cmd\":\"HELLO\"}";
    char rx_buffer[160];

    for (int attempt = 1; attempt <= 20; attempt++) {
        if (send(s_udp_sock, hello, strlen(hello), 0) < 0) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        int len = recv(s_udp_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len <= 0) continue;
        
        rx_buffer[len] = 0;

        if (strstr(rx_buffer, "FULL") != NULL) return false;
        
        if (strstr(rx_buffer, "WELCOME") != NULL) {
            s_player_id = json_get_int(rx_buffer, "pid", -1);
            if (s_player_id >= 0) {
                ESP_LOGI(TAG, "Handshake ok! PID = %d", s_player_id);
                
                fcntl(s_udp_sock, F_SETFL, O_NONBLOCK);
                return true;
            }
        }
    }
    return false;
}

void udp_send_player_input(float jx, float jy, int kick)
{
    if (s_udp_sock < 0 || s_player_id < 0) return;

    char tx_buf[96];
    snprintf(tx_buf, sizeof(tx_buf),
             "{\"cmd\":\"INPUT\",\"pid\":%d,\"jx\":%.3f,\"jy\":%.3f,\"kick\":%d}",
             s_player_id, jx, jy, kick);

    send(s_udp_sock, tx_buf, strlen(tx_buf), 0);
}

void udp_receive_server_data(void)
{
    if (s_udp_sock < 0) return;

    char rx_buf[512];
    int len = recv(s_udp_sock, rx_buf, sizeof(rx_buf) - 1, 0);
    
    if (len > 0) {
        rx_buf[len] = 0;
    }
}