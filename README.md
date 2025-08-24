# DMG Boy Display: Multi-Display Support for Game Boy LCD Capture

This project provides a complete Game Boy LCD capture and display system for the Raspberry Pi Pico, supporting ST7789, ILI9341, and ST7796 displays through a common interface.

## Architecture Overview

The multi-display support is implemented using the following architecture:

```
┌─────────────────────┐
│  Application Code   │
├─────────────────────┤
│ DisplayInterface    │  ← Common interface for all displays
├─────────────────────┤
│ DisplayFactory      │  ← Creates display instances
├─────────────────────┤
│   ST7789   │ ILI9341│  ← Specific display implementations
│   Driver   │ Driver │
└─────────────────────┘
```

### Key Components

1. **DisplayInterface** (`include/display_interface.hpp`): Common interface that all displays implement
2. **DisplayConfig** (`include/displays/display_config.hpp`): Common configuration structure
3. **DisplayFactory** (`include/displays/display_factory.hpp`): Factory pattern to create display instances
4. **ST7789 Driver**: Original ST7789 driver adapted to implement DisplayInterface
5. **ILI9341 Driver**: New ILI9341 driver implementing DisplayInterface

## Usage

### Quick Start

```cpp
#include "displays/display_factory.hpp"
#include "displays/display_config.hpp"

// Create configuration
display::Config config;

// For ST7789
config.display_type = display::DISPLAY_ST7789;
config.spi_speed_hz = 40 * 1000 * 1000;  // 40MHz

// OR for ILI9341
config = display::Config::createILI9341Config();

// Common pin configuration
config.spi_inst = spi0;
config.pin_din = 19;    // MOSI
config.pin_sck = 18;    // SCK
config.pin_cs = 17;     // CS
config.pin_dc = 20;     // DC
config.pin_reset = 15;  // RESET
config.pin_bl = 10;     // Backlight

// Create display using factory
auto lcd = display::DisplayFactory::createDisplay(config);

// Initialize and use
if (lcd->begin(config)) {
    lcd->fillScreen(display::BLACK);
    lcd->drawString(10, 10, "Hello World!", display::WHITE, display::BLACK, 2);
}
```

### Switching Between Displays

To switch between ST7789 and ILI9341, simply change the display type:

```cpp
// In main.cpp, uncomment the desired display:

#define USE_ST7789     // Use ST7789 display
// #define USE_ILI9341    // Use ILI9341 display
```

## Files Added/Modified

## Project Structure

### Main Files
- `main.cpp` - Main program supporting all display types
- `CMakeLists.txt` - Build configuration

### Display Headers (`include/displays/`)
- `display_config.hpp` - Common configuration structure
- `display_factory.hpp` - Factory for creating displays
- `display_interface.hpp` - Common display interface
- `st7789/` - ST7789 display driver files (st7789.hpp, st7789_hal.hpp, st7789_gfx.hpp, st7789_config.hpp)
- `ili9341/` - ILI9341 display driver files (ili9341.hpp, ili9341_hal.hpp, ili9341_gfx.hpp, ili9341_config.hpp)
- `st7796/` - ST7796 display driver files (st7796.hpp, st7796_hal.hpp, st7796_gfx.hpp, st7796_config.hpp)

### Display Implementations (`src/displays/`)
- `st7789/` - ST7789 implementation files (st7789.cpp, st7789_hal.cpp, st7789_gfx.cpp, st7789_font.cpp)
- `ili9341/` - ILI9341 implementation files (ili9341.cpp, ili9341_hal.cpp, ili9341_gfx.cpp)
- `st7796/` - ST7796 implementation files (st7796.cpp, st7796_hal.cpp, st7796_gfx.cpp)

### PIO Programs (`pio/`)
- `gblcd/` - Game Boy LCD capture PIO library
  - `gblcd.pio` - PIO program for Game Boy LCD data capture
  - `README.md` - Detailed documentation of the PIO program
- `src/ili9341_gfx.cpp` - ILI9341 graphics implementation

### New Example Files
- `main.cpp` - Main program supporting all display types

### Modified Files
- `include/st7789.hpp` - Updated to implement DisplayInterface
- `src/st7789.cpp` - Added wrapper for common config
- `CMakeLists.txt` - Added new libraries and targets

## Display-Specific Settings

### ST7789
- **Recommended SPI Speed**: 40MHz
- **Default Resolution**: 240x320
- **Initialization**: Requires specific command sequence for proper color and orientation

### ILI9341  
- **Recommended SPI Speed**: 25MHz
- **Default Resolution**: 240x320
- **Initialization**: Different command sequence with gamma correction

## Pin Configuration

Both displays use the same pin interface:

| Signal | Pin | Description |
|--------|-----|-------------|
| MOSI   | 19  | SPI Data Output |
| SCK    | 18  | SPI Clock |
| CS     | 17  | Chip Select |
| DC     | 20  | Data/Command |
| RESET  | 15  | Reset |
| BL     | 10  | Backlight |

## Building

The CMakeLists.txt now includes the main target:

```bash
# Build all targets
cmake --build build

# Main target:
# dmg_boy_display_demo - Game Boy LCD capture with multi-display support
```

## API Compatibility

The new architecture maintains full backward compatibility with existing ST7789 code while adding the new common interface. Existing code will continue to work without modification.

## Adding New Display Types

To add support for a new display type:

1. Create header files: `include/newdisplay.hpp`, `include/newdisplay_hal.hpp`, `include/newdisplay_gfx.hpp`
2. Implement the `DisplayInterface` in your new display class
3. Add the new display type to `display::DisplayType` enum in `display_config.hpp`
4. Update `DisplayFactory::createDisplay()` to handle the new type
5. Add source files and update `CMakeLists.txt`

## Performance Notes

- **DMA Support**: Both drivers support DMA for fast image transfers
- **Optimizations**: The scaling and image drawing code has been optimized for the Game Boy LCD capture use case
- **Memory Usage**: The DMG Boy Display demo uses ~101KB for frame buffers

## Troubleshooting

1. **Compilation Issues**: Ensure all new files are included in your build
2. **Display Not Working**: Check pin connections and SPI speed settings
3. **Wrong Colors**: Verify the display type matches your hardware
4. **Performance Issues**: Enable DMA and use appropriate SPI speeds

## Future Enhancements

- [ ] Add support for more display types (SSD1351, etc.)
- [ ] Implement PWM-based backlight control
- [ ] Add touch screen support for compatible displays
- [ ] Optimize font rendering system
- [ ] Add rotation-aware coordinate systems
