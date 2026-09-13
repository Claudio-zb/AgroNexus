# Verificación de la integración

- Nodo sensor: compilación correcta con PlatformIO espressif32@7.0.1 / ESP-IDF 6.0.1. Código C idéntico al original.
- Gateway: compilación correcta con espressif32@6.12.0 / ESP-IDF 5.5.0, tanto con configuración vacía como con valores ficticios para verificar el enlace completo de Wi-Fi, OLED y HTTPS. Después se restauró secrets.local.h vacío.
- El binario de prueba completo del gateway ocupa 993929 de 1048576 bytes de la partición de aplicación (94.8%). El binario del sensor ocupa 169117 bytes (16.1%).
- Ambos muestran una advertencia: la placa esp32dev declara 4 MB de flash y sdkconfig configura 2 MB. Es una discrepancia entre configuraciones, no una medición del hardware. Se conserva el valor original; verificar la flash física antes de cargar o ajustar particiones.
- Originales de legacy_code verificados por hash sin modificaciones, excluyendo artefactos .pio.
- Se verificó que los cuatro valores privados del receptor original no aparecen en los archivos elegibles para Git y que las reglas excluyen originales, configuración privada, .env y .pio.
- No se cargó firmware, no se conectó a Supabase y no se validó el flujo en hardware. Los binarios actuales son de prueba: completar la configuración local y recompilar antes de cargar.
- No se ejecutaron git add, commit ni push.
