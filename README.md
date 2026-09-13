# AgroNexus

Proyecto IoT orientado al monitoreo agrícola.

## Objetivo

Desarrollar una red de nodos sensores capaces de medir variables agrícolas y transmitir la información hacia una plataforma central para su almacenamiento, visualización y análisis.

## Tecnologías

* ESP32
* LoRa
* Sensores de humedad de suelo
* Supabase
* Vercel
* Python
* Git y GitHub

## Estado

Proyecto en desarrollo.

## Organización del código

- [Firmware](firmware/README.md): nodo sensor y gateway, proyectos ESP-IDF con PlatformIO y SDK independientes.
- [Backend](backend/README.md): espacio para recuperar esquema y migraciones de Supabase.
- [Dashboard](dashboard/README.md): pendiente de integrar el código existente de Vercel.
- [Arquitectura](docs/architecture.md) y [guía de Git](docs/git-workflow.md).

Los originales de `legacy_code/` se conservan localmente y están excluidos de Git. La configuración privada del gateway vive en `secrets.local.h`, también excluido. No versionar claves, contraseñas ni archivos .env.
