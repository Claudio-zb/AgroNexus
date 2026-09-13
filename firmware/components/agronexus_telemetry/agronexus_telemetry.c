#include "agronexus_telemetry.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool number(const char **cursor, long min, long max, int *value)
{
    const char *start = *cursor;
    if (!(*start >= '0' && *start <= '9') && !(*start == '-' && start[1] == '1'))
        return false;
    char *end;
    errno = 0;
    long result = strtol(start, &end, 10);
    if (errno || end == start || result < min || result > max)
        return false;
    *cursor = end;
    *value = (int)result;
    return true;
}

int agronexus_encode(char *out, size_t size, const agronexus_reading_t *reading)
{
    if (reading->humidity_raw < 0 || reading->humidity_raw > 4095 ||
        reading->battery_mv < -1 || reading->battery_mv > 5000)
        return -1;
    int len = snprintf(out, size, "AN1,%d,%d\n", reading->humidity_raw, reading->battery_mv);
    return len >= 0 && (size_t)len < size ? len : -1;
}

bool agronexus_parse(const char *line, agronexus_reading_t *reading)
{
    agronexus_reading_t candidate = {.battery_mv = AGRONEXUS_BATTERY_UNKNOWN};
    bool versioned = strncmp(line, "AN1,", 4) == 0;
    const char *cursor = line + (versioned ? 4 : 0);
    if (!number(&cursor, 0, 4095, &candidate.humidity_raw))
        return false;
    if (versioned) {
        if (*cursor++ != ',' || !number(&cursor, -1, 5000, &candidate.battery_mv))
            return false;
    }
    if (*cursor == '\r') cursor++;
    if (*cursor != '\0') return false;
    *reading = candidate;
    return true;
}

bool agronexus_feed(agronexus_decoder_t *decoder, char byte, agronexus_reading_t *reading)
{
    if (byte == '\n') {
        decoder->line[decoder->used] = '\0';
        bool valid = !decoder->dropping && decoder->used > 0 && agronexus_parse(decoder->line, reading);
        decoder->used = 0;
        decoder->dropping = false;
        return valid;
    }
    if (decoder->dropping) return false;
    if (byte == '\0' || decoder->used >= sizeof(decoder->line) - 1) {
        decoder->dropping = true;
        return false;
    }
    decoder->line[decoder->used++] = byte;
    return false;
}
