#include "botao.h"

#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define BTN_KICK_GPIO    ((gpio_num_t)27)

static volatile int s_btn_kick_pressed = 0;

static void IRAM_ATTR botao_chute_isr_handler(void *arg)
{
    (void)arg;
    s_btn_kick_pressed = 1;
}

void botao_chute_init(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_KICK_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    s_btn_kick_pressed = 0;

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_KICK_GPIO, botao_chute_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_intr_enable(BTN_KICK_GPIO));
}

int botao_chute_is_pressed(void)
{
    int pressed = s_btn_kick_pressed;
    s_btn_kick_pressed = 0;
    return pressed;
}
