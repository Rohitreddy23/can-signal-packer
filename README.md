[![CI](https://github.com/Rohitreddy23/can-signal-packer/actions/workflows/ci.yml/badge.svg)](https://github.com/Rohitreddy23/can-signal-packer/actions)

# CAN Signal Packer

A small, dependency-free **C library** for packing and unpacking CAN
signals into PDU bytes — the kind of thing an AUTOSAR COM module does
for every frame on the bus.

- **Intel (little-endian)** and **Motorola (big-endian)** bit layouts
- **Scaling & offset**: `physical = raw * factor + offset`
- **Signed / unsigned** signals with proper sign extension
- **Range clamping** on pack, **bounds checking** (signals that don't
  fit the PDU are rejected)

## API

```c
#include "can_packer.h"

uint8_t pdu[8] = {0};

can_signal_t rpm = {
    .start_bit = 16, .length = 16,
    .factor = 0.25f, .offset = 0.0f,
    .is_signed = false, .little_endian = true, /* Intel */
};

can_pack_signal(pdu, sizeof pdu, &rpm, 3000.0f);

float value;
can_unpack_signal(pdu, sizeof pdu, &rpm, &value); /* -> 3000.0 */
```

Both functions return `0` on success, `-1` on invalid parameters.

## Build & test

```bash
make test
```

The test suite covers Intel/Motorola layouts, byte-crossing signals,
signed values, scaling/offset round-trips, clamping, and invalid
signals — with zero external dependencies.

## Layout notes

- Bit 0 is the LSB of byte 0. For Intel, `start_bit` is the signal's
  LSB position; for Motorola it is the MSB position, with bits running
  "downhill" (toward bit 0, then continuing at bit 7 of the next byte)
  — matching DBC conventions.

Personal learning project — a portable take on COM-style signal handling.
