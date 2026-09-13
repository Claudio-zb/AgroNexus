#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "driver/uart.h"
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

/* =====================================================
   CONFIGURACION
   ===================================================== */

#include "secrets.local.h"
#include "agronexus_telemetry.h"
#include "gateway_config.h"

/* E220 */

#define LORA_UART UART_NUM_2
#define LORA_TX   GPIO_NUM_17
#define LORA_RX   GPIO_NUM_16

/* OLED */

#define SDA_PIN   GPIO_NUM_21
#define SCL_PIN   GPIO_NUM_22
#define OLED_ADDR 0x3C

/* =====================================================
   VARIABLES DE ESTADO
   ===================================================== */

static volatile bool wifi_ok = false;
static volatile bool db_ok = false;
static int http_status = 0;
static int contador = 0;
static int bateria_mv = AGRONEXUS_BATTERY_UNKNOWN;

static i2c_master_dev_handle_t oled;

static EventGroupHandle_t wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0

/* =====================================================
   FUENTE OLED 5x7
   ===================================================== */

static const uint8_t font_digits[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};

static const uint8_t font_letters[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}  // Z
};

/* =====================================================
   OLED
   ===================================================== */

static void oled_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};

    ESP_ERROR_CHECK(
        i2c_master_transmit(oled, data, 2, 100)
    );
}

static void oled_data(const uint8_t *data, size_t len)
{
    uint8_t buffer[129];

    buffer[0] = 0x40;

    memcpy(&buffer[1], data, len);

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            oled,
            buffer,
            len + 1,
            100
        )
    );
}

static void oled_clear(void)
{
    uint8_t blank[128] = {0};

    for (int page = 0; page < 8; page++)
    {
        oled_cmd(0xB0 + page);

        oled_cmd(0x02);
        oled_cmd(0x10);

        oled_data(blank, 128);
    }
}

static const uint8_t *get_char(char c)
{
    static const uint8_t space[5] =
        {0,0,0,0,0};

    static const uint8_t colon[5] =
        {0x00,0x36,0x36,0x00,0x00};

    if (c >= '0' && c <= '9')
        return font_digits[c - '0'];

    if (c >= 'A' && c <= 'Z')
        return font_letters[c - 'A'];

    if (c >= 'a' && c <= 'z')
        return font_letters[c - 'a'];

    if (c == ':')
        return colon;

    return space;
}

static void oled_print(
    uint8_t page,
    uint8_t column,
    const char *text)
{
    oled_cmd(0xB0 + page);

    /* SH1106 tiene offset de 2 columnas */

    column += 2;

    oled_cmd(0x00 | (column & 0x0F));
    oled_cmd(0x10 | (column >> 4));

    while (*text)
    {
        const uint8_t *glyph =
            get_char(*text);

        oled_data(glyph, 5);

        uint8_t espacio = 0;

        oled_data(&espacio, 1);

        text++;
    }
}

static void actualizar_pantalla(void)
{
    char linea[24];

    oled_clear();

    oled_print(
        0,
        0,
        "AGRO NEXUS"
    );

    if (wifi_ok)
        oled_print(2, 0, "WIFI: OK");
    else
        oled_print(2, 0, "WIFI: NO");

    snprintf(
        linea,
        sizeof(linea),
        "LORA: %d",
        contador
    );

    oled_print(
        4,
        0,
        linea
    );

    if (bateria_mv >= 0)
        snprintf(linea, sizeof(linea), "BAT: %d MV", bateria_mv);
    else
        snprintf(linea, sizeof(linea), "BAT: ND");
    oled_print(5, 0, linea);

    if (db_ok)
        oled_print(6, 0, "DB: OK");
    else
        oled_print(6, 0, "DB: NO");
    snprintf(
    linea,
    sizeof(linea),
    "HTTP: %d",
    http_status
);

oled_print(7, 0, linea);
}

static void oled_init(void)
{
    i2c_master_bus_handle_t bus;

    i2c_master_bus_config_t bus_config = {

        .i2c_port = I2C_NUM_0,

        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,

        .clk_source =
            I2C_CLK_SRC_DEFAULT,

        .glitch_ignore_cnt = 7,

        .flags.enable_internal_pullup =
            true
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &bus
        )
    );

    i2c_device_config_t dev_config = {

        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            OLED_ADDR,

        .scl_speed_hz =
            400000
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus,
            &dev_config,
            &oled
        )
    );

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    oled_cmd(0xAE);

    oled_cmd(0xD5);
    oled_cmd(0x80);

    oled_cmd(0xA8);
    oled_cmd(0x3F);

    oled_cmd(0xD3);
    oled_cmd(0x00);

    oled_cmd(0x40);

    oled_cmd(0xAD);
    oled_cmd(0x8B);

    oled_cmd(0xA1);
    oled_cmd(0xC8);

    oled_cmd(0xDA);
    oled_cmd(0x12);

    oled_cmd(0x81);
    oled_cmd(0x7F);

    oled_cmd(0xD9);
    oled_cmd(0x22);

    oled_cmd(0xDB);
    oled_cmd(0x35);

    oled_cmd(0xA4);
    oled_cmd(0xA6);

    oled_cmd(0xAF);

    oled_clear();

    oled_print(
        3,
        0,
        "INICIANDO..."
    );
}

