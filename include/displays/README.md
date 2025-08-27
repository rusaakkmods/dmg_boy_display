# Display Drivers

This folder contains display driver headers for supported TFT displays in the DMG Boy Display project.

## Supported Displays

- **ST7789** (240x240)
- **ILI9341** (240x320)
- **ST7796** (320x480)
- **ILI9342** (240x320) ← *New!*

Each display has its own subfolder with configuration, graphics, and hardware abstraction headers.

## Adding a New Display

1. Create a new subfolder (e.g., `ili9342/`).
2. Add the required header files: `*_config.hpp`, `*_gfx.hpp`, `*_hal.hpp`, and the main `*.hpp`.
3. Implement the corresponding source files in `src/displays/<display>/`.
4. Update `main.cpp` and `CMakeLists.txt` to include the new display.

## Dithering Mode for Monochrome Displays

A dithering mode is available for monochrome (1-bit) display output. This can be enabled in the display configuration to simulate grayscale using ordered dithering patterns, improving the visual quality on black-and-white screens.

---

*See each display's subfolder for specific configuration options and usage details.*
