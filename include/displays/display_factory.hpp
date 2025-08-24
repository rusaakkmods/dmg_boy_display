#pragma once

#include "display_interface.hpp"
#include "st7789.hpp"
#include "ili9341.hpp"
#include <memory>

namespace display {

// Factory class to create display instances
class DisplayFactory {
public:
    // Create a display instance based on the configuration
    static std::unique_ptr<DisplayInterface> createDisplay(const Config& config) {
        switch (config.display_type) {
            case DISPLAY_ST7789: {
                auto display = std::make_unique<st7789::ST7789>();
                return display;
            }
            case DISPLAY_ILI9341: {
                auto display = std::make_unique<ili9341::ILI9341>();
                return display;
            }
            default:
                return nullptr;
        }
    }
    
    // Create a display instance with explicit type
    static std::unique_ptr<DisplayInterface> createST7789() {
        return std::make_unique<st7789::ST7789>();
    }
    
    static std::unique_ptr<DisplayInterface> createILI9341() {
        return std::make_unique<ili9341::ILI9341>();
    }
};

} // namespace display
