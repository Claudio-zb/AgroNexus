# Firmware AgroNexus

Dos proyectos PlatformIO con framework ESP-IDF para `esp32dev`.

| Proyecto | Plataforma PlatformIO | ESP-IDF registrado |
| --- | --- | --- |
| sensor-node | espressif32@7.0.1 | 6.0.1 |
| gateway | espressif32@6.12.0 | 5.5.0 |

La diferencia de SDK es intencional: el receptor usa 5.5.0 por compatibilidad con la pantalla y Wi-Fi. No unificar ni actualizar sin verificar ambos dispositivos. La plataforma del transmisor se fijó según la instalación local existente.

Se conserva `src/`, los archivos CMake y `sdkconfig.esp32dev` de cada proyecto. El gateway conserva además `sdkconfig.defaults` para certificados TLS. El componente `components/agronexus_telemetry` comparte codificación y recepción de humedad RAW y batería en mV. El montaje está en [sensor-node/README.md](sensor-node/README.md).

Desde la raíz:

```sh
pio run -d firmware/sensor-node
pio run -d firmware/gateway
```

Si `pio` no está en PATH, usar el terminal de PlatformIO o `~/.platformio/penv/bin/pio`.

Ver los README de cada proyecto antes de cargar firmware. Los originales de `legacy_code/` se conservan localmente, excluidos de Git porque contienen credenciales.
