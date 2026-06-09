#include "wifi.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "inttypes.h"
#include "nvs_flash.h"
#include "esp_websocket_client.h"
#include "esp_task_wdt.h"

#define botao_pin 5
#define botao_pin1 18

volatile bool button_state = false;
volatile bool button_state1 = false;

static volatile TickType_t ultimo_clique = 0;
static volatile TickType_t ultimo_clique1 = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    TickType_t tempo_atual = xTaskGetTickCountFromISR();

    if (tempo_atual - ultimo_clique > pdMS_TO_TICKS(200)) {
        button_state = true;
        ultimo_clique = tempo_atual;
    }

    if (tempo_atual - ultimo_clique1 > pdMS_TO_TICKS(200)) {
        button_state1 = true;
        ultimo_clique1 = tempo_atual;
    }
}

void app_main(void)
{
    esp_task_wdt_deinit();
    // 1. Configuração do Botão
    gpio_reset_pin(botao_pin);
    gpio_set_direction(botao_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(botao_pin, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(botao_pin, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(botao_pin, gpio_isr_handler, NULL);
    gpio_intr_enable(botao_pin);

    gpio_reset_pin(botao_pin1);
    gpio_set_direction(botao_pin1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(botao_pin1, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(botao_pin1, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(botao_pin1, gpio_isr_handler, NULL);
    gpio_intr_enable(botao_pin1);

    // 2. Inicialização do Wi-Fi
    nvs_flash_init();
    wifi_connection();

    vTaskDelay(pdMS_TO_TICKS(5000));

    // 3. Configuração do WebSocket
    esp_websocket_client_config_t ws_config = {
        .uri = "ws://192.168.1.100:3000?player=p1",
    };

    esp_websocket_client_handle_t client = esp_websocket_client_init(&ws_config);
    esp_websocket_client_start(client);

    // Aguarda o handshake do WebSocket terminar
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 4. Loop Principal
    while (1) {
        if (button_state == true) {
            const char *dados = "{\"type\":\"forward\",\"pixels\":10}";

            esp_websocket_client_send_text(
                client,
                dados,
                strlen(dados),
                portMAX_DELAY
            );

            button_state = false;

            ESP_LOGI("WS", "Comando enviado: andar para frente");
        }

        if (button_state1 == true) {
            const char *dados = "{\"type\":\"left\",\"degrees\":15}";

            esp_websocket_client_send_text(
                client,
                dados,
                strlen(dados),
                portMAX_DELAY
            );

            button_state1 = false;

            ESP_LOGI("WS", "Comando enviado: andar para frente");
        }
    }
}