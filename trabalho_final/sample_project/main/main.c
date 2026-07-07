#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "esp_task_wdt.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* ── Configurações ─────────────────────────────────────────── */
#define WIFI_SSID "203"
#define WIFI_PASSWORD "pacoquita"

#define UDP_SERVER_IP    "192.168.1.102"   /* ← IP do PC */
#define UDP_SERVER_PORT  5555

int retry_num = 0;

/* ── Joystick (analógico) ─────────────────────────────────────── */
#define JOY_X_ADC_CHANNEL   ADC_CHANNEL_6   /* GPIO34 */
#define JOY_Y_ADC_CHANNEL   ADC_CHANNEL_7   /* GPIO35 */
#define JOY_ADC_UNIT        ADC_UNIT_1

#define JOY_CALIB_SAMPLES   64     /* amostras para calibrar o centro no boot */
#define JOY_DEADZONE_NORM   0.05f /* zona morta pequena, já normalizada (-1..1) */
#define JOY_POLL_MS         33     /* ~30 Hz, igual ao TICK_RATE do servidor */

/* ── Botão de chute ───────────────────────────────────────────── */
#define BTN_KICK_GPIO    ((gpio_num_t)27)

/* ── Logging ─────────────────────────────────────────────────── */
static const char *TAG = "LAB9";

/* ── Event group bits ────────────────────────────────────────── */
#define WIFI_CONNECTED_BIT  BIT0
#define PLAYER_READY_BIT    BIT1
static EventGroupHandle_t s_event_group;

/* ── Socket UDP ──────────────────────────────────────────────── */
static int s_udp_sock = -1;
static int s_player_id = -1;

/* ── ADC ─────────────────────────────────────────────────────── */
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static float s_center_x = 2048.0f;
static float s_center_y = 2048.0f;

/* ══════════════════════════════════════════════════════════════
   WiFi
   ══════════════════════════════════════════════════════════════ */
static void wifi_event_handler(void *arg,
                                esp_event_base_t base,
                                int32_t event_id,
                                void *data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        printf("WIFI CONNECTING....\n");
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi CONNECTED\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi lost connection\n");
        if (retry_num < 5) { esp_wifi_connect(); retry_num++; printf("Retrying to Connect...\n"); }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("Wifi got IP...\n\n");
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_initiation));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                wifi_event_handler, NULL));

    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = { .capable = true, .required = false },
        }
    };
    strcpy((char *)wifi_configuration.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_configuration.sta.password, WIFI_PASSWORD);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configuration));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "wifi_init finalizado. SSID:%s", WIFI_SSID);
}

/* ══════════════════════════════════════════════════════════════
   Parser JSON minimalista (só o que precisamos: campos inteiros)
   Servidor manda ex: {"cmd": "WELCOME", "pid": 1, "team": "A", ...}
   ══════════════════════════════════════════════════════════════ */
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

/* ══════════════════════════════════════════════════════════════
   UDP: cria socket, "conecta" (fixa destino) e faz o handshake
   HELLO -> WELCOME antes de liberar as outras tasks.
   ══════════════════════════════════════════════════════════════ */
static bool udp_init(void)
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);
    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_port        = htons(UDP_SERVER_PORT);

    s_udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s_udp_sock < 0) {
        ESP_LOGE(TAG, "Falha ao criar socket UDP: errno %d", errno);
        return false;
    }

    int err = connect(s_udp_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Falha ao 'conectar' socket UDP: errno %d", errno);
        close(s_udp_sock);
        s_udp_sock = -1;
        return false;
    }

    /* Timeout curto para o loop de handshake poder reenviar HELLO */
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(s_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ESP_LOGI(TAG, "Socket UDP criado → destino %s:%d", UDP_SERVER_IP, UDP_SERVER_PORT);
    return true;
}

