#pragma once
#include <stdbool.h>
#include <stddef.h>

#define AGRONEXUS_FRAME_SIZE 48
#define AGRONEXUS_BATTERY_UNKNOWN (-1)

typedef struct {
    int humidity_raw;
    int battery_mv;
} agronexus_reading_t;

typedef struct {
    char line[AGRONEXUS_FRAME_SIZE];
    size_t used;
    bool dropping;
} agronexus_decoder_t;

int agronexus_encode(char *out, size_t size, const agronexus_reading_t *reading);
bool agronexus_parse(const char *line, agronexus_reading_t *reading);
bool agronexus_feed(agronexus_decoder_t *decoder, char byte, agronexus_reading_t *reading);