/* =====================================================
   WIFI
   ===================================================== */

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START
    )
    {
        esp_wifi_connect();
    }

    else if (
        event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED
    )
    {
        wifi_ok = false;

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        printf(
            "WiFi desconectado\n"
        );

        esp_wifi_connect();
    }

    else if (
        event_base == IP_EVENT &&
        event_id == IP_EVENT_STA_GOT_IP
    )
    {
        wifi_ok = true;

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        printf(
            "WiFi conectado\n"
        );
    }
}

static void wifi_init(void)
{
    wifi_event_group =
        xEventGroupCreate();

    esp_err_t ret =
        nvs_flash_init();

    if (
        ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND
    )
    {
        ESP_ERROR_CHECK(
            nvs_flash_erase()
        );

        ESP_ERROR_CHECK(
            nvs_flash_init()
        );
    }

    ESP_ERROR_CHECK(
        esp_netif_init()
    );

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler,
            NULL
        )
    );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        }
    };

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(
            WIFI_MODE_STA
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );
}

/* =====================================================
   SUPABASE
   ===================================================== */

static void enviar_supabase(
    int valor)
{
    if (!wifi_ok)
    {
        db_ok = false;

        printf(
            "Sin WiFi\n"
        );

        return;
    }

    char json[128];

#if AGRONEXUS_SUPABASE_SCHEMA_V2
    char battery_json[16];
    if (bateria_mv < 0) snprintf(battery_json, sizeof(battery_json), "null");
    else snprintf(battery_json, sizeof(battery_json), "%d", bateria_mv);
    snprintf(json, sizeof(json),
             "{\"nodo\":\"nodo_1\",\"humedad_raw\":%d,\"bateria_mv\":%s}", valor, battery_json);
#else
    // Compatibilidad mientras no se aplique el nuevo esquema en el destino.
    snprintf(json, sizeof(json), "{\"nodo\":\"nodo_1\",\"contador\":%d}", valor);
#endif

    esp_http_client_config_t config = {
        .url = SUPABASE_URL,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach =
            esp_crt_bundle_attach
    };

    esp_http_client_handle_t client =
        esp_http_client_init(&config);

    esp_http_client_set_header(
        client,
        "apikey",
        SUPABASE_KEY
    );

    esp_http_client_set_header(
        client,
        "Content-Type",
        "application/json"
    );

    esp_http_client_set_header(
        client,
        "Prefer",
        "return=minimal"
    );

    esp_http_client_set_post_field(
        client,
        json,
        strlen(json)
    );

    esp_err_t err =
        esp_http_client_perform(
            client
        );

    if (err == ESP_OK)
{
    http_status =
        esp_http_client_get_status_code(client);

    db_ok =
        (http_status >= 200 &&
         http_status < 300);

    printf("HTTP: %d\n", http_status);
}
else
{
    http_status = -1;
    db_ok = false;

    printf(
        "HTTPS ERROR: %s\n",
        esp_err_to_name(err)
    );
}

esp_http_client_cleanup(client);
}

/* =====================================================
   LORA
   ===================================================== */

static void lora_init(void)
{
    uart_config_t config = {

        .baud_rate = 9600,

        .data_bits =
            UART_DATA_8_BITS,

        .parity =
            UART_PARITY_DISABLE,

        .stop_bits =
            UART_STOP_BITS_1,

        .flow_ctrl =
            UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            LORA_UART,
            1024,
            0,
            0,
            NULL,
            0
        )
    );

    ESP_ERROR_CHECK(
        uart_param_config(
            LORA_UART,
            &config
        )
    );

    ESP_ERROR_CHECK(
        uart_set_pin(
            LORA_UART,
            LORA_TX,
            LORA_RX,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );
}

/* =====================================================
   MAIN
   ===================================================== */

void app_main(void)
{
    if (WIFI_SSID[0] == '\0' || SUPABASE_URL[0] == '\0' || SUPABASE_KEY[0] == '\0') {
        printf("Configura include/secrets.local.h antes de iniciar el gateway.\n");
        return;
    }

    printf(
        "Iniciando Agro Nexus...\n"
    );

    /* Primero OLED para poder mostrar estados */

    oled_init();

    /* LoRa */

    lora_init();

    /* WiFi */

    wifi_init();

    char buffer[128];
    agronexus_decoder_t decoder = {0};

    TickType_t ultima_pantalla =
        0;

    while (1)
    {
        /* Leer LoRa */

        int len =
            uart_read_bytes(
                LORA_UART,
                buffer,
                sizeof(buffer) - 1,
                pdMS_TO_TICKS(100)
            );

        for (int i = 0; i < len; ++i) {
            agronexus_reading_t reading;
            if (agronexus_feed(&decoder, buffer[i], &reading)) {
                contador = reading.humidity_raw;
                bateria_mv = reading.battery_mv;
                printf("Humedad RAW: %d; bateria mV: %d\n", contador, bateria_mv);
                enviar_supabase(contador);
                actualizar_pantalla();
            }
        }

        /* Refrescar estados cada 5 segundos */

        if (
            xTaskGetTickCount()
            - ultima_pantalla
            >= pdMS_TO_TICKS(5000)
        )
        {
            actualizar_pantalla();

            ultima_pantalla =
                xTaskGetTickCount();
        }

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}