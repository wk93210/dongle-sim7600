# chan_dongle - SIM7600/SIMCOM Channel Driver for Asterisk

Asterisk channel driver for SIM7600 and compatible SIMCOM cellular modules. Supports voice calls, SMS, and USSD on Asterisk 20.

## Features

- **Voice Calls**: Make and receive calls through the cellular module
- **SMS**: Send and receive SMS messages
- **USSD**: Send USSD codes and receive responses
- **Multiple Devices**: Support for multiple SIM7600 modules
- **SIM7600 Optimized**: Specifically tested and optimized for SIM7600 modules
- **Asterisk 20**: Compatible with Asterisk 20.x (previous versions dropped)

## Hardware Requirements

### Supported Modules
- **SIM7600E-H** (Primary target, fully tested)
- SIM7600A-H, SIM7600SA-H, SIM7600JC-H (should work)
- Other SIMCOM modules with similar AT command set

### System Requirements
- Linux kernel 5.x or newer recommended
- **Asterisk 20.x** (only version supported)
- USB port for module connection
- 256MB RAM minimum, 512MB recommended

### USB Connection
The SIM7600 exposes multiple USB serial ports:
- `/dev/ttyUSB0` - AT command port (primary)
- `/dev/ttyUSB1` - AT command port (secondary)
- `/dev/ttyUSB2` - Diagnostic/GPS (not used)
- `/dev/ttyUSB3` - Diagnostic (not used)
- `/dev/ttyUSB5` - PCM audio (voice data)

A udev rule typically creates `/dev/ttyUSB-PCM` as a symlink to `/dev/ttyUSB5`.

## Installation

### Prerequisites

```bash
# Debian/Ubuntu/Raspberry Pi OS
sudo apt-get update
sudo apt-get install -y build-essential autoconf automake libtool
sudo apt-get install -y libsqlite3-dev asterisk-dev

# Verify Asterisk headers are installed
ls /usr/include/asterisk/asterisk.h
```

### Building from Source

The project uses GNU Autotools for cross-platform builds. Pre-generated `configure` is included for convenience.

```bash
cd /path/to/dongle-sim7600

# Configure (auto-detects Asterisk installation)
./configure

# Or specify custom Asterisk paths
./configure --with-asterisk=/opt/asterisk
./configure --with-asterisk-headers=/usr/local/include/asterisk
./configure --with-asterisk-modules=/usr/local/lib/asterisk/modules

# Build
make

# Install (to detected or specified Asterisk modules directory)
sudo make install
```

#### Build Options

| Option | Description |
|--------|-------------|
| `--with-asterisk=DIR` | Asterisk installation prefix |
| `--with-asterisk-headers=DIR` | Asterisk headers directory |
| `--with-asterisk-modules=DIR` | Asterisk modules directory |
| `--disable-applications` | Disable dialplan applications |
| `--disable-manager` | Disable AMI extensions |

#### Regenerating Build System (Developers)

If you modify `configure.ac` or `Makefile.am`:

```bash
./autogen.sh    # Regenerates configure script
./configure
make
```

### Configuration

Sample configuration files are provided in the `etc/` directory:

```bash
# Copy sample configuration files
sudo cp etc/dongle.conf /etc/asterisk/dongle.conf
sudo cp etc/extensions.conf /etc/asterisk/extensions.d/dongle.conf
```

Then edit `/etc/asterisk/dongle.conf` to match your device:

```ini
[general]
interval=15
smsdb=/var/lib/asterisk/smsdb

[defaults]
context=dongle-incoming
rxgain=0
txgain=0

[dongle0]
; Device paths - adjust based on your system
; Use /dev/serial/by-id/... for persistent naming
data=/dev/ttyUSB0
audio=/dev/ttyUSB5

; Or use the PCM symlink if available
; audio=/dev/ttyUSB-PCM

; SIM identification (optional - used for auto-discovery)
;imei=123456789012345
;imsi=310260123456789

; Audio gain (-100 to 100)
rxgain=50
txgain=50

; Call waiting
callwaiting=no

; Auto-delete SMS after reading
autodeletesms=yes
```

See `etc/dongle.conf` for more configuration options including jitter buffer settings.

### Set Permissions

