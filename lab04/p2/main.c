#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#define PINO_ENTRADA 4

void app_main(void)
{
    // configura entrada
    esp_task_wdt_deinit();
    gpio_reset_pin(PINO_ENTRADA);
    gpio_set_direction(PINO_ENTRADA, GPIO_MODE_INPUT);

    //gpio_set_pull_mode(PINO_ENTRADA, GPIO_PULLDOWN_ONLY);

    int estado_anterior = 0;
    int estado_atual    = 0;
    int64_t tempo_anterior = 0;
    int64_t tempo_atual    = 0;
    int contador = 0;

    while (1) {
        int64_t agora = esp_timer_get_time();

        // lê frequência
        estado_atual = gpio_get_level(PINO_ENTRADA);

        if (estado_anterior == 0 && estado_atual == 1) {
            tempo_atual = agora;

            if (tempo_anterior != 0) {
                int64_t periodo    = tempo_atual - tempo_anterior;
                float   frequencia = 1000000.0f / periodo;
                contador++;
            if (contador >= 50) {  // imprime a cada 50 ciclos
                printf("Freq: %.2f Hz\n", frequencia);
                contador = 0;
            }
            }

            tempo_anterior = tempo_atual;
        }

        estado_anterior = estado_atual;
        
    }
}