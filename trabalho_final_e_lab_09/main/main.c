#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

//locais
#include "wifi.h"
#include "joystick.h"
#include "botao.h"
#include "udp_cliente.h"

#define JOY_POLL_MS 33 
#define INPUT_EPSILON 0.001f

static const char *TAG = "APP_MAIN";

static int input_changed(float current, float previous)
{
    float diff = current - previous;
    if (diff < 0.0f) {
        diff = -diff;
    }
    return diff > INPUT_EPSILON;
}

void app_main(void)
{
    esp_task_wdt_deinit();
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }

    wifi_connection();
    ESP_LOGI(TAG, "Aguardando IP...");
    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi conectado! Configurando Hardware...");

    joystick_init();
    botao_chute_init();

    if (!udp_init_and_handshake()) {
        ESP_LOGE(TAG, "Falha na comunicação com o servidor UDP. Reiniciando...");
        esp_restart();
    }

    ESP_LOGI(TAG, "Sistema Modular pronto para jogar! Entrando no loop principal...");

    float last_jx = 0.0f;
    float last_jy = 0.0f;
    int last_kick = 0;

    while (1) {
        
        udp_receive_server_data();

        float jx, jy;
        joystick_read_normalized(&jx, &jy);
        int kick = botao_chute_is_pressed();

        int joystick_active = (jx != 0.0f || jy != 0.0f);
        int joystick_changed = input_changed(jx, last_jx) || input_changed(jy, last_jy);

        if (kick == 1 ||
            kick != last_kick ||
            joystick_active ||
            joystick_changed) {
            udp_send_player_input(jx, jy, kick);
            last_jx = jx;
            last_jy = jy;
            last_kick = kick;
        }

        vTaskDelay(pdMS_TO_TICKS(JOY_POLL_MS));
    }
}
