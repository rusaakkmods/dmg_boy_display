# rMODS DMG Boy Display

A high-performance Game Boy LCD capture and display system using the Raspberry Pi Pico.

## 🚨 Important: Hardware Version Compatibility

This project supports **two different hardware board versions** with different pin assignments. Using the wrong firmware will cause display issues or complete malfunction.

### Quick Hardware Identification

| Version | Game Boy Capture Pins | Display SPI | Controls |
|---------|----------------------|-------------|----------|
| **v1.0** | GPIO 2-5 | SPI1 (GPIO 9-13) | Single potentiometer |
| **v1.1a** | GPIO 9-12 | SPI0 (GPIO 3-7) | Potentiometer + Mode switch |

📋 **[Complete Hardware Guide](boards/README.md)** - Detailed board identification, pin assignments, and troubleshooting

### Firmware Files

The build system automatically generates firmware for both hardware versions:
- `dmg_boy_display_v1_0.uf2` - For v1.0 hardware
- `dmg_boy_display_v1_1a.uf2` - For v1.1a hardware

⚠️ **Always use the firmware that matches your hardware version!**

## Configuration Options

### CMake Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_DISPLAY_TEST` | OFF | Enable test pattern display instead of Game Boy capture |
| `ENABLE_BW_DITHER` | OFF | Enable black & white dithering |
| `DISABLE_PALETTE_SELECTION` | OFF | Disable palette selection (saves GPIO 29) |
| `DITHER_MODE` | FAST | Dithering algorithm: `FAST` or `BEST` |
| `SPI_SPEED` | 62.5 | SPI speed in MHz: `40` (stable) or `62.5` (fast) |
| `OFFSET_X_ADJUST` | 0 | X offset adjustment from base (47): -47 to +273 |
| `OFFSET_Y_ADJUST` | 0 | Y offset adjustment from base (2): -2 to +238 |


### Display Offset Configuration

**Adjust display position without modifying base values!**

The display offset system allows you to fine-tune the Game Boy screen position on your LCD display:

- **Base offsets**: X=47, Y=2 (optimized for most displays)
- **Adjustable range**: X: -47 to +273, Y: -2 to +238
- **Auto-clamping**: Values are automatically limited to valid screen area
- **Build-time configuration**: Set via CMake parameters

**Examples:**
```bash
# Shift display left by 1 pixel (final X offset: 47-1 = 46)
cmake -B build -DOFFSET_X_ADJUST=-1

# Shift display down by 3 pixels (final Y offset: 2+3 = 5)  
cmake -B build -DOFFSET_Y_ADJUST=3

# Both adjustments
cmake -B build -DOFFSET_X_ADJUST=-10 -DOFFSET_Y_ADJUST=5

# Large adjustments (will be auto-clamped to valid range)
cmake -B build -DOFFSET_X_ADJUST=-100 -DOFFSET_Y_ADJUST=50
```

The system calculates final offsets as: `Base + Adjustment`, then clamps to ensure the Game Boy screen stays within LCD bounds.

### Default Palette Configuration

**Default palette is now configured directly in `config.h` - no build options needed!**

To change the default palette, edit `include/config.h` and modify:
```c
#define DEFAULT_PALETTE_NAME GRAYSCALE  // Change to any palette name
```

### Available Palettes

| Palette Name | Description |
|--------------|-------------|
| `MODERN` | Modern color scheme |
| `ADVENTURER` | Adventure-themed colors |
| `SGB` | Super Game Boy colors |
| `LCD` | Classic Game Boy LCD green |
| `CLOUDY` | Cloudy sky tones |
| `VINTAGE` | Vintage sepia tones |
| `BLUE_HUE` | Blue color theme |
| `HIGHLIGHT_BLUE` | Highlighted blue theme |
| `NEON` | Neon bright colors |
| `PEACH` | Peach color tones |
| `MODERN2` | Alternative modern theme |
| `ROMANCE` | Romantic soft colors |
| `RETRO` | Retro color scheme |
| `GRAY_SHADES` | Gray color variations |
| `RED_PASTEL_SHADES` | Red pastel tones |
| `TEAL_SHADES` | Teal color variations |
| `YELLOW_SHADES` | Yellow color variations |
| `GREEN_SHADES` | Green color variations |
| `GRAYSCALE_INVERT` | Inverted grayscale |
| `GRAYSCALE` | Standard grayscale (**default**) |

