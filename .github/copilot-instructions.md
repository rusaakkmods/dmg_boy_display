# DMG Boy Display - AI Agent Instructions

## Project Overview

This is a **Raspberry Pi Pico (RP2040) firmware project** that captures Game Boy (DMG) LCD output in real-time and displays it on modern LCD screens. The system uses PIO (Programmable I/O) for high-speed capture and supports multiple display drivers.

**Critical Context**: This project supports **two incompatible hardware versions** (v1.0 and v1.1a) with different GPIO pin assignments. The build system automatically generates separate firmware binaries for each version.

## Architecture

### Core Data Flow
```
Game Boy LCD → PIO Capture (gblcd.pio) → Frame Buffer → Scaler → Dithering (optional) → Display Driver (SPI)
```

1. **PIO Capture** ([pio/gblcd/gblcd.pio](pio/gblcd/gblcd.pio)): Single state machine captures 4 signals (CPG clock, LD0, LD1, VSYNC) at 4.194304 MHz
2. **Frame Assembly** ([main.cpp](main.cpp)): CPU reads PIO FIFO and builds 160x144 frame buffer with palette mapping
3. **Scaling** ([src/scaler.cpp](src/scaler.cpp)): Nearest-neighbor scaling (1.6x) to 256x230 using pre-computed lookup maps
4. **Dithering** (optional, [src/dither.cpp](src/dither.cpp)): Floyd-Steinberg or Atkinson algorithms for B&W output
5. **Display** ([src/displays/](src/displays/)): SPI transfer to ILI9341/ST7789/etc. at 40-62.5 MHz

### Hardware Version System

**Pin assignments are version-specific and compile-time defined:**
- v1.0: Game Boy on GPIO 2-5, Display on SPI1 (GPIO 9-13), single potentiometer
- v1.1a: Game Boy on GPIO 9-12, Display on SPI0 (GPIO 3-7), potentiometer + mode switch

CMake builds both variants automatically with `-DVERSION_V1_0` and `-DVERSION_V1_1a` preprocessor flags.

See [boards/README.md](boards/README.md) for complete pin mappings and [include/config.h](include/config.h) for conditional compilation.

## Build System

### CMake Configuration Options

All features are controlled via CMake options, **not** by editing source files:

| Option | Values | Description |
|--------|--------|-------------|
| `ENABLE_DISPLAY_TEST` | ON/OFF | Test patterns instead of Game Boy capture |
| `ENABLE_BW_DITHER` | ON/OFF | Black & white dithering mode |
| `DITHER_MODE` | FAST/BEST | Floyd-Steinberg (FAST) vs Atkinson (BEST) |
| `SPI_SPEED` | 40/62.5 | SPI clock in MHz (40=stable, 62.5=fast) |
| `OFFSET_X_ADJUST` | -47 to +273 | X position adjustment from base (47) |
| `OFFSET_Y_ADJUST` | -2 to +238 | Y position adjustment from base (2) |
| `DISABLE_PALETTE_SELECTION` | ON/OFF | Disable ADC/palette switching (saves GPIO 29) |

### Build Workflow

**Using VS Code Tasks** (preferred):
- `Ctrl+Shift+P` → "Tasks: Run Task" → Select from:
  - `Compile Project` - Builds both v1.0 and v1.1a with current options
  - `Build v1.0 Only` / `Build v1.1a Only` - Single-variant builds
  - `Configure Build - [Option]` - Reconfigure CMake with specific options

**Using build scripts**:
```bash
# Linux/Mac
./build_all.sh --dither-fast --spi-40 --offset-x=-1

# Windows
build_all.bat --dither-fast --spi-40 --offset-x=-1
```

**Manual CMake**:
```bash
cmake -B build -DENABLE_BW_DITHER=ON -DDITHER_MODE=FAST -DSPI_SPEED=40
ninja -C build  # or: %USERPROFILE%\.pico-sdk\ninja\v1.12.1\ninja.exe -C build
```

Output: `build/dmg_boy_display_v1_0.uf2` and `build/dmg_boy_display_v1_1a.uf2`

## Project-Specific Conventions

### Configuration Philosophy
- **Never hardcode in source**: All tunable parameters live in [include/config.h](include/config.h) as macros
- **Build-time selection**: Features toggle via CMake options (converted to preprocessor flags)
- **Hardware abstraction**: Use `#ifdef VERSION_V1_0` / `VERSION_V1_1a` for pin assignments

### Palette System
Default palette is set in [config.h](include/config.h):
```c
#define DEFAULT_PALETTE_NAME GRAYSCALE  // Change to LCD, SGB, MODERN, etc.
```

To add/remove palettes:
1. Define array in [include/palettes.hpp](include/palettes.hpp)
2. Add pointer to `PALETTE_LIST[]` in [src/palettes.cpp](src/palettes.cpp)
3. Runtime switching via ADC (GPIO 29) reads palette index from `PALETTE_LIST`

