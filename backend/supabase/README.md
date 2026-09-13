# Supabase: esquema propuesto de AgroNexus

`public.mediciones` corresponde al primer MVP de comunicación IoT: sensor ESP32 → LoRa → gateway → Supabase. La migración inicial **se ha rediseñado localmente** para separar humedad y batería y reducir permisos. Ya no es una reproducción literal del remoto.

`schema.snapshot.json` e `inspect_public.sql` conservan la captura y la consulta del esquema remoto original, con `contador`, sus grants amplios y políticas anónimas. No se sobrescribió esa evidencia ni se modificó Supabase remoto.

## Modelo nuevo

| Columna | Tipo | Condición |
| --- | --- | --- |
| id | bigint | Identidad ALWAYS, clave primaria, generada por el servidor |
| fecha | timestamptz | NOT NULL, default now(), generada por el servidor |
| nodo | text | NOT NULL, nodo_1 en este MVP de un transmisor |
| humedad_raw | integer | NOT NULL, 0–4095; no es porcentaje |
| bateria_mv | integer | 0–5000 mV o NULL si no está disponible |

El índice `mediciones_fecha_idx` ordena por fecha descendente. La identidad crea su secuencia y el índice de clave primaria. No se calcula porcentaje de batería ni se exporta estado de secuencias.

## Permisos mínimos del MVP

- `anon`: únicamente INSERT de `nodo`, `humedad_raw`, `bateria_mv`, limitado por RLS a `nodo_1`. No puede elegir id/fecha ni consultar filas. El gateway utiliza `Prefer: return=minimal`.
- `authenticated`: únicamente SELECT, y RLS permite solo los nodos presentes en `app_metadata.agronexus_nodes` del JWT. Un administrador deberá asignar, por ejemplo, `{"agronexus_nodes":["nodo_1"]}` al usuario que deba leer ese nodo. No se asignaron permisos a usuarios remotos en esta tarea. Un usuario sin ese atributo no ve filas; cambios en el atributo requieren renovar su JWT.
- Se revocan permisos preexistentes por defaults en la tabla y la secuencia, incluidos los de `service_role`. No se conceden UPDATE, DELETE, TRUNCATE ni acceso directo a la secuencia. La identidad usa su secuencia internamente al insertar.
- No se cambian `public`, sus propietarios, roles administrados ni privilegios por defecto globales. Se presupone el entorno estándar Supabase con USAGE de public para sus roles API.

La inserción `anon` conserva el modo actual del firmware; **no autentica al ESP32 ni evita que alguien con acceso a la API envíe lecturas falsas**. Los grants mínimos no resuelven esa limitación del prototipo. Una siguiente fase puede incorporar identidad de gateway y controles de ingestión. La lectura del dashboard queda restringida por autorización explícita de nodo, no solo por iniciar sesión.

## Compatibilidad y aplicación

La base remota actual sigue usando `nodo` y `contador`. El gateway, con `AGRONEXUS_SUPABASE_SCHEMA_V2=0`, mantiene ese POST y muestra batería únicamente por serial/OLED. Con el modo 1 envía `humedad_raw` y `bateria_mv`; -1 del protocolo pasa a NULL.

**No aplicar esta migración inicial sobre el proyecto existente.** Está destinada a una base vacía. Para actualizar el remoto hará falta una migración incremental revisada que trate `contador`, los datos existentes, políticas y permisos. Esa operación no está autorizada ni realizada aquí. Activar el modo 1 del gateway solo cuando su destino use el esquema nuevo.

## Archivos y reproducción

- `migrations/20260913203640_initial_public_schema.sql`: propuesta revisada, creada originalmente con Supabase CLI 2.117.0 mediante `migration new`.
- `config.toml`: configuración local PostgreSQL 17 sin seeds, inicializada con la misma CLI; no es configuración exportada de Auth/Storage remotos.
- `schema.snapshot.json`: referencia histórica del remoto antes del rediseño; **no debe compararse esperando igualdad con la nueva migración**.
- `inspect_public.sql`: SELECT de metadatos; sirve para inspeccionar el catálogo sin leer mediciones.

La CLI temporal usada está en `/private/tmp/agronexus-supabase-cli/supabase`; no está versionada ni instalada en PATH. Consultar `supabase --help` y las ayudas de subcomandos antes de usarlos. Para una instancia local nueva con Docker disponible: `supabase start --workdir backend`. Para reconstruir solo una instancia local desechable: `supabase db reset --local --no-seed --workdir backend`, tras revisar `db reset --help`.

No se ha ejecutado una restauración local ni pruebas de RLS contra PostgreSQL: faltan Docker/PostgreSQL en el entorno. Se revisaron estáticamente las columnas enviadas por el gateway, los límites y las políticas; falta validar su ejecución y denegaciones en una instancia local. No se ejecutaron CLI de aplicación, cambios de base remota, git add, commits ni push. No se incluyen datos, claves, passwords, tokens ni .env.

Referencia de autorización: [RLS y app_metadata en Supabase](https://supabase.com/docs/guides/database/postgres/row-level-security).
