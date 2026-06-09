#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#define FREQ_PIN GPIO_NUM_4
#define PCNT_HIGH_LIMIT 100
#define PCNT_LOW_LIMIT  -100

void app_main() {
  esp_task_wdt_deinit();

  pcnt_unit_config_t unit_config = {
      .high_limit = PCNT_HIGH_LIMIT,
      .low_limit = PCNT_LOW_LIMIT,
      .intr_priority = 0,
  };
  pcnt_unit_handle_t pcnt_unit = NULL;
  ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

  pcnt_chan_config_t chan_config = {
    .edge_gpio_num  = FREQ_PIN,
    .level_gpio_num = -1,
  };

  pcnt_channel_handle_t pcnt_chan = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_config, &pcnt_chan));

  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
      pcnt_chan,
      PCNT_CHANNEL_EDGE_ACTION_INCREASE, // Vai incrementar na borda de subida
      PCNT_CHANNEL_EDGE_ACTION_HOLD
  ));

  ESP_ERROR_CHECK(pcnt_channel_set_level_action(
    pcnt_chan,
    PCNT_CHANNEL_LEVEL_ACTION_KEEP,  
    PCNT_CHANNEL_LEVEL_ACTION_KEEP
  ));

  ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
  ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

  // Loop de leitura
  int qtdPulso = 0;
  int lim = 99;
  uint64_t tempoAtual = esp_timer_get_time();
  float periodo;
  float freq;
  float toMin = 1000000.0f;


  while (true) {
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &qtdPulso));
    if(qtdPulso >= lim){
      periodo = (esp_timer_get_time() - tempoAtual);
      freq= (100*toMin)/periodo;
    printf("QTD: %d, frequência: %f \n", qtdPulso, freq);
    tempoAtual = esp_timer_get_time();

    }
  }

}