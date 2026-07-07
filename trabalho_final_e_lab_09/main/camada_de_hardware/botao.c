#include "botao.h"

// Inclusão exclusiva da Camada de Hardware
#include "driver/gpio.h"

/* ── Definição de Hardware do Botão ───────────────────────────── */
#define BTN_KICK_GPIO    ((gpio_num_t)27)

/* ── Implementação da API Pública ────────────────────────────── */

void botao_chute_init(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_KICK_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    
    // Configura o pino de acordo com a estrutura acima
    gpio_config(&io_conf);
}

int botao_chute_is_pressed(void)
{
    // Como o botão usa resistor de pull-up (mantém em nível alto),
    // o nível lógico baixo (0) significa que ele foi pressionado e fechou contato com o GND.
    return (gpio_get_level(BTN_KICK_GPIO) == 0) ? 1 : 0;
}