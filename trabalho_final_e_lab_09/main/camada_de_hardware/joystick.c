#include "joystick.h"

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "soc/soc_caps.h"

#define JOY_X_ADC_CHANNEL   ADC_CHANNEL_6   
#define JOY_Y_ADC_CHANNEL   ADC_CHANNEL_7
#define JOY_ADC_UNIT        ADC_UNIT_1

#define JOY_CALIB_SAMPLES   64    
#define JOY_DEADZONE_NORM   0.05f 
#define JOY_ADC_SAMPLE_FREQ_HZ 1000
#define JOY_ADC_FRAME_SIZE  SOC_ADC_DIGI_DATA_BYTES_PER_CONV

static const char *TAG = "HW_JOYSTICK";

static adc_continuous_handle_t s_adc_handle = NULL;
static float s_center_x = 2048.0f;
static float s_center_y = 2048.0f;
static volatile int s_raw_x = 2048;
static volatile int s_raw_y = 2048;

static float normalize_axis(int raw, float center)
{
    float v = (raw - center) / 2048.0f;
    if (v > 1.0f) v = 1.0f;
    if (v < -1.0f) v = -1.0f;
    if (v > -JOY_DEADZONE_NORM && v < JOY_DEADZONE_NORM) v = 0.0f;
    return v;
}

static bool joystick_adc_conv_done_cb(adc_continuous_handle_t handle,
                                      const adc_continuous_evt_data_t *edata,
                                      void *user_data)
{
    (void)handle;
    (void)user_data;

    for (uint32_t i = 0; i + SOC_ADC_DIGI_RESULT_BYTES <= edata->size; i += SOC_ADC_DIGI_RESULT_BYTES) {
        const adc_digi_output_data_t *sample =
            (const adc_digi_output_data_t *)&edata->conv_frame_buffer[i];
        const int channel = sample->type1.channel;
        const int data = sample->type1.data;

        if (channel == JOY_X_ADC_CHANNEL) {
            s_raw_x = data;
        } else if (channel == JOY_Y_ADC_CHANNEL) {
            s_raw_y = data;
        }
    }

    return false;
}

static void joystick_calibrate(void)
{
    adc_oneshot_unit_handle_t adc1_handle = NULL;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = JOY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_X_ADC_CHANNEL, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_Y_ADC_CHANNEL, &chan_config));

    ESP_LOGI(TAG, "Calibrando Joystick...");
    int64_t sum_x = 0, sum_y = 0;

    for (int i = 0; i < JOY_CALIB_SAMPLES; i++) {
        int raw_x = 0, raw_y = 0;
        adc_oneshot_read(adc1_handle, JOY_X_ADC_CHANNEL, &raw_x);
        adc_oneshot_read(adc1_handle, JOY_Y_ADC_CHANNEL, &raw_y);
        sum_x += raw_x;
        sum_y += raw_y;
        esp_rom_delay_us(5000);
    }

    s_center_x = (float)sum_x / JOY_CALIB_SAMPLES;
    s_center_y = (float)sum_y / JOY_CALIB_SAMPLES;
    s_raw_x = (int)s_center_x;
    s_raw_y = (int)s_center_y;

    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    ESP_LOGI(TAG, "Calibrado: cx=%.1f cy=%.1f", s_center_x, s_center_y);
}

void joystick_init(void)
{
    joystick_calibrate();

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 256,
        .conv_frame_size = JOY_ADC_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &s_adc_handle));

    adc_digi_pattern_config_t adc_pattern[2] = {
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = JOY_X_ADC_CHANNEL,
            .unit = JOY_ADC_UNIT,
            .bit_width = ADC_BITWIDTH_12,
        },
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = JOY_Y_ADC_CHANNEL,
            .unit = JOY_ADC_UNIT,
            .bit_width = ADC_BITWIDTH_12,
        },
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = JOY_ADC_SAMPLE_FREQ_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 2,
        .adc_pattern = adc_pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(s_adc_handle, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = joystick_adc_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(s_adc_handle, &cbs, NULL));

    ESP_ERROR_CHECK(adc_continuous_start(s_adc_handle));
    ESP_LOGI(TAG, "ADC continuo do joystick iniciado em %d Hz", JOY_ADC_SAMPLE_FREQ_HZ);
}

void joystick_read_normalized(float *x, float *y)
{
    const int raw_x = s_raw_x;
    const int raw_y = s_raw_y;

    if (x != NULL) {
        *x = normalize_axis(raw_x, s_center_x);
    }
    if (y != NULL) {
        *y = normalize_axis(raw_y, s_center_y);
    }
}
