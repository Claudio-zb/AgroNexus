idf_component_register(SRCS "main.c" INCLUDE_DIRS "../include"
    REQUIRES agronexus_telemetry esp_driver_uart esp_driver_i2c esp_wifi esp_event
    esp_netif nvs_flash esp_http_client mbedtls)
