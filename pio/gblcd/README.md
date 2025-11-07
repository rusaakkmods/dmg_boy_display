# Game Boy LCD Capture (gblcd.pio)

A high-performance PIO (Programmable I/O) program for capturing Game Boy LCD data using the Raspberry Pi Pico.

## Author

**Ken (What's Ken Making)**
- GitHub: [@whatskenmaking](https://github.com/whatskenmaking)
- YouTube: [What's Ken Making](https://www.youtube.com/@whatskenmaking)
- Master Repository: [Pico-DMG-LCD](https://github.com/whatskenmaking/Pico-DMG-LCD)

## Overview

The `gblcd.pio` is a specialized PIO program designed to capture raw LCD data from the original Game Boy (DMG) hardware in real-time. This library enables the Raspberry Pi Pico to act as a high-speed data capture interface between the Game Boy's LCD controller and external display devices.

## What it Does

The gblcd.pio program captures:
- **Pixel Data**: Raw 2-bit grayscale pixel information from the Game Boy LCD
- **Timing Signals**: VSYNC and pixel clock synchronization
- **Frame Data**: Complete 160x144 pixel frames at 59.7 Hz refresh rate
- **Real-time Processing**: Zero-latency capture suitable for live display output

## How it Works

### PIO Architecture
The program utilizes the RP2040's Programmable I/O (PIO) **Single State Machine** to captures pixel data and VSYNC with precise clock synchronization
   - Waits for clock edges on PIO pin index 0 (GPIO 2 - Game Boy clock)
   - Reads 4 input pins simultaneously using pin indices 0-3 (GPIO 2-5)
   - PIO pin index 0 (GPIO 2): Game Boy clock signal (CPG)
   - PIO pin index 1 (GPIO 3): Pixel data bit 0 (LD0)
   - PIO pin index 2 (GPIO 4): Pixel data bit 1 (LD1)
   - PIO pin index 3 (GPIO 5): VSYNC signal
   - Transfers data to main CPU via FIFO

### Signal Capture Process

```
Game Boy LCD → PIO State Machine → CPU Processing → External Display
```

#### 1. Signal Input
- **CPG (Clock Pulse Generator)**: 4.194304 MHz pixel clock from Game Boy (GPIO 2)
- **LD0-LD1**: 2-bit pixel data lines (grayscale values 0-3) on GPIO 3-4
- **VSYNC**: Vertical sync signal for frame timing (GPIO 5)
- Note: PIO waits on clock (GPIO 2) then captures all signals simultaneously

#### 2. Data Processing
- PIO captures 2-bit pixels at 4.194304 MHz rate
- Each line contains 160 pixels (320 bits of data)
- Frame consists of 144 active lines
- Automatic synchronization with Game Boy timing using VSYNC

#### 3. Memory Management
- CPU polling of PIO FIFO for continuous capture
- 23,040 bytes per frame (160×144×1 byte per pixel)
- Frame synchronization via VSYNC detection
- Direct transfer to display drivers

### Technical Specifications

| Parameter | Value |
|-----------|-------|
| Input Clock | 4.194304 MHz |
| Frame Rate | ~59.7 Hz |
| Resolution | 160×144 pixels |
| Color Depth | 2-bit grayscale (4 shades) |
| Data Rate | ~1.97 MB/s |
| Latency | <1 frame (~16.7ms) |

## Usage

### Hardware Connections

**Important**: This project supports two hardware versions with different GPIO assignments:

#### Hardware Version v1.0 (Legacy)
```cpp
#define GB_PIN_BASE   2   // PIO input base pin
// Game Boy connections:
// GPIO 2 (PIO pin 0): Game Boy pixel clock (CPG)
// GPIO 3 (PIO pin 1): Game Boy data bit 0 (LD0)  
// GPIO 4 (PIO pin 2): Game Boy data bit 1 (LD1)
// GPIO 5 (PIO pin 3): Game Boy vertical sync (VSYNC)
```

#### Hardware Version v1.1a (Current)
```cpp
#define GB_PIN_BASE   9   // PIO input base pin  
// Game Boy connections:
// GPIO 9 (PIO pin 0): Game Boy pixel clock (CPG)
// GPIO 10 (PIO pin 1): Game Boy data bit 0 (LD0)
// GPIO 11 (PIO pin 2): Game Boy data bit 1 (LD1)  
// GPIO 12 (PIO pin 3): Game Boy vertical sync (VSYNC)
```

#### PIO Configuration Notes
The PIO program uses consecutive GPIO pins starting from `GB_PIN_BASE`:
- PIO automatically assigns 4 consecutive GPIO pins as input pins
- Pin assignment is set via `gblcd_program_init(pio, sm, offset, GB_PIN_BASE)`
- The base pin configuration is defined in `config.h` based on hardware version

### Software Integration

The PIO program is automatically compiled and included in your project:

```cpp
#include "gblcd.pio.h"
#include "config.h"

// Initialize PIO program with hardware-specific pin configuration
PIO pio = pio0;
uint state_machine_id = 0;
uint offset = pio_add_program(pio, &gblcd_program);
gblcd_program_init(pio, state_machine_id, offset, GB_PIN_BASE);

// GB_PIN_BASE is defined in config.h:
// - v1.0 hardware: GB_PIN_BASE = 2 (GPIO 2-5)
// - v1.1a hardware: GB_PIN_BASE = 9 (GPIO 9-12)
```

## Performance Features

- **Minimal CPU Overhead**: PIO handles timing-critical pixel capture
- **FIFO Integration**: Automatic data transfers without complex buffering
- **Real-time Capture**: Frame-synchronized capture via VSYNC detection
- **Low Latency**: Immediate processing suitable for gaming applications
- **Power Efficient**: Dedicated hardware reduces power consumption

## Applications

This library enables various Game Boy LCD capture applications:
- External display drivers (ST7789, ILI9341, ST7796)
- LCD replacement/upgrade projects
- Game Boy video recording systems
- Real-time streaming solutions
- Emulator development and testing

## Hardware Version Support

This PIO program supports two hardware configurations:

### Version v1.0 (Legacy)
- **GPIO Assignment**: 2-5 (sequential from GPIO 2)
- **SPI Interface**: SPI1 (GPIO 9-13 for display)
- **Features**: Basic Game Boy capture with manual brightness control
- **Pin Conflicts**: None between Game Boy capture and display SPI

### Version v1.1a (Current)  
- **GPIO Assignment**: 9-12 (sequential from GPIO 9)
- **SPI Interface**: SPI0 (GPIO 3-7 for display)
- **Features**: Advanced controls with mode switching (palette/brightness)
- **Improvements**: Better pin separation, enhanced control interface

### Migration Notes
When upgrading hardware from v1.0 to v1.1a:
1. Update `GB_PIN_BASE` in config.h (2 → 9)
2. Rewire Game Boy capture signals to new GPIO pins
3. Update display SPI connections for SPI0 interface
4. Build firmware with `-DVERSION_V1_1a` flag

The PIO program automatically adapts to the configured pin assignment through the `GB_PIN_BASE` parameter.

## Technical Notes

### Timing Requirements
- The PIO program is carefully tuned for Game Boy LCD timing
- Clock edges and data sampling are precisely aligned
- VSYNC detection ensures proper frame synchronization

### Buffer Management
- Implements ping-pong buffering for continuous operation
- Automatic detection of frame boundaries
- Configurable buffer sizes for different display types

### Compatibility
- Designed for original Game Boy (DMG) hardware
- Compatible with Game Boy Color in DMG mode
- Supports various Game Boy LCD revisions

## License

This project is part of the Pico-DMG-LCD repository. Please refer to the main repository for licensing information.

## Contributing

For questions, issues, or contributions:
1. Visit the [main repository](https://github.com/whatskenmaking/Pico-DMG-LCD)
2. Check out the [YouTube channel](https://www.youtube.com/@whatskenmaking) for tutorials
3. Submit issues or pull requests on GitHub

---