### Adding/Removing Palettes

**Easy palette management:**
1. **Add new palette**: Define it in `palettes.hpp`, add to `PALETTE_LIST` array in `palettes.cpp`
2. **Remove palette**: Remove from both `palettes.hpp` and `PALETTE_LIST` array  
3. **Reorder palettes**: Change order in `PALETTE_LIST` array - potentiometer will follow new order
4. **Change default**: Edit `DEFAULT_PALETTE_NAME` in `config.h`

No need to rebuild with different options - just edit, compile, and flash!

**Example: Change default to LCD green:**
```c
// In include/config.h, change:
#define DEFAULT_PALETTE_NAME GRAYSCALE
// To:
#define DEFAULT_PALETTE_NAME LCD
```

### Build Methods

#### 1. VS Code Tasks
- **Ctrl+Shift+P** → Search for these tasks:
  - `Compile Project` - Build both variants (default options)
  - `Build v1.0 Only` - Build only v1.0 variant  
  - `Build v1.1a Only` - Build only v1.1a variant
  - `Configure Build - Test Mode` - Enable test mode
  - `Configure Build - BW Dither (Fast)` - Enable fast dithering
  - `Configure Build - BW Dither (Best Quality)` - Enable best quality dithering
  - `Configure Build - No Palette Selection` - Disable palette selection
  - `Configure Build - SPI 40MHz (Stable)` - Set SPI speed to 40MHz
  - `Configure Build - SPI 62.5MHz (Fast)` - Set SPI speed to 62.5MHz
  - `Configure Build - Default (Reset All Options)` - Reset to defaults

#### 2. Command Line (Manual)
```bash
# Default build
cmake -B build
ninja -C build

# Test mode build
cmake -B build -DENABLE_DISPLAY_TEST=ON
ninja -C build

# BW Dithering (fast)
cmake -B build -DENABLE_BW_DITHER=ON -DDITHER_MODE=FAST
ninja -C build

# BW Dithering (best quality)
cmake -B build -DENABLE_BW_DITHER=ON -DDITHER_MODE=BEST
ninja -C build

# Disable palette selection
cmake -B build -DDISABLE_PALETTE_SELECTION=ON
ninja -C build

# SPI speed configuration
cmake -B build -DSPI_SPEED=40        # 40MHz (stable)
ninja -C build

cmake -B build -DSPI_SPEED=62.5      # 62.5MHz (fast)
ninja -C build

# Display offset adjustment (shift display position)
cmake -B build -DOFFSET_X_ADJUST=-1     # X offset: 47-1 = 46 pixels
ninja -C build

cmake -B build -DOFFSET_Y_ADJUST=3      # Y offset: 2+3 = 5 pixels  
ninja -C build

cmake -B build -DOFFSET_X_ADJUST=-10 -DOFFSET_Y_ADJUST=5  # Both adjustments
ninja -C build

# Multiple options
cmake -B build -DENABLE_DISPLAY_TEST=ON -DDISABLE_PALETTE_SELECTION=ON -DSPI_SPEED=40
ninja -C build
```

#### 3. Build Scripts

**Linux/Mac (bash):**
```bash
./build_all.sh                          # Default build (62.5MHz SPI)
./build_all.sh --test                   # Test mode
./build_all.sh --dither-fast            # Fast BW dithering
./build_all.sh --dither-best            # Best quality BW dithering  
./build_all.sh --no-palette             # No palette selection
./build_all.sh --spi-40                 # 40MHz SPI (stable)
./build_all.sh --spi-62.5               # 62.5MHz SPI (fast, default)
./build_all.sh --offset-x=-1            # Shift display left by 1 pixel
./build_all.sh --offset-y=3             # Shift display down by 3 pixels
./build_all.sh --offset-x=-5 --offset-y=2  # Both X and Y adjustments
./build_all.sh --spi-40 --no-palette    # Stable build without palette selection
./build_all.sh --clean --test           # Clean + test mode
./build_all.sh --help                   # Show all options
```

