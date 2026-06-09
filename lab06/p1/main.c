#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "inttypes.h"

#define botao_pin 5

volatile bool button_state = false;

static volatile TickType_t ultimo_clique = 0;

static void IRAM_ATTR gpio_isr_handler(void *arg){
    // Pega o tempo atual do FreeRTOS (em Ticks)
    TickType_t tempo_atual = xTaskGetTickCountFromISR();

    // Se passou mais de 200ms (convertidos para ticks) desde o último clique, valida!
    if (tempo_atual - ultimo_clique > pdMS_TO_TICKS(200)) {
        button_state = true;
        ultimo_clique = tempo_atual; // Atualiza a hora do último clique
    }
}

void app_main(void)
{

    gpio_reset_pin(botao_pin);
    gpio_set_direction(botao_pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(botao_pin, GPIO_PULLUP_ONLY);

    gpio_set_intr_type(botao_pin, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(botao_pin, gpio_isr_handler, NULL);
    gpio_intr_enable(botao_pin);

    while(1){
        if(button_state ==true){
            printf("Pressionado!! \n");
            button_state= false;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);

    }

}
