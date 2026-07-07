#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

// Nossas Camadas Inferiores
#include "wifi.h"
#include "joystick.h"
#include "botao.h"
#include "udp_cliente.h"

#define JOY_POLL_MS 33 

static const char *TAG = "APP_MAIN";

void app_main(void)
{
    esp_task_wdt_deinit();
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }

    // Inicializa Sistema (Wi-Fi)
    wifi_connection();
    ESP_LOGI(TAG, "Aguardando IP...");
    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi conectado! Configurando Hardware...");

    // Inicializa Hardware
    joystick_init();
    botao_chute_init();

    // Inicializa Sistema (UDP)
    if (!udp_init_and_handshake()) {
        ESP_LOGE(TAG, "Falha na comunicação com o servidor UDP. Reiniciando...");
        esp_restart();
    }

    ESP_LOGI(TAG, "Sistema Modular pronto para jogar! Entrando no loop principal...");

    // Loop Principal da Aplicação
    while (1) {
        
        // 1. Receber dados do servidor (não-bloqueante)
        udp_receive_server_data();

        // 2. Ler Hardware
        float jx, jy;
        joystick_read_normalized(&jx, &jy);
        int kick = botao_chute_is_pressed();

        // 3. Enviar Comando para o Sistema
        udp_send_player_input(jx, jy, kick);

        // 4. Controlar a taxa de atualização (~30Hz)
        vTaskDelay(pdMS_TO_TICKS(JOY_POLL_MS));
    }
}