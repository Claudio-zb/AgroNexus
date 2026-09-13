#include "agronexus_telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    agronexus_reading_t r = {0};
    assert(agronexus_parse("AN1,2048,4200", &r));
    assert(r.humidity_raw == 2048 && r.battery_mv == 4200);
    assert(agronexus_parse("AN1,0,-1\r", &r));
    assert(r.battery_mv == -1);
    assert(agronexus_parse("4095", &r) && r.battery_mv == -1);
    const char *invalid[] = {"", "AN2,1,2", "AN1,4096,4200", "AN1,-1,4200",
        "AN1,4,5001", "AN1,4,-2", "AN1,4,", "AN1,4,12garbage", "AN1,4,3,2",
        "9999999999999999999999999999", "AN1,4,9999999999999999999999999",
        "nan", " 123", "AN1,1,+2", "AN1,1,3.7", "AN1,4"};
    for (size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i)
        assert(!agronexus_parse(invalid[i], &r));
    char frame[AGRONEXUS_FRAME_SIZE];
    r = (agronexus_reading_t){.humidity_raw = 1250, .battery_mv = 3980};
    int length = agronexus_encode(frame, sizeof(frame), &r);
    assert(length == (int)strlen("AN1,1250,3980\n"));
    assert(strcmp(frame, "AN1,1250,3980\n") == 0);
    assert(agronexus_encode(frame, 3, &r) == -1);
    agronexus_decoder_t decoder = {0};
    const char *chunks[] = {"AN1,12", "50,39", "80\n1024\nAN1,2048,-1\r\n"};
    int count = 0;
    for (size_t c = 0; c < 3; ++c)
        for (const char *p = chunks[c]; *p; ++p)
            if (agronexus_feed(&decoder, *p, &r)) ++count;
    assert(count == 3 && r.humidity_raw == 2048 && r.battery_mv == -1);
    for (int i = 0; i < 200; ++i) assert(!agronexus_feed(&decoder, '1', &r));
    assert(!agronexus_feed(&decoder, '\n', &r));
    const char binary[] = {'1', '\0', '2', '\n'};
    for (size_t i = 0; i < sizeof(binary); ++i) assert(!agronexus_feed(&decoder, binary[i], &r));
    const char *recovered = "AN1,100,3700\n";
    count = 0;
    for (const char *p = recovered; *p; ++p) if (agronexus_feed(&decoder, *p, &r)) ++count;
    assert(count == 1 && r.battery_mv == 3700);
    puts("OK: encoding, legacy, fragmented/combined frames, invalid values, overflow and resynchronization");
}
