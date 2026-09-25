#include "can_packer.h"

#include <math.h>
#include <stdint.h>

#define BITS_PER_BYTE 8

static int put_bit(uint8_t *pdu, size_t pdu_len, unsigned pos, int val)
{
    if (pos >= pdu_len * BITS_PER_BYTE) {
        return -1;
    }
    if (val) {
        pdu[pos / BITS_PER_BYTE] |= (uint8_t)(1u << (pos % BITS_PER_BYTE));
    } else {
        pdu[pos / BITS_PER_BYTE] &= (uint8_t)~(1u << (pos % BITS_PER_BYTE));
    }
    return 0;
}

static int get_bit(const uint8_t *pdu, size_t pdu_len, unsigned pos)
{
    if (pos >= pdu_len * BITS_PER_BYTE) {
        return -1;
    }
    return (pdu[pos / BITS_PER_BYTE] >> (pos % BITS_PER_BYTE)) & 1u;
}

/* Motorola (big-endian) bit walk: from the MSB position, bits run toward
 * bit 0 of the byte, then continue at bit 7 of the next byte. */
static unsigned moto_next(unsigned pos)
{
    return (pos % BITS_PER_BYTE == 0) ? pos + 15u : pos - 1u;
}

static int validate(const can_signal_t *sig, size_t pdu_len)
{
    unsigned i, pos;

    if (sig == NULL || pdu_len == 0) {
        return -1;
    }
    if (sig->length == 0 || sig->length > 32) {
        return -1;
    }
    if (sig->factor == 0.0f) {
        return -1;
    }
    if (sig->start_bit >= pdu_len * BITS_PER_BYTE) {
        return -1;
    }
    if (sig->little_endian) {
        if ((unsigned)sig->start_bit + sig->length > pdu_len * BITS_PER_BYTE) {
            return -1;
        }
    } else {
        /* walk the Motorola layout; every bit must land inside the PDU */
        pos = sig->start_bit;
        for (i = 0; i < sig->length; i++) {
            if (pos >= pdu_len * BITS_PER_BYTE) {
                return -1;
            }
            pos = moto_next(pos);
        }
    }
    return 0;
}

int can_pack_signal(uint8_t *pdu, size_t pdu_len,
                    const can_signal_t *sig, float physical)
{
    int64_t raw_min, raw_max, raw, mask;
    uint32_t u;
    unsigned i, pos;

    if (pdu == NULL || validate(sig, pdu_len) != 0) {
        return -1;
    }

    if (sig->is_signed) {
        raw_min = -(1LL << (sig->length - 1));
        raw_max = (1LL << (sig->length - 1)) - 1;
    } else {
        raw_min = 0;
        raw_max = (1LL << sig->length) - 1;
    }

    raw = llroundf((physical - sig->offset) / sig->factor);
    if (raw < raw_min) {
        raw = raw_min;
    }
    if (raw > raw_max) {
        raw = raw_max;
    }

    mask = (sig->length == 32) ? 0xFFFFFFFFLL : (1LL << sig->length) - 1;
    u = (uint32_t)(raw & mask);

    if (sig->little_endian) {
        for (i = 0; i < sig->length; i++) {
            put_bit(pdu, pdu_len, sig->start_bit + i, (u >> i) & 1u);
        }
    } else {
        pos = sig->start_bit;
        for (i = 0; i < sig->length; i++) {
            put_bit(pdu, pdu_len, pos, (u >> (sig->length - 1 - i)) & 1u);
            pos = moto_next(pos);
        }
    }
    return 0;
}

int can_unpack_signal(const uint8_t *pdu, size_t pdu_len,
                      const can_signal_t *sig, float *physical)
{
    uint32_t u = 0;
    int32_t s;
    unsigned i, pos;
    int b;

    if (pdu == NULL || physical == NULL || validate(sig, pdu_len) != 0) {
        return -1;
    }

    if (sig->little_endian) {
        for (i = 0; i < sig->length; i++) {
            b = get_bit(pdu, pdu_len, sig->start_bit + i);
            if (b < 0) {
                return -1;
            }
            u |= (uint32_t)b << i;
        }
    } else {
        pos = sig->start_bit;
        for (i = 0; i < sig->length; i++) {
            b = get_bit(pdu, pdu_len, pos);
            if (b < 0) {
                return -1;
            }
            u |= (uint32_t)b << (sig->length - 1 - i);
            pos = moto_next(pos);
        }
    }

    if (sig->is_signed && sig->length < 32 &&
        (u & (1u << (sig->length - 1))) != 0) {
        /* sign-extend */
        u |= (uint32_t)(~0u << sig->length);
    }
    s = (int32_t)u;

    *physical = (float)s * sig->factor + sig->offset;
    return 0;
}
