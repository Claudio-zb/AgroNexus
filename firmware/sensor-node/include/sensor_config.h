#pragma once

// Montaje definido para ESP32 clásico (esp32dev).
#define SOIL_ADC_CHANNEL ADC_CHANNEL_6     // GPIO34
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_7  // GPIO35
#define BATTERY_R_TOP_OHM 10000            // batería + -> ADC
#define BATTERY_R_BOTTOM_OHM 10000         // ADC -> GND
#define ADC_SAMPLES 64
#define SAMPLE_INTERVAL_MS 5000

// Dejar en 0 para exigir calibración eFuse. Solo completar si se ha medido Vref.
#define BATTERY_DEFAULT_VREF_MV 0
