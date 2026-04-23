# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`chan_dongle` is an Asterisk channel driver for SIM7600 and compatible SIMCOM cellular modules. It bridges Asterisk SIP/PJSIP channels to GSM voice calls, SMS, and USSD over USB serial.

**Target Asterisk version:** 20.x only. All `ASTERISK_VERSION_NUM` preprocessor checks have been removed.

## Build System

Uses GNU Autotools. Pre-generated `configure` is included.

```bash
# Configure (auto-detects Asterisk headers and modules dir)
./configure

# Or with custom paths
./configure --with-asterisk-headers=/usr/local/include/asterisk --with-asterisk-modules=/usr/local/lib/asterisk/modules

# Build
make

# Run unit tests
make check
# Or manually
./tests/test_at_parse

# Install to detected Asterisk modules directory
sudo make install

# Regenerate build system (if configure.ac or Makefile.am changed)
./autogen.sh
```

Dependencies: `build-essential`, `autoconf`, `automake`, `libtool`, `libsqlite3-dev`, `asterisk-dev` (headers).

## Architecture

### Core Components

- **`chan_dongle.c/h`** — Module entry point, device lifecycle, monitor thread that polls devices and reads AT responses.
- **`channel.c/h`** — Asterisk channel interface (`ast_channel_tech`). Handles audio I/O (`read`/`write`), DTMF, calling, answering, and hanging up. Contains the call state machine dispatcher.
- **`at_command.c/h`** — Constructs AT command strings and enqueues them. Key functions: `at_enqueue_dial()`, `at_enqueue_answer()`, `at_enqueue_hangup()`, `at_enqueue_pcmreg()`.
- **`at_response.c/h`** — Parses asynchronous AT responses and URCs from the modem. This is where SIM7600-specific behavior is handled.
- **`at_queue.c/h`** — Manages the outgoing AT command queue, handles timeouts and retries.
- **`at_parse.c/h`** — Low-level parsing helpers for individual response types.
- **`cpvt.c/h`** — Call-private data structure. One `cpvt` exists per call on a device. Tracks call state, flags, audio pipes, and mixstream.
- **`mixbuffer.c/h`** — Audio mixing for conference/hold scenarios. Attaches/detaches `cpvt` mixstreams.
- **`pdiscovery.c/h`** — USB device discovery and port enumeration.

### Call State Machine

Defined in `cpvt.h` (`call_state_t`):

```
INIT -> DIALING -> ACTIVE -> RELEASED
INIT -> INCOMING -> ACTIVE -> RELEASED
```

State transitions happen in `channel.c:change_channel_state()`. This function is **not** a pure state setter; it has side effects:
- Transitioning to `CALL_STATE_ACTIVE` calls `activate_call()` (attaches audio fds, sends `AT+CPCMREG=1` for SIM7600).
- Transitioning to `CALL_STATE_RELEASED` calls `disactivate_call()` (detaches audio, sends `AT+CPCMREG=0`) and then **`cpvt_free()`**, which removes the `cpvt` from `pvt->chans` and frees it.

### Data Flows

**AT commands (control):**
```
Dialplan/App -> at_queue.c -> at_command.c -> /dev/ttyUSB0 (data tty)
                                    ^
                              at_response.c (async URCs/responses)
```

**Audio:**
```
Asterisk channel <-> channel.c <-> mixbuffer.c <-> /dev/ttyUSB4 (audio tty)
```

Frame size is 320 bytes (20ms @ 8kHz 16-bit signed linear mono).

## SIM7600-Specific Behavior

The driver auto-detects SIM7600/SIMCOM modules in `at_response.c:at_response_cgmi()` when `AT+CGMI` returns "SIMCOM" or "SimTech". It sets `pvt->has_voice_simcom = 1`.

When `has_voice_simcom` is true:

- **No `AT^CVOICE`** — These modules do not support the Quectel-style `AT^CVOICE` command.
- **`VOICE CALL: BEGIN` / `END`** — Instead of `^CONN:<idx>,<type>` (call connected) and `NO CARRIER` (call ended), the SIM7600 emits `VOICE CALL: BEGIN` and `VOICE CALL: END`. The handlers in `at_response.c` (`RES_VOICE_CALL_B` and `RES_VOICE_CALL_E`) translate these into state transitions via `change_channel_state()`.
- **`AT+CPCMREG`** — PCM audio is enabled/disabled explicitly. `activate_call()` sends `AT+CPCMREG=1`; `disactivate_call()` sends `AT+CPCMREG=0`. Both check `CALL_FLAG_ACTIVATED` for idempotency.

## Critical Safety Patterns

### Iterator Invalidation

`change_channel_state(cpvt, CALL_STATE_RELEASED, 0)` calls `cpvt_free(cpvt)`, which removes the element from `pvt->chans` and frees it. **Never continue iterating `AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry)` after releasing the current element.** Always `break` immediately after the state change.

This was the root cause of a segfault in `RES_VOICE_CALL_E`.

### AT+CPCMREG Idempotency

Do **not** send `AT+CPCMREG` explicitly in response handlers. Always route through `change_channel_state()` -> `activate_call()` / `disactivate_call()`. These functions check `CALL_FLAG_ACTIVATED` and only send the command when necessary. Sending it twice causes the module to error on rapid redial.

## Testing

### Unit Tests

`tests/test_at_parse.c` — Standalone test for AT parsing functions. Run with `make check` or directly `./tests/test_at_parse`.

### Manual Asterisk Testing

```bash
# Check module loaded
sudo asterisk -rx "module show like chan_dongle"

# Check device state
sudo asterisk -rx "dongle show devices"
sudo asterisk -rx "dongle show device dongle0"

# Originate a test call
sudo asterisk -rx "channel originate dongle/dongle0/+8613800100186 application playback demo-congrats"

# Check active channels
sudo asterisk -rx "core show channels"

# Debug logging
sudo asterisk -rx "core set debug 9"
sudo tail -f /var/log/asterisk/full | grep -E "DONGLE|SIM7600"
```

## CI / Quality

GitHub Actions workflows:
- **ci.yml** — Builds against Asterisk 20.11.0 source, runs `make check`.
- **static-analysis.yml** — Valgrind memcheck on tests, CodeQL analysis.
- **pre-commit.yml** — Runs pre-commit hooks.

Pre-commit hooks (`.pre-commit-config.yaml`) include `trailing-whitespace`, `end-of-file-fixer`, `check-yaml`, `check-json`, and `cppcheck` for C code. Install with `pip install pre-commit && pre-commit install`.
