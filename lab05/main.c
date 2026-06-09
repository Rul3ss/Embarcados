#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"

void app_main() {
    //adc config
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);  //pino 34
    
    int adc_raw;
    //float voltage;
    
    while (true) {
        adc_raw = adc1_get_raw(ADC1_CHANNEL_6); 
        // valor digital em decimal que representa a tensão analógica de entrada
        
        //voltage = (adc_raw / 4095.0) * 3.3;
        
        //printf("ADC Raw: %d | Tensão: %.3f V\n", adc_raw, voltage);

        printf("ADC Raw: %d\n", adc_raw);
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}