#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "can_packer.h"

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("FAIL line %d: %s\n", __LINE__, #cond);                  \
            failures++;                                                    \
        }                                                                  \
    } while (0)

#define CHECK_CLOSE(a, b) CHECK(fabsf((a) - (b)) < 1e-4f)

static can_signal_t sig(uint8_t start, uint8_t len, float factor,
                        float offset, bool is_signed, bool little_endian)
{
    can_signal_t s;
    s.start_bit = start;
    s.length = len;
    s.factor = factor;
    s.offset = offset;
    s.is_signed = is_signed;
    s.little_endian = little_endian;
    return s;
}

int main(void)
{
    uint8_t pdu[8];
    can_signal_t s;
    float phys;

    /* 1. Intel, 12-bit unsigned at bit 0: 0xABC -> BC 0A */
    memset(pdu, 0, sizeof pdu);
    s = sig(0, 12, 1.0f, 0.0f, false, true);
    CHECK(can_pack_signal(pdu, 8, &s, 2748.0f) == 0);
    CHECK(pdu[0] == 0xBC && pdu[1] == 0x0A);
    CHECK(can_unpack_signal(pdu, 8, &s, &phys) == 0);
    CHECK_CLOSE(phys, 2748.0f);

    /* 2. Motorola, 12-bit, MSB at bit 7: 0xABC -> AB C0 */
    memset(pdu, 0, sizeof pdu);
    s = sig(7, 12, 1.0f, 0.0f, false, false);
    CHECK(can_pack_signal(pdu, 8, &s, 2748.0f) == 0);
    CHECK(pdu[0] == 0xAB && pdu[1] == 0xC0);
    CHECK(can_unpack_signal(pdu, 8, &s, &phys) == 0);
    CHECK_CLOSE(phys, 2748.0f);

    /* 3. Signed Intel 8-bit at bit 8: -5 -> 0xFB in byte 1 */
    memset(pdu, 0, sizeof pdu);
    s = sig(8, 8, 1.0f, 0.0f, true, true);
    CHECK(can_pack_signal(pdu, 8, &s, -5.0f) == 0);
    CHECK(pdu[1] == 0xFB);
    CHECK(can_unpack_signal(pdu, 8, &s, &phys) == 0);
    CHECK_CLOSE(phys, -5.0f);

    /* 4. Scaling: 10-bit, factor 0.5, offset -40 (coolant-temp style) */
    memset(pdu, 0, sizeof pdu);
    s = sig(16, 10, 0.5f, -40.0f, false, true);
    CHECK(can_pack_signal(pdu, 8, &s, 90.0f) == 0); /* raw = 260 */
    CHECK(pdu[2] == 0x04 && pdu[3] == 0x01);
    CHECK(can_unpack_signal(pdu, 8, &s, &phys) == 0);
    CHECK_CLOSE(phys, 90.0f);

    /* 5. Out-of-range physical value clamps to raw max */
    memset(pdu, 0, sizeof pdu);
    s = sig(0, 8, 1.0f, 0.0f, false, true);
    CHECK(can_pack_signal(pdu, 8, &s, 9999.0f) == 0);
    CHECK(pdu[0] == 0xFF);

    /* 6. Signal exceeding the PDU is rejected */
    s = sig(60, 8, 1.0f, 0.0f, false, true);
    CHECK(can_pack_signal(pdu, 8, &s, 1.0f) == -1);

    /* 7. Motorola crossing bytes: 16-bit, MSB at bit 15: 0x1234 */
    memset(pdu, 0, sizeof pdu);
    s = sig(15, 16, 1.0f, 0.0f, false, false);
    CHECK(can_pack_signal(pdu, 8, &s, 0x1234) == 0);
    CHECK(pdu[1] == 0x12 && pdu[2] == 0x34);
    CHECK(can_unpack_signal(pdu, 8, &s, &phys) == 0);
    CHECK_CLOSE(phys, 4660.0f);

    /* 8. Round-trip over a few layouts (values exactly representable) */
    {
        const can_signal_t layouts[] = {
            sig(0, 1, 1.0f, 0.0f, false, true),
            sig(3, 5, 2.0f, -10.0f, true, true),
            sig(23, 9, 0.5f, 0.0f, false, false),
            sig(31, 32, 1.0f, 0.0f, true, false),
        };
        const float phys_vals[] = { 1.0f, 8.0f, 7.0f, 7.0f };
        size_t k;
        for (k = 0; k < sizeof layouts / sizeof layouts[0]; k++) {
            memset(pdu, 0, sizeof pdu);
            CHECK(can_pack_signal(pdu, 8, &layouts[k], phys_vals[k]) == 0);
            CHECK(can_unpack_signal(pdu, 8, &layouts[k], &phys) == 0);
            CHECK_CLOSE(phys, phys_vals[k]);
        }
    }

    if (failures == 0) {
        printf("All tests passed.\n");
    }
    return failures != 0;
}
