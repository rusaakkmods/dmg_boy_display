#!/bin/bash
# Build script for both v1.0 and v1.1 variants with configurable options

print_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --test              Enable display test mode"
    echo "  --dither-fast       Enable fast BW dithering"  
    echo "  --dither-best       Enable best quality BW dithering"
    echo "  --no-palette        Disable palette selection"
    echo "  --spi-40            Set SPI speed to 40MHz (stable)"
    echo "  --spi-62.5          Set SPI speed to 62.5MHz (fast, default)"
    echo "  --offset-x=N        X offset adjustment from base (47): -47 to +273"
    echo "  --offset-y=N        Y offset adjustment from base (2): -2 to +238"
    echo "  --clean             Clean build directory first"
    echo "  --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                      # Default build (62.5MHz SPI)"
    echo "  $0 --test               # Test mode build"
    echo "  $0 --spi-40             # Stable 40MHz SPI build"
    echo "  $0 --dither-fast        # Fast dither build"
    echo "  $0 --offset-x=-1        # Shift display left by 1 pixel"
    echo "  $0 --offset-y=3         # Shift display down by 3 pixels"
    echo "  $0 --offset-x=-5 --offset-y=2  # Both X and Y adjustments"
    echo "  $0 --clean --test       # Clean + test mode build"
    echo "  $0 --spi-40 --no-palette # Stable build, no palette selection"
}

# Parse command line arguments
CMAKE_ARGS=""
CLEAN_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --test)
            CMAKE_ARGS="$CMAKE_ARGS -DENABLE_DISPLAY_TEST=ON"
            shift
            ;;
        --dither-fast)
            CMAKE_ARGS="$CMAKE_ARGS -DENABLE_BW_DITHER=ON -DDITHER_MODE=FAST"
            shift
            ;;
        --dither-best)
            CMAKE_ARGS="$CMAKE_ARGS -DENABLE_BW_DITHER=ON -DDITHER_MODE=BEST"
            shift
            ;;
        --no-palette)
            CMAKE_ARGS="$CMAKE_ARGS -DDISABLE_PALETTE_SELECTION=ON"
            shift
            ;;
        --spi-40)
            CMAKE_ARGS="$CMAKE_ARGS -DSPI_SPEED=40"
            shift
            ;;
        --spi-62.5)
            CMAKE_ARGS="$CMAKE_ARGS -DSPI_SPEED=62.5"
            shift
            ;;
        --offset-x=*)
            OFFSET_X="${1#*=}"
            CMAKE_ARGS="$CMAKE_ARGS -DOFFSET_X_ADJUST=$OFFSET_X"
            shift
            ;;
        --offset-y=*)
            OFFSET_Y="${1#*=}"
            CMAKE_ARGS="$CMAKE_ARGS -DOFFSET_Y_ADJUST=$OFFSET_Y"
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

echo "Building DMG Boy Display - All Variants"
echo "======================================"

# Clean if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

mkdir -p build
cd build

# Configure and build
echo "Configuring project with options: $CMAKE_ARGS"
cmake .. -G Ninja $CMAKE_ARGS

echo "Building both variants..."
ninja

echo ""
echo "Build Results:"
echo "=============="
ls -la *.uf2 | grep -E "(v1_0|v1_1)"

echo ""
echo "Files ready for flashing:"
echo "  dmg_boy_display_v1_0.uf2   - For v1.0 hardware"
echo "  dmg_boy_display_v1_1.uf2   - For v1.1 hardware"