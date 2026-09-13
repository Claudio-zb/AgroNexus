# Protocolo LoRa AgroNexus AN1

Trama ASCII `AN1,<humedad_raw>,<bateria_mv>\n`, sin espacios, a 9600 baudios 8N1. Humedad: entero 0–4095. Batería: entero 0–5000 mV o -1 si no está disponible. Ejemplo: `AN1,2048,3980\n`. La trama no incluye identificador: este MVP tiene un transmisor y el gateway lo etiqueta `nodo_1`. No conectar varios transmisores esperando distinguirlos.

El componente `firmware/components/agronexus_telemetry` comparte el formato entre SDK 6.0.1 y 5.5.0. El gateway acumula hasta salto de línea, admite CRLF, procesa varios mensajes por lectura UART y descarta tramas inválidas, NUL y desbordamientos hasta el próximo salto de línea. También acepta el entero seguido de salto de línea del transmisor anterior; en ese caso la batería queda no disponible.

Actualizar primero el gateway y después el transmisor. El receptor antiguo interpretaría incorrectamente el prefijo AN1 con `atoi`.

El gateway conserva por defecto el contrato remoto `nodo`/`contador`; batería se muestra por serial/OLED, pero **no se guarda remotamente todavía**. `AGRONEXUS_SUPABASE_SCHEMA_V2=1` en `firmware/gateway/include/gateway_config.h` activa `nodo`/`humedad_raw`/`bateria_mv`, convirtiendo -1 a JSON null. Usarlo solamente con un destino donde exista el nuevo esquema. No se ha aplicado SQL remoto.

No hay ACK, cola persistente, autenticación de tramas ni reintentos HTTP. El POST es bloqueante y ráfagas prolongadas pueden superar el buffer UART. AUX no se usa; la prueba inicial debe comprobar la recepción estable con el intervalo de 5 s.

Pruebas del protocolo sin hardware, desde la raíz:

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I firmware/components/agronexus_telemetry/include \
  firmware/components/agronexus_telemetry/agronexus_telemetry.c \
  firmware/tests/telemetry_test.c -o /tmp/agronexus-telemetry-test
/tmp/agronexus-telemetry-test
```