```bash
# Add asterisk user to dialout group
sudo usermod -a -G dialout asterisk

# Or set appropriate udev rules for device permissions
```

### Restart Asterisk

```bash
sudo systemctl restart asterisk
```

### Verify the Module Loaded

```bash
sudo asterisk -rx "module show like chan_dongle"
sudo asterisk -rx "dongle show devices"
```

## Dialplan Examples

Sample dialplan examples are provided in `etc/extensions.conf`. Key examples:

### Incoming Calls

```asterisk
[dongle-incoming]
exten => s,1,Verbose(Incoming call on ${DONGLENAME})
 same => n,Set(CALLERID(name)=${CALLERID(num)})
 same => n,Dial(PJSIP/100,30)
 same => n,Hangup()
```

### Outgoing Calls

```asterisk
[outgoing]
; Dial using dongle0
exten => _X.,1,Dial(Dongle/dongle0/${EXTEN})
```

### SMS Handling

```asterisk
[dongle-incoming]
exten => sms,1,Verbose(Incoming SMS from ${CALLERID(num)})
 same => n,System(echo '${STRFTIME(${EPOCH},,%Y-%m-%d %H:%M:%S)} - ${DONGLENAME} - ${CALLERID(num)}: ${BASE64_DECODE(${SMS_BASE64})}' >> /var/log/asterisk/sms.log)
 same => n,Hangup()
```

### USSD Handling

```asterisk
[dongle-incoming]
exten => ussd,1,Verbose(Incoming USSD: ${BASE64_DECODE(${USSD_BASE64})})
 same => n,System(echo '${STRFTIME(${EPOCH},,%Y-%m-%d %H:%M:%S)} - ${DONGLENAME}: ${BASE64_DECODE(${USSD_BASE64})}' >> /var/log/asterisk/ussd.log)
 same => n,Hangup()
```

See `etc/extensions.conf` for more dialplan examples including:
- Subscriber number routing
- DongleStatus application usage
- Channel function usage (callstate, dtmf)
- Conference and hold options

## Applications

### DongleSendSMS

Send SMS from dialplan:

```asterisk
exten => *123,1,DongleSendSMS(dongle0,+1234567890,Hello World,14400,0,)
```

Parameters:
1. Device name (e.g., dongle0)
2. Destination number
3. Message text
4. Validity period (minutes)
5. Report request (0/1)
6. Payload (included in delivery report)

### DongleSendUSSD

Send USSD code:

```asterisk
exten => *222,1,DongleSendUSSD(dongle0,*101#)
```

### DongleStatus

Check device status:

```asterisk
exten => *555,1,DongleStatus(dongle0,STATUS)
 same => n,Verbose(Status: ${STATUS})
```

Status values:
- `1` - Device not found
- `2` - Device connected and free
- `3` - Device connected and in use

## CLI Commands

```bash
# Show connected devices
sudo asterisk -rx "dongle show devices"

# Show device details
sudo asterisk -rx "dongle show device dongle0"

# Send SMS from CLI
sudo asterisk -rx "dongle sms dongle0 +1234567890 Hello"

# Send USSD from CLI
sudo asterisk -rx "dongle ussd dongle0 *101#"

# Reset device
sudo asterisk -rx "dongle reset dongle0"

# Reload configuration
sudo asterisk -rx "module reload chan_dongle"
```

## Troubleshooting

### Check Device Detection

```bash
# List USB devices
ls -la /dev/ttyUSB*

# Check kernel messages
dmesg | grep -i "ttyUSB\|simcom\|7600"

# Check module state
sudo asterisk -rx "dongle show device dongle0"
```

### PCM Audio Issues

If you see "Write error!" messages:

1. Verify audio device path in dongle.conf
2. Check permissions on `/dev/ttyUSB5`
3. Ensure no other process is using the device

Debug logging:
```bash
sudo asterisk -rx "core set verbose 5"
sudo asterisk -rx "core set debug 5"
sudo tail -f /var/log/asterisk/full | grep -E "DONGLE|Write error"
```

### Common Issues

**Module won't load:**
- Check Asterisk version (must be 20.x)
- Verify sqlite3 is installed
- Check for conflicting modules