/* Faz o handshake HELLO/WELCOME, com retentativas (pacote UDP pode se perder). */
static bool udp_handshake(void)
{
    const char *hello = "{\"cmd\":\"HELLO\"}";
    char rx_buffer[160];

    for (int attempt = 1; attempt <= 20; attempt++) {
        int sent = send(s_udp_sock, hello, strlen(hello), 0);
        if (sent < 0) {
            ESP_LOGW(TAG, "Falha ao enviar HELLO (tentativa %d): errno %d", attempt, errno);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        ESP_LOGI(TAG, "HELLO enviado (tentativa %d), aguardando WELCOME...", attempt);

        int len = recv(s_udp_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len <= 0) {
            /* timeout — tenta de novo */
            continue;
        }
        rx_buffer[len] = 0;
        ESP_LOGI(TAG, "RX handshake [%d B]: %s", len, rx_buffer);

        if (strstr(rx_buffer, "FULL") != NULL) {
            ESP_LOGE(TAG, "Servidor cheio, não há vagas.");
            return false;
        }
        if (strstr(rx_buffer, "WELCOME") != NULL) {
            s_player_id = json_get_int(rx_buffer, "pid", -1);
            if (s_player_id < 0) {
                ESP_LOGW(TAG, "WELCOME sem pid válido, tentando de novo...");
                continue;
            }
            ESP_LOGI(TAG, "Conectado! player_id=%d", s_player_id);
            return true;
        }
        /* mensagem desconhecida — ignora e tenta de novo */
    }

    ESP_LOGE(TAG, "Handshake falhou após várias tentativas.");
    return false;
}

/* Task: escuta mensagens do servidor (STATE, GOAL, etc.) depois do handshake. */
static void udp_recv_task(void *pv_param)
{
    xEventGroupWaitBits(s_event_group, PLAYER_READY_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Task de recepção UDP iniciada.");

    /* timeout maior aqui, já que o servidor manda STATE ~30x/s */
    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(s_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char rx_buffer[512];
    for (;;) {
        int len = recv(s_udp_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            continue; /* timeout, normal em UDP */
        }
        rx_buffer[len] = 0;
        /* Descomente para depurar o conteúdo do STATE:
        ESP_LOGI(TAG, "RX [%d B]: %s", len, rx_buffer); */
    }
}

/* ══════════════════════════════════════════════════════════════
   ADC / Joystick
   ══════════════════════════════════════════════════════════════ */
static void adc_joystick_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = JOY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, JOY_X_ADC_CHANNEL, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, JOY_Y_ADC_CHANNEL, &chan_config));

    ESP_LOGI(TAG, "Joystick ADC inicializado (X=GPIO34, Y=GPIO35)");

    /* Calibração do centro: assume que o stick está solto/parado no boot
     * e tira a média de várias leituras. Corrige o desvio de fábrica do
     * potenciômetro (raramente cai exatamente em 2048), que era a causa
     * do "movimento fantasma" sem o stick ser tocado. */
    int64_t sum_x = 0, sum_y = 0;
    for (int i = 0; i < JOY_CALIB_SAMPLES; i++) {
        int raw_x = 0, raw_y = 0;
        adc_oneshot_read(s_adc1_handle, JOY_X_ADC_CHANNEL, &raw_x);
        adc_oneshot_read(s_adc1_handle, JOY_Y_ADC_CHANNEL, &raw_y);
        sum_x += raw_x;
        sum_y += raw_y;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    s_center_x = (float)sum_x / JOY_CALIB_SAMPLES;
    s_center_y = (float)sum_y / JOY_CALIB_SAMPLES;

    ESP_LOGI(TAG, "Joystick calibrado: center_x=%.1f center_y=%.1f",
             s_center_x, s_center_y);
}

static float normalize_axis(int raw, float center)
{
    float v = (raw - center) / 2048.0f;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    if (v > -JOY_DEADZONE_NORM && v < JOY_DEADZONE_NORM) v = 0.0f;
    return v;
}

/* ══════════════════════════════════════════════════════════════
   Botão de chute — leitura simples por nível (o servidor detecta
   a borda de subida sozinho, comparando com o kick do pacote
   anterior). Pino com pull-up: pressionado = nível baixo.
   ══════════════════════════════════════════════════════════════ */
static void gpio_init_kick_button(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_KICK_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_LOGI(TAG, "Botão de chute configurado (GPIO %d)", (int)BTN_KICK_GPIO);
}

static inline int kick_is_pressed(void)
{
    return (gpio_get_level(BTN_KICK_GPIO) == 0) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
   Task: lê joystick + botão periodicamente e envia INPUT via UDP
   ══════════════════════════════════════════════════════════════ */
static void input_task(void *pv_param)
{
    xEventGroupWaitBits(s_event_group, PLAYER_READY_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Task de input iniciada (pid=%d).", s_player_id);

    char buf[96];

    for (;;) {
        int raw_x = 0, raw_y = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc1_handle, JOY_X_ADC_CHANNEL, &raw_x));
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc1_handle, JOY_Y_ADC_CHANNEL, &raw_y));

        float jx = normalize_axis(raw_x, s_center_x);
        float jy = normalize_axis(raw_y, s_center_y);
        int kick = kick_is_pressed();

        snprintf(buf, sizeof(buf),
                 "{\"cmd\":\"INPUT\",\"pid\":%d,\"jx\":%.3f,\"jy\":%.3f,\"kick\":%d}",
                 s_player_id, jx, jy, kick);

        int sent = send(s_udp_sock, buf, strlen(buf), 0);
        if (sent < 0) {
            ESP_LOGW(TAG, "Falha ao enviar INPUT: errno %d", errno);
        }

        vTaskDelay(pdMS_TO_TICKS(JOY_POLL_MS));
    }
}

/* ══════════════════════════════════════════════════════════════
   app_main
   ══════════════════════════════════════════════════════════════ */
void app_main(void)
{
    esp_task_wdt_deinit();
    s_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init();

    ESP_LOGI(TAG, "Aguardando conexão WiFi...");
    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi OK — iniciando UDP");

    adc_joystick_init();
    gpio_init_kick_button();

    if (!udp_init()) {
        ESP_LOGE(TAG, "Não foi possível criar o socket UDP. Abortando.");
        return;
    }

    if (!udp_handshake()) {
        ESP_LOGE(TAG, "Handshake com o servidor falhou. Abortando.");
        return;
    }

    xEventGroupSetBits(s_event_group, PLAYER_READY_BIT);

    xTaskCreate(input_task,    "input",    4096, NULL, 5, NULL);
    xTaskCreate(udp_recv_task, "udp_recv", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "Sistema pronto. Mova o joystick e pressione o botão para chutar.");
}