### Display Driver Pattern
All display drivers follow this structure ([src/displays/](src/displays/)):
```
displays/<driver>/
├── <driver>.cpp       - Core driver implementation
├── <driver>.hpp       - Public API (begin, drawImage, etc.)
├── <driver>_hal.cpp   - Hardware abstraction (SPI, GPIO)
└── <driver>_gfx.cpp   - Graphics primitives (optional)
```

Drivers implement a common interface (see ILI9341 as reference).

### Memory Management
- **Static buffers**: All frame buffers are static arrays to avoid heap fragmentation
  ```cpp
  static uint16_t screenBuffer[DMG_W * DMG_H];      // 160x144
  static uint16_t scaledBuf[SCALED_W * SCALED_H];   // 256x230
  static int xmap[SCALED_W], ymap[SCALED_H];        // Scale lookup tables
  ```
- **Single-pass processing**: Capture → scale → dither → transmit in one frame interval

### PIO Integration
The PIO program ([pio/gblcd/gblcd.pio](pio/gblcd/gblcd.pio)) is **extremely simple** (6 lines):
- Waits for clock edge on pin 0 (CPG)
- Reads 4 pins simultaneously (CPG, LD0, LD1, VSYNC)
- Pushes to FIFO for CPU consumption

**Do not modify unless changing capture pin count**. Generated header is `build/gblcd.pio.h`.

## Common Tasks

### Adding a New Display Driver
1. Create driver directory: `src/displays/<driver>/` and `include/displays/<driver>/`
2. Implement `begin()`, `setRotation()`, `drawImage()`, `clearScreen()`, `setBrightness()`
3. Update [main.cpp](main.cpp) to instantiate new driver (replace `ili9341::ILI9341`)
4. Add source files to `COMMON_SOURCES` in [CMakeLists.txt](CMakeLists.txt)

### Adjusting Display Positioning
**Do not modify `X_OFF_BASE` or `Y_OFF_BASE` in [config.h](include/config.h)**. Use CMake options:
```bash
cmake -B build -DOFFSET_X_ADJUST=5 -DOFFSET_Y_ADJUST=-2
```
Final offsets are auto-clamped to screen bounds.

### Debugging Capture Issues
1. Enable test mode: `cmake -B build -DENABLE_DISPLAY_TEST=ON`
2. Verify test patterns display correctly (rules out display/SPI issues)
3. Check PIO FIFO overruns (means CPU processing is too slow)
4. Verify GPIO pin assignments match hardware version in [boards/README.md](boards/README.md)

### Optimizing Performance
- **SPI speed**: Try 62.5 MHz first (`-DSPI_SPEED=62.5`), fall back to 40 MHz if artifacts appear
- **Dithering**: `DITHER_MODE=FAST` uses Floyd-Steinberg (faster), `BEST` uses Atkinson (higher quality)
- **Scaling**: Pre-computed maps in `buildScaleMaps()` avoid floating-point math in frame loop

## Dependencies

- **Pico SDK 2.2.0**: Managed by VS Code extension (`~/.pico-sdk/`)
- **Build tools**: 
  - Ninja 1.12.1 (Windows: `%USERPROFILE%\.pico-sdk\ninja\v1.12.1\ninja.exe`)
  - CMake 3.13+
  - ARM GCC 14_2_Rel1
- **No external libraries**: All display drivers are custom implementations

## Gotchas

1. **Wrong firmware on hardware**: Using v1.0 firmware on v1.1a (or vice versa) causes display corruption or no output
2. **Offset overflow**: Setting `OFFSET_X_ADJUST` too large silently clamps - check calculated `X_OFF` in [config.h](include/config.h)
3. **ADC noise**: Palette switching reads GPIO 29 ADC 8 times and averages (see [src/helpers.cpp](src/helpers.cpp))
4. **Backlight control differs**: v1.0 uses GPIO on/off, v1.1a uses PWM via driver's `setBrightness()`
5. **PIO pin base**: Changing `GB_PIN_BASE` requires pins to be sequential (0-3 relative to base)

## Testing

No automated test suite. Manual testing workflow:
1. Build with `ENABLE_DISPLAY_TEST=ON`
2. Flash to Pico (drag UF2 to RPI-RP2 drive or use `Run Project` task)
3. Verify 6 test patterns cycle every second
4. Build normal firmware and test with actual Game Boy

## Key Files Reference

- [main.cpp](main.cpp) - Main capture loop, PIO initialization
- [include/config.h](include/config.h) - **All configuration constants**
- [CMakeLists.txt](CMakeLists.txt) - Build options, target definitions
- [pio/gblcd/gblcd.pio](pio/gblcd/gblcd.pio) - PIO capture program
- [boards/README.md](boards/README.md) - Hardware version identification
- [src/scaler.cpp](src/scaler.cpp) - Nearest-neighbor scaling implementation
- [src/palettes.cpp](src/palettes.cpp) - Palette definitions and management
