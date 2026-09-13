#pragma once

// 0: compatible con la tabla remota actual (nodo, contador).
// 1: nueva migración local (nodo, humedad_raw, bateria_mv).
// Cambiar a 1 solo cuando el endpoint destino tenga el nuevo esquema.
#ifndef AGRONEXUS_SUPABASE_SCHEMA_V2
#define AGRONEXUS_SUPABASE_SCHEMA_V2 0
#endif
