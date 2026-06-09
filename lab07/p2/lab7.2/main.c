#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/adc.h"
#include "esp_task_wdt.h"

int isr_count = 0;
static volatile int adc_raw;

static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_ctx)
{   isr_count++;
    adc_raw = adc1_get_raw(ADC1_CHANNEL_6);
    return false;  // não solicita troca de contexto
}

void timer_adc_config () {
    gptimer_handle_t gptimer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz -> 1 tick = 1 us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1000000,     // 10.000 us = 10 ms
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));



    // Configuração da unidade ADC
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Configura atenuação do canal
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // GPIO34

}



void app_main() {
  esp_task_wdt_deinit();
  timer_adc_config();

  int lastCount = 0;

  while(1){
    if(isr_count != lastCount){
      printf("V= %d\n", adc_raw);
      lastCount = isr_count;
    }
  }
  
}
