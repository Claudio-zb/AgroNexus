# Gateway

Origen: `legacy_code/receptor`. PlatformIO espressif32@6.12.0 con ESP-IDF 5.5.0, mantenido por compatibilidad con OLED y Wi-Fi.

Recibe por UART2 (TX17/RX16 a 9600 baudios), muestra estados en OLED SH1106 (SDA21/SCL22, dirección 0x3C) y hace POST HTTPS a Supabase.

## Configuración local

En esta integración se creó `include/secrets.local.h` vacío e ignorado por Git. En otro equipo, copiar `include/secrets.example.h` a `include/secrets.local.h` antes de compilar.

Completar localmente `WIFI_SSID`, `WIFI_PASS`, `SUPABASE_URL` y `SUPABASE_KEY`. La URL debe ser el endpoint REST completo de la tabla existente. Usar la clave publishable; no usar claves secret o service_role. No guardar valores reales en el ejemplo, documentación ni sdkconfig.

El gateway retorna al iniciar si faltan SSID, URL o clave. Se permite contraseña vacía para redes abiertas. El archivo de ejemplo contiene únicamente cadenas vacías; no se usan archivos .env.

```sh
pio run -d firmware/gateway
```

Para cargar explícitamente: `pio run -d firmware/gateway -t upload`. Monitor: `pio device monitor -b 115200`.

Recibe tramas AN1 con humedad y batería; las lecturas se muestran en serial y la batería en la OLED, en milivoltios. El parser comparte código con el transmisor, reconstruye mensajes fragmentados y acepta las tramas antiguas de solo humedad. Ver [protocolo](../../docs/lora-protocol.md).

Por defecto `AGRONEXUS_SUPABASE_SCHEMA_V2=0`: envía únicamente `nodo` y `contador` al esquema remoto existente. **La batería aún no se persiste en ese modo.** El modo 1 envía `humedad_raw` y `bateria_mv` (null cuando no está disponible); activarlo solo cuando el destino tenga la nueva migración aplicada. Esta tarea no cambia la base remota. No hay cola de reintentos para envíos fallidos.

Los binarios compilados con valores reales contienen la configuración: `.pio/` queda excluido de Git.
