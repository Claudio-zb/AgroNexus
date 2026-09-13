# Arquitectura actual

Sensor capacitivo → ADC del nodo ESP32 → UART/E220 → radio LoRa → E220/UART del gateway → HTTPS → Supabase.

El gateway muestra estados Wi-Fi, LoRa y HTTP en OLED SH1106. El dashboard en Vercel forma parte de la arquitectura prevista, pero su código no está en este repositorio todavía.

La integración conserva dos SDK distintos por compatibilidad: transmisor 6.0.1 y receptor 5.5.0. Se mantienen los proyectos PlatformIO independientes.

Batería 18650, TP4056 y panel solar forman parte del hardware previsto. El transmisor mide batería con un divisor 10 kΩ/10 kΩ en GPIO35 y la envía junto con humedad por AN1. El gateway muestra ambas lecturas, pero mantiene por defecto el POST remoto antiguo hasta adoptar el nuevo esquema. No hay gestión de ahorro energético. El valor del sensor es RAW, sin calibración a porcentaje.
