#include "bridge.h"
#include <stdio.h>
int main(void)
{
    static const size_t lengths[] = {0, 1, 16, 127, 128};
    static const uint8_t sequences[] = {0, 0x7d, 0x7e, 255};
    uint8_t payload[128], wire[BR_WIRE_MAX];
    for (unsigned l = 0; l < 5; ++l) {
        for (unsigned s = 0; s < 4; ++s) {
            for (size_t i = 0; i < lengths[l]; ++i)
                payload[i] = (uint8_t)(i * 17u + sequences[s]);
            size_t n = bridge_encode(wire, BR_DATA, 2, sequences[s], payload, lengths[l]);
            for (size_t i = 0; i < n; ++i) printf("%02x", wire[i]);
            puts("");
        }
    }
    return 0;
}
