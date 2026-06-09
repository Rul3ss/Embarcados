#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h" // Incluído para controlar o botão
#include <string.h>

#define BUTTON_PIN 4
#define UART_PORT UART_NUM_2

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

void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, 1024, 0, 0, NULL, 0);

    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, NULL);
    gpio_intr_enable(BUTTON_PIN);

    char letra_atual = 'A';
    char dado_recebido[8];

    printf("começou!\n");

    while(1) {

        if (button_state ==true) {
            
            uart_write_bytes(UART_PORT, &letra_atual, 1);
            printf("\n[TX] Enviado: %c\n", letra_atual);

            letra_atual++;
            if (letra_atual > 'Z') {
                letra_atual = 'A';
            }
            
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
        button_state= false;

        int len = uart_read_bytes(UART_PORT, dado_recebido, 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            dado_recebido[len] = '\0';
            printf("[RX] Recebido: %s\n", dado_recebido);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}