#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <rom/ets_sys.h>
#include "esp_task_wdt.h"

#define PINO_SAIDA 2

#define TEMPO_INICIAL_US 00.5
#define TEMPO_MIN_US     10000   
#define PASSO_US         10000   
 

void app_main(void)
{
    esp_task_wdt_deinit();
    gpio_reset_pin(PINO_SAIDA);
    gpio_set_direction(PINO_SAIDA, GPIO_MODE_OUTPUT);

    int tempo = TEMPO_INICIAL_US;
    int tick = 0;

    while (1) {
        gpio_set_level(PINO_SAIDA, 1);
        ets_delay_us(tempo);
        gpio_set_level(PINO_SAIDA, 0);
        ets_delay_us(tempo);
        //tick++;

        if (tick >= 100) {
            tick = 0;
            tempo -= PASSO_US;

            if (tempo < TEMPO_MIN_US) {
                tempo = TEMPO_INICIAL_US;
                printf("RESETANDO\n");
            }

            printf("Novo tempo: %d us\n", tempo);
        }
    }
}
