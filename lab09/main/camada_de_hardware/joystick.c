#include "joystick.h"

// Inclusões específicas de Hardware e Sistema Operacional
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ── Definições de Hardware do Joystick ──────────────────────── */
#define JOY_X_ADC_CHANNEL   ADC_CHANNEL_6   /* GPIO34 */
#define JOY_Y_ADC_CHANNEL   ADC_CHANNEL_7   /* GPIO35 */
#define JOY_ADC_UNIT        ADC_UNIT_1

#define JOY_CALIB_SAMPLES   64    
#define JOY_DEADZONE_NORM   0.05f 

static const char *TAG = "HW_JOYSTICK";

/* ── Variáveis Globais Privadas (static) ─────────────────────── */
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static float s_center_x = 2048.0f;
static float s_center_y = 2048.0f;

/* ── Funções Auxiliares Privadas ─────────────────────────────── */
static float normalize_axis(int raw, float center)
{
    float v = (raw - center) / 2048.0f;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    if (v > -JOY_DEADZONE_NORM && v < JOY_DEADZONE_NORM) v = 0.0f;
    return v;
}

/* ── Implementação da API Pública ────────────────────────────── */

void joystick_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = JOY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, JOY_X_ADC_CHANNEL, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc1_handle, JOY_Y_ADC_CHANNEL, &chan_config));

    ESP_LOGI(TAG, "Calibrando Joystick...");
    int64_t sum_x = 0, sum_y = 0;
    
    for (int i = 0; i < JOY_CALIB_SAMPLES; i++) {
        int raw_x = 0, raw_y = 0;
        adc_oneshot_read(s_adc1_handle, JOY_X_ADC_CHANNEL, &raw_x);
        adc_oneshot_read(s_adc1_handle, JOY_Y_ADC_CHANNEL, &raw_y);
        sum_x += raw_x;
        sum_y += raw_y;
        vTaskDelay(pdMS_TO_TICKS(5)); // Delay necessário para o ADC estabilizar
    }
    
    s_center_x = (float)sum_x / JOY_CALIB_SAMPLES;
    s_center_y = (float)sum_y / JOY_CALIB_SAMPLES;
    ESP_LOGI(TAG, "Calibrado: cx=%.1f cy=%.1f", s_center_x, s_center_y);
}

void joystick_read_normalized(float *x, float *y)
{
    int raw_x = 0, raw_y = 0;
    
    // Lê o hardware diretamente
    adc_oneshot_read(s_adc1_handle, JOY_X_ADC_CHANNEL, &raw_x);
    adc_oneshot_read(s_adc1_handle, JOY_Y_ADC_CHANNEL, &raw_y);

    // Aplica a matemática de normalização e passa o resultado para os ponteiros
    if (x != NULL) {
        *x = normalize_axis(raw_x, s_center_x);
    }
    if (y != NULL) {
        *y = normalize_axis(raw_y, s_center_y);
    }
}