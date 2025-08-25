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
- **Timing Signals**: HSYNC, VSYNC, and pixel clock synchronization
- **Frame Data**: Complete 160x144 pixel frames at 59.7 Hz refresh rate
- **Real-time Processing**: Zero-latency capture suitable for live display output

## How it Works

### PIO Architecture
The program utilizes the RP2040's Programmable I/O (PIO) state machines to:

1. **State Machine 0**: Captures horizontal sync (HSYNC) and manages line timing
2. **State Machine 1**: Captures pixel data with precise clock synchronization  
3. **State Machine 2**: Handles vertical sync (VSYNC) and frame management
4. **State Machine 3**: Manages data transfer to main CPU via DMA

### Signal Capture Process

```
Game Boy LCD → PIO State Machines → DMA → CPU Processing → External Display
```

#### 1. Signal Input
- **CPG (Clock Pulse Generator)**: 4.194304 MHz pixel clock from Game Boy
- **LD0-LD1**: 2-bit pixel data lines (grayscale values 0-3)
- **LSYNC**: Line sync signal for horizontal timing
- **VSYNC**: Vertical sync signal for frame timing

#### 2. Data Processing
- PIO captures 2-bit pixels at 4.194304 MHz rate
- Each line contains 160 pixels (320 bits of data)
- Frame consists of 144 active lines
- Automatic synchronization with Game Boy timing

#### 3. Memory Management
- Double-buffered DMA for continuous capture
- 23,040 bytes per frame (160×144×1 byte per pixel)
- Automatic buffer switching for smooth operation
- Zero-copy transfer to display drivers

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
Connect the following Game Boy signals to Raspberry Pi Pico GPIO pins:

```cpp
// Default pin assignments (configurable in main.cpp)
#define GB_CLK_PIN    2   // Game Boy pixel clock (CPG)
#define GB_DATA0_PIN  3   // Game Boy data bit 0 (LD0)  
#define GB_DATA1_PIN  4   // Game Boy data bit 1 (LD1)
#define GB_HSYNC_PIN  5   // Game Boy horizontal sync (LSYNC)
#define GB_VSYNC_PIN  6   // Game Boy vertical sync (VSYNC)
```

### Software Integration

The PIO program is automatically compiled and included in your project:

```cpp
#include "gblcd.pio.h"

// Initialize PIO program
uint offset = pio_add_program(pio0, &gblcd_program);
gblcd_program_init(pio0, 0, offset, clk_pin, data_pin);
```

## Performance Features

- **Zero CPU Overhead**: PIO handles all timing-critical operations
- **DMA Integration**: Automatic memory transfers without CPU intervention
- **Real-time Capture**: No frame drops or synchronization issues
- **Low Latency**: Immediate processing suitable for gaming applications
- **Power Efficient**: Dedicated hardware reduces power consumption

## Applications

This library enables various Game Boy LCD capture applications:
- External display drivers (ST7789, ILI9341, ST7796)
- LCD replacement/upgrade projects
- Game Boy video recording systems
- Real-time streaming solutions
- Emulator development and testing

## Technical Notes

### Timing Requirements
- The PIO program is carefully tuned for Game Boy LCD timing
- Clock edges and data sampling are precisely aligned
- HSYNC and VSYNC detection ensures proper frame synchronization

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

