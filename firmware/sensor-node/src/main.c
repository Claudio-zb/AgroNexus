#include <stdio.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "agronexus_telemetry.h"
#include "sensor_config.h"

#define LORA_UART UART_NUM_2
#define LORA_TX GPIO_NUM_17
#define LORA_RX GPIO_NUM_16
static const char *TAG = "sensor";
_Static_assert(BATTERY_R_TOP_OHM > 0 && BATTERY_R_BOTTOM_OHM > 0, "Divisor invalido");
_Static_assert(ADC_SAMPLES > 0 && ADC_SAMPLES <= 256, "Muestreo invalido");

static esp_err_t average(adc_oneshot_unit_handle_t adc, adc_channel_t channel, int *raw)
{
    int sum = 0;
    int discarded;
    esp_err_t err = adc_oneshot_read(adc, channel, &discarded);
    if (err != ESP_OK) return err;
    for (int i = 0; i < ADC_SAMPLES; ++i) {
        int sample;
        err = adc_oneshot_read(adc, channel, &sample);
        if (err != ESP_OK) return err;
        sum += sample;
    }
    *raw = (sum + ADC_SAMPLES / 2) / ADC_SAMPLES;
    return ESP_OK;
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t adc_config = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc));
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, SOIL_ADC_CHANNEL, &channel_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, BATTERY_ADC_CHANNEL, &channel_config));

    adc_cali_handle_t calibration = NULL;
    adc_cali_line_fitting_config_t calibration_config = {
        .unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12, .default_vref = BATTERY_DEFAULT_VREF_MV
    };
    esp_err_t calibration_error = adc_cali_create_scheme_line_fitting(&calibration_config, &calibration);
    if (calibration_error != ESP_OK) {
        calibration = NULL;
        ESP_LOGW(TAG, "Sin calibracion ADC (%s): bateria no disponible, envia -1", esp_err_to_name(calibration_error));
    }

    uart_config_t uart_config = {
        .baud_rate = 9600, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_driver_install(LORA_UART, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(LORA_UART, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(LORA_UART, LORA_TX, LORA_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    vTaskDelay(pdMS_TO_TICKS(100));

    while (1) {
        agronexus_reading_t reading = {.battery_mv = AGRONEXUS_BATTERY_UNKNOWN};
        esp_err_t err = average(adc, SOIL_ADC_CHANNEL, &reading.humidity_raw);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Lectura de humedad fallida: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
            continue;
        }
        int battery_raw;
        int pin_mv;
        if (calibration && average(adc, BATTERY_ADC_CHANNEL, &battery_raw) == ESP_OK &&
            battery_raw < 4095 && adc_cali_raw_to_voltage(calibration, battery_raw, &pin_mv) == ESP_OK) {
            int64_t scaled = (int64_t)pin_mv * (BATTERY_R_TOP_OHM + BATTERY_R_BOTTOM_OHM);
            int battery_mv = (int)((scaled + BATTERY_R_BOTTOM_OHM / 2) / BATTERY_R_BOTTOM_OHM);
            if (battery_mv >= 0 && battery_mv <= 5000) reading.battery_mv = battery_mv;
        }
        char frame[AGRONEXUS_FRAME_SIZE];
        int len = agronexus_encode(frame, sizeof(frame), &reading);
        if (len > 0) {
            int written = uart_write_bytes(LORA_UART, frame, len);
            if (written != len) ESP_LOGW(TAG, "Envio UART incompleto");
            ESP_LOGI(TAG, "Humedad RAW: %d; bateria mV: %d", reading.humidity_raw, reading.battery_mv);
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}
