-- AgroNexus: esquema inicial propuesto para humedad y batería.
-- Destino: base Supabase PostgreSQL 17 vacía con esquema public y roles estándar.
-- NO es una copia literal del remoto ni una actualización para la tabla existente.
-- La captura histórica del remoto se conserva en schema.snapshot.json.

CREATE TABLE public.mediciones (
    id bigint GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    fecha timestamp with time zone DEFAULT now() NOT NULL,
    nodo text NOT NULL,
    humedad_raw integer NOT NULL,
    bateria_mv integer,
    CONSTRAINT mediciones_nodo_check CHECK (nodo = 'nodo_1'),
    CONSTRAINT mediciones_humedad_raw_check CHECK (humedad_raw BETWEEN 0 AND 4095),
    CONSTRAINT mediciones_bateria_mv_check CHECK (bateria_mv BETWEEN 0 AND 5000)
);

CREATE INDEX mediciones_fecha_idx ON public.mediciones (fecha DESC);
ALTER TABLE public.mediciones ENABLE ROW LEVEL SECURITY;

-- Elimina permisos heredados de defaults Supabase solo en estos objetos nuevos.
REVOKE ALL PRIVILEGES ON TABLE public.mediciones
    FROM PUBLIC, anon, authenticated, service_role;
REVOKE ALL PRIVILEGES ON SEQUENCE public.mediciones_id_seq
    FROM PUBLIC, anon, authenticated, service_role;

-- El gateway puede insertar solo las lecturas: no el id ni la fecha del servidor.
-- La identidad genera el id internamente, sin conceder acceso directo a la secuencia.
GRANT INSERT (nodo, humedad_raw, bateria_mv) ON public.mediciones TO anon;
CREATE POLICY mediciones_gateway_insert ON public.mediciones
    FOR INSERT TO anon
    WITH CHECK (nodo = 'nodo_1');

-- Lectura para usuarios del dashboard autorizados explícitamente por nodo.
-- app_metadata debe asignarlo un administrador; no usar user_metadata editable.
GRANT SELECT ON public.mediciones TO authenticated;
CREATE POLICY mediciones_dashboard_select ON public.mediciones
    FOR SELECT TO authenticated
    USING (
        ((SELECT auth.jwt()) -> 'app_metadata' -> 'agronexus_nodes') ? nodo
    );

-- Sin UPDATE, DELETE, TRUNCATE, permisos de esquema, cambios de OWNER
-- ni ALTER DEFAULT PRIVILEGES. No se crean roles ni se incluyen claves o datos.
-- La inserción anon mantiene la compatibilidad del MVP: no autentica el dispositivo.