**Windows (batch):**
```cmd
build_all.bat                           # Default build (62.5MHz SPI)
build_all.bat --test                    # Test mode
build_all.bat --dither-fast             # Fast BW dithering
build_all.bat --dither-best             # Best quality BW dithering
build_all.bat --no-palette              # No palette selection
build_all.bat --spi-40                  # 40MHz SPI (stable)
build_all.bat --spi-62.5                # 62.5MHz SPI (fast, default)
build_all.bat --offset-x=-1             # Shift display left by 1 pixel
build_all.bat --offset-y=3              # Shift display down by 3 pixels
build_all.bat --offset-x=-5 --offset-y=2  # Both X and Y adjustments
build_all.bat --spi-40 --no-palette     # Stable build without palette selection
build_all.bat --clean --test            # Clean + test mode
build_all.bat --help                    # Show all options
```

## Feature Descriptions

### Display Test Mode (`ENABLE_DISPLAY_TEST=ON`)
- Shows test patterns instead of capturing Game Boy video
- Useful for testing display functionality without Game Boy hardware
- Cycles through gradient and checkerboard patterns
- Allows testing palette switching and brightness controls

### BW Dithering (`ENABLE_BW_DITHER=ON`)
- Converts color output to black & white with dithering
- **FAST mode**: Uses Bayer dithering (faster, lower quality)
- **BEST mode**: Uses Floyd-Steinberg error diffusion (slower, better quality)

### Disable Palette Selection (`DISABLE_PALETTE_SELECTION=ON`)  
- Disables ADC-based palette switching
- Frees up GPIO 29 for other uses
- Uses fixed palette defined by `DEFAULT_PALETTE_NAME` in config.h
- Useful for simplified v1.0 builds

### SPI Speed Selection (`SPI_SPEED`)
- **40MHz**: More stable/reliable operation, better for production or problematic hardware
- **62.5MHz**: Maximum performance when hardware can handle it reliably (default)
- Both v1.0 and v1.1a variants use the same SPI speed setting
- Lower speeds can help resolve display issues or signal integrity problems

## Workflow Examples

### Development Testing
```bash
# Test display functionality
./build_all.sh --test
# Flash dmg_boy_display_v1_1a.uf2 to test hardware

# Test BW dither quality
./build_all.sh --dither-best
# Compare with --dither-fast
```

### Production Builds
```bash  
# Standard production build (62.5MHz SPI)
./build_all.sh

# Stable production build (40MHz SPI)
./build_all.sh --spi-40

# Simplified v1.0 build (no palette selection)
./build_all.sh --no-palette
# Use dmg_boy_display_v1_0.uf2 for simplified hardware

# Ultra-stable build (40MHz SPI + no palette selection)
./build_all.sh --spi-40 --no-palette
```

### Clean Development
```bash
# Fresh build with new options
./build_all.sh --clean --test --no-palette

# Test different SPI speeds if experiencing display issues
./build_all.sh --clean --spi-40 --test    # Test with stable SPI speed
./build_all.sh --clean --spi-62.5 --test  # Test with fast SPI speed
```

### Troubleshooting Display Issues

#### Wrong Hardware/Firmware Combination
**Symptoms**: Display shows garbage, wrong rotation, controls don't work
**Solution**: 
1. Check your hardware version using the [Hardware Guide](boards/README.md)
2. Flash the correct firmware file for your board

#### Display Artifacts/Performance Issues
If you experience display corruption, flickering, or other visual artifacts:

1. **Try stable SPI speed first**:
   ```bash
   ./build_all.sh --spi-40
   ```

2. **Combine with simplified configuration**:
   ```bash
   ./build_all.sh --spi-40 --no-palette
   ```

3. **Use test mode to isolate hardware issues**:
   ```bash
   ./build_all.sh --spi-40 --test
   ```

## Documentation

- 📋 **[Hardware Guide](boards/README.md)** - Board versions, pin assignments, hardware identification
- 🎯 **[PIO Documentation](pio/gblcd/README.md)** - Game Boy capture implementation details

The build system automatically handles all source file compilation and creates both hardware variants with the selected options.