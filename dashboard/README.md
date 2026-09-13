# Dashboard AgroNexus

Se construirá una aplicación nueva dentro de esta carpeta. El código, dependencias y lockfile se conservarán en este mismo repositorio.

Propuesta inicial pendiente de confirmar: última lectura por nodo, historial de humedad RAW y estado de actualización. La consulta de datos se definirá a partir del esquema real exportado de Supabase. No convertir RAW a porcentaje sin calibración ni presentar datos simulados como mediciones reales.

Después de implementar y verificar localmente la aplicación, se conectará Vercel al repositorio de GitHub con `dashboard/` como directorio raíz. No se ha creado ni conectado ningún proyecto Vercel en esta etapa.

El código y los cambios de base de datos serán revisables en Git; los datos remotos, credenciales y configuración del servicio seguirán requiriendo sus conexiones respectivas. Tener el código local no exporta automáticamente esas configuraciones.

No incorporar claves, tokens ni archivos .env. Documentar solamente los nombres de variables necesarias. Los commits y push requieren una indicación explícita del usuario.
