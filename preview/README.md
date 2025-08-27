# DMG Boy Display - Preview Gallery

This folder contains preview images and videos of the DMG Boy Display project in action with different TFT displays.

## 📸 Preview Images

### ST7789 Display (240x240)
![ST7789 Display](st7789_cs.jpg)
*ST7789 240x240 display showing Game Boy content with 1.5x scaling*

### ILI9341 Display (240x320)
![ILI9341 Display](ili9341.jpg)
*ILI9341 240x320 display showing Game Boy content with 1.5x scaling*


### ILI9342 Display (240x320) *(New!)*
![ILI9342 Display](ili9342.jpg)
*ILI9342 240x320 display showing Game Boy content with 1.5x scaling*

### ST7796 Display (320x480)
![ST7796 Display](st7796.jpg)
*ST7796 320x480 display showing Game Boy content with 2x scaling*

### RP2040-Zero Compact Build
![RP2040-Zero Build](rp2040-Zero.jpg)
*Compact build using Waveshare RP2040-Zero for minimal footprint*

## 🎥 Video Demo
- `preview.mp4` - Video demonstration of the system in action
- `rp2040-Zero_preview.mp4` - RP2040-Zero compact build demonstration

## 📱 Follow for More Projects

**Follow me on Instagram for more retro gaming mods and electronics projects:**

**[@rusaakkmods](https://www.instagram.com/rusaakkmods/)**

🔥 Don't forget to **like** and **follow** for:
- Behind-the-scenes development
- New project updates
- Retro gaming modifications
- Electronics tutorials
- Hardware hacks

## 🛠️ Hardware Specifications

All images show the same Game Boy LCD capture system with different configurations:

### Standard Pico Build
- **Raspberry Pi Pico** (RP2040)
- **Real-time PIO capture** from Game Boy DMG-01
- **Hardware DMA acceleration** for smooth frame rates
- **Authentic Game Boy color palettes**

### Compact RP2040-Zero Build  
- **Waveshare RP2040-Zero** (23.5×18mm)
- **60% smaller** than standard Pico
- **Same performance** and capabilities
- **Ideal for portable/embedded applications**

## 📐 Display Comparisons

| Display  | Resolution | Scaling | Position | Performance |
|----------|------------|---------|----------|-------------|
| ST7789   | 240x240    | 1.5x    | Centered | 40MHz SPI   |
| ILI9341  | 240x320    | 1.5x    | Top      | 40MHz SPI   |
| ILI9342  | 240x320    | 1.5x    | Top      | 40MHz SPI   |
| ST7796   | 320x480    | 2.0x    | Top      | 62.5MHz SPI |

---


## Black & White (Monochrome) Feature for OLED/Monochrome Displays

A new black-and-white mode is now available for future OLED and other monochrome (1-bit) display support. This mode uses a simple, fast 8x8 Bayer ordered dithering algorithm (instead of the more CPU-intensive Floyd-Steinberg method) to simulate grayscale on black-and-white screens. This approach is chosen for its efficiency and suitability for microcontrollers, making it ideal for OLED and similar displays. Enable this in the display configuration for best results on monochrome hardware.

> **Note:** The Bayer dither is much less CPU-intensive than Floyd-Steinberg, making it a better fit for real-time applications on the RP2040.

---

**Project by [@rusaakkmods](https://www.instagram.com/rusaakkmods/)**
