# Nodo transmisor: humedad y batería

Firmware ESP-IDF 6.0.1 con PlatformIO `espressif32@7.0.1`, para un **ESP32 clásico esp32dev**. Mantiene GPIO34 y UART2 del montaje original; añade GPIO35 para batería. Este documento define el montaje a implementar, no certifica un circuito ya conectado.

## Conexiones al ESP32

Usar los nombres GPIO de la placa, no números de posición: el orden físico cambia entre DevKit de 30 y 38 pines.

| ESP32 | Conexión | Función |
| --- | --- | --- |
| GPIO34 | Salida analógica AOUT del sensor | Humedad RAW, ADC1 canal 6 |
| 3V3 | VCC del sensor capacitivo compatible con 3.3 V | Alimentación del sensor |
| GND | GND del sensor | Referencia común |
| GPIO35 | Punto medio de R1/R2 | Batería, ADC1 canal 7 |
| GPIO17 (TX2) | RXD del E220 | Transmisión UART |
| GPIO16 (RX2) | TXD del E220 | Recepción UART |
| Pin de entrada 5V de la DevKit | Salida regulada de 5 V del elevador | Alimentación de la placa, no del pin 3V3 |
| GND | OUT− protegido, GND del elevador y GND del E220 | Referencia común |

E220-900T22D: VCC a la salida regulada de 5 V; M0 y M1 a GND (modo transparente normal). UART usa lógica de 3.3 V y 9600 8N1. AUX queda sin conectar en este MVP; no hay control de disponibilidad del módulo. Colocar su antena antes de transmitir. Ambos radios deben tener frecuencia, canal, velocidad aérea y modo compatibles; el firmware no los programa ni garantiza que la configuración de fábrica sea la deseada para 915 MHz.

## Diagrama de cableado

```text
18650 (+) ───────────── B+   TP4056 CON PROTECCIÓN
18650 (−) ───────────── B−   (terminales B+/B− y OUT+/OUT−)

Salida OUT+ ── interruptor ── VBAT_SW ──┬── IN+ elevador regulado a 5 V
                                      │
                                    R1 10 kΩ (1 %)
                                      │
                                      ├────────── GPIO35 / ADC1_CH7
                                      │                 │
                                    R2 10 kΩ         C1 100 nF
                                      │                 │
Salida OUT− ─────────── GND ───────────┴─────────────────┘
                         ├── IN− y OUT− del elevador
                         ├── ESP32 GND
                         ├── sensor GND
                         └── E220 GND, M0 y M1

Elevador OUT+ (5 V) ──────┬── entrada 5V de ESP32 DevKit
                         └── E220 VCC
ESP32 3V3 ───────────────── sensor VCC
Sensor AOUT ─────────────── GPIO34 / ADC1_CH6
ESP32 GPIO17 / TX2 ──────── E220 RXD
ESP32 GPIO16 / RX2 ◀─────── E220 TXD
```

El divisor mide la salida protegida de la batería **antes del elevador**, después del interruptor. Así no se alimenta el GPIO35 desde la batería cuando el nodo está apagado. El negativo común es OUT−; no unir B− directamente a GND, porque se puede puentear la protección del módulo. Si tu TP4056 no tiene OUT+/OUT− y protección de descarga, este diagrama requiere añadir esa protección.

R1 y R2 son **dos resistencias de 10 kΩ**, preferiblemente 1 %. C1 es un capacitor cerámico de 100 nF entre GPIO35 y GND, cerca del ESP32. La 18650 nunca se conecta directamente al ADC ni al pin 3V3.

```text
V_GPIO35 = V_batería × R2 / (R1 + R2)
V_batería = V_GPIO35 × 2
4.20 V de batería → aproximadamente 2.10 V en GPIO35
```

El divisor consume aproximadamente 0.21 mA a 4.2 V mientras el interruptor está cerrado. Si las resistencias medidas difieren, actualizar `BATTERY_R_TOP_OHM` y `BATTERY_R_BOTTOM_OHM` en `include/sensor_config.h`.

Usar un elevador regulado a 5 V con capacidad suficiente para los picos del ESP32 y del radio; como objetivo de diseño, 1 A de salida. Ajustarlo y comprobar polaridad/voltajes con multímetro **antes** de conectar los módulos. Confirmar que la entrada marcada 5V/VIN de tu DevKit admite 5 V; no conectar esa salida al pin 3V3. No conectar simultáneamente USB y el elevador a la placa sin un circuito de selección/aislamiento de alimentación apropiado.

El TP4056 es el cargador, no el regulador de alimentación del ESP32. Para la primera prueba, cargar la batería con el nodo apagado. La entrada solar y un circuito de reparto de alimentación/carga quedan pendientes: no conectar un panel de tensión desconocida directamente a IN+/IN−. Usar para la carga una fuente de 5 V adecuada al módulo.

## Lecturas y mensajes

Promedia 64 muestras por canal con resolución de 12 bits y atenuación de 12 dB. La humedad se mantiene en RAW (0–4095); no es un porcentaje. La batería se convierte de ADC a milivoltios con calibración lineal de ESP-IDF y se corrige por el divisor 2:1.

Se usa calibración eFuse cuando está disponible. Si falta y `BATTERY_DEFAULT_VREF_MV` está en 0, transmite batería `-1` (no disponible), sin inventar un voltaje. Ese valor de configuración es la referencia interna ADC medida, **no** 3300 mV ni el voltaje de batería. Lecturas fallidas, saturación o resultados fuera de 0–5000 mV también producen `-1`. Un cero no permite distinguir batería ausente de un problema de cableado.

Cada ciclo espera 5 segundos después de medir y enviar:

```text
AN1,2048,3980\n
```

Significa versión de trama AN1, humedad RAW 2048 y batería 3980 mV (3.980 V). El salto de línea delimita el mensaje. Ver [protocolo](../../docs/lora-protocol.md).

## Compilar y comprobar

Desde la raíz:

```sh
pio run -d firmware/sensor-node
```

Actualizar también el gateway: el receptor antiguo basado en `atoi` no entiende AN1. El gateway nuevo sigue aceptando los enteros antiguos.

Antes de cargar: verificar el circuito con multímetro, confirmar aproximadamente la mitad del voltaje de batería en GPIO35 y revisar el tamaño real de flash de la DevKit. Los sdkconfig originales configuran 2 MB y esp32dev declara 4 MB; no se modificó esa discrepancia sin comprobar el hardware.

Para cargar explícitamente: `pio run -d firmware/sensor-node -t upload`. Monitor: `pio device monitor -b 115200`. Comparar la batería informada con el multímetro y registrar el error; calibración y tolerancia de resistencias afectan la medida. No se ha probado este montaje físicamente.

No implementa porcentaje de batería, deep sleep ni protección de descarga por software. La protección eléctrica debe existir en el hardware.

Referencias: [ADC ESP32 y calibración](https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_calibration.html), [datasheet ESP32](https://documentation.espressif.com/esp32_datasheet_en.html), [manual Ebyte E220-900T22D](https://www.ebyte.com/Uploadfiles/Files/2021-7-6/202176188184448.pdf). La API se contrastó además con los headers de ESP-IDF 6.0.1 instalados.