**No audio on calls:**
- Verify audio=/dev/ttyUSB5 in dongle.conf
- Check `rxgain` and `txgain` settings
- Ensure PCM device permissions

**SMS not sending:**
- Check signal strength: `dongle show device dongle0`
- Verify SIM can send SMS normally
- Check SMS center number is configured

## Architecture

### Key Components

- **chan_dongle.c** - Main module, device management
- **channel.c** - Asterisk channel interface, audio I/O
- **at_command.c** - AT command construction
- **at_response.c** - AT response parsing
- **at_queue.c** - Command queue management
- **mixbuffer.c** - Audio mixing for conference calls

### Audio Flow

```
Asterisk Channel <-> channel.c <-> mixbuffer.c <-> /dev/ttyUSB5 (PCM)
```

Frame size: 320 bytes (20ms @ 8kHz 16-bit signed linear)

### AT Command Flow

```
Dialplan/App <-> at_queue.c <-> at_command.c <-> /dev/ttyUSB0
                                      ↑
                              at_response.c (async responses)
```

## Development

### Building for Development

```bash
# Clean previous builds
make clean

# Configure with debug symbols
./configure CFLAGS="-g -O0"

# Build
make

# Install and reload
sudo make install
sudo asterisk -rx "module reload chan_dongle"
```

### Running Tests

```bash
# Enable debug output
sudo asterisk -rx "core set debug 9"
sudo asterisk -rx "dongle reset dongle0"
```

### Pre-commit Hooks

This project uses [pre-commit](https://pre-commit.com/) to ensure code quality.

```bash
# Install pre-commit
pip install pre-commit

# Install hooks in your local repo
pre-commit install

# Run hooks manually on all files
pre-commit run --all-files
```

Configured hooks:
- **Trailing whitespace** - Remove trailing whitespace
- **End of file fixer** - Ensure files end with a newline
- **YAML/JSON checks** - Validate configuration files
- **clang-format** - C code formatting
- **cppcheck** - Static analysis for C/C++
- **Shellcheck** - Bash script linting

### Code Structure

```
dongle-sim7600/
├── app.c/h           - Dialplan applications
├── at_command.c/h    - AT command generation
├── at_parse.c/h      - AT response parsing
├── at_queue.c/h      - Command queue
├── at_read.c/h       - Device reading
├── at_response.c/h   - Response handling
├── chan_dongle.c/h   - Main module
├── channel.c/h       - Asterisk channel interface
├── char_conv.c/h     - Character encoding
├── cli.c/h           - CLI commands
├── configure.ac      - Autoconf configuration
├── Makefile.am       - Automake configuration
├── autogen.sh        - Build system generator
├── cpvt.c/h          - Call private data
├── dc_config.c/h     - Device configuration
├── etc/              - Sample configuration files
├── helpers.c/h       - Utility functions
├── manager.c/h       - AMI events
├── mixbuffer.c/h     - Audio mixing
├── pdu.c/h           - SMS PDU encoding
├── ringbuffer.c/h    - Ring buffer
├── smsdb.c/h         - SMS database
└── tests/            - Unit tests
```

## License

This project is licensed under the GPLv2. See LICENSE.txt for details.

## Acknowledgments

- Original chan_dongle by Artem Makhutov, Dmitry Vagin, and bg
- Quectel support added by various contributors
- SIM7600 fixes and modern Asterisk compatibility

## Support

For issues, questions, or contributions, please use the GitHub issue tracker.

## Version History

- **v2.0** - SIM7600 optimized release, Asterisk 20 only
  - Dropped support for Asterisk 14/16/18 (Asterisk 20 only)
  - Removed all ASTERISK_VERSION_NUM preprocessor checks
  - Fixed PCM write errors with retry logic and usleep delays
  - Added SIM7600-specific "VOICE CALL: BEGIN/END" handling
  - SIMCOM auto-detection via AT+CGMI
  - Added autotools build system for cross-platform compilation
  - Removed dead code: contrib/, test/, tools/, BUGS, TODO.txt
  - Updated documentation with sample config files
  - Fixed build for modern glibc/GCC compatibility
  - Added unit tests for AT parsing functions
  - Added pre-commit hooks for code quality
  - Added GitHub Actions workflows for CI and static analysis
