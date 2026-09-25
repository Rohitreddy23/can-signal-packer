#ifndef CAN_PACKER_H
#define CAN_PACKER_H

/* Pack/unpack CAN signals into PDU bytes, AUTOSAR-COM style.
 *
 * Bit numbering: bit 0 is the LSB of byte 0, bit 63 the MSB of byte 7.
 *  - Intel (little-endian): start_bit is the LSB position of the signal;
 *    signal bits occupy start_bit .. start_bit + length - 1.
 *  - Motorola (big-endian): start_bit is the MSB position of the signal;
 *    bits run "downhill": within a byte toward bit 0, then continue at
 *    bit 7 of the next byte.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t start_bit;    /* Intel: LSB pos / Motorola: MSB pos (0..63) */
    uint8_t length;       /* signal length in bits, 1..32 */
    float   factor;       /* physical = raw * factor + offset */
    float   offset;
    bool    is_signed;
    bool    little_endian; /* true: Intel, false: Motorola */
} can_signal_t;

/* Pack a physical value into the PDU. Out-of-range values are clamped
 * to the signal's raw range. Returns 0 on success, -1 on bad params. */
int can_pack_signal(uint8_t *pdu, size_t pdu_len,
                    const can_signal_t *sig, float physical);

/* Unpack a physical value from the PDU. Returns 0 on success,
 * -1 on bad params. */
int can_unpack_signal(const uint8_t *pdu, size_t pdu_len,
                      const can_signal_t *sig, float *physical);

#ifdef __cplusplus
}
#endif

#endif /* CAN_PACKER_H */
