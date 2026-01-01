@echo off
REM Build script for both v1.0 and v1.1 variants with configurable options

setlocal enabledelayedexpansion

if "%1"=="--help" (
    echo Usage: %0 [options]
    echo.
    echo Options:
    echo   --test              Enable display test mode
    echo   --dither-fast       Enable fast BW dithering
    echo   --dither-best       Enable best quality BW dithering
    echo   --no-palette        Disable palette selection
    echo   --spi-40            Set SPI speed to 40MHz (stable)
    echo   --spi-62.5          Set SPI speed to 62.5MHz (fast, default)
    echo   --offset-x=N        X offset adjustment from base (47): -47 to +273
    echo   --offset-y=N        Y offset adjustment from base (2): -2 to +238
    echo   --clean             Clean build directory first
    echo   --help              Show this help
    echo.
    echo Examples:
    echo   %0                      # Default build (62.5MHz SPI)
    echo   %0 --test               # Test mode build
    echo   %0 --spi-40             # Stable 40MHz SPI build
    echo   %0 --dither-fast        # Fast dither build
    echo   %0 --offset-x=-1        # Shift display left by 1 pixel
    echo   %0 --offset-y=3         # Shift display down by 3 pixels
    echo   %0 --offset-x=-5 --offset-y=2  # Both X and Y adjustments
    echo   %0 --clean --test       # Clean + test mode build
    echo   %0 --spi-40 --no-palette # Stable build, no palette selection
    exit /b 0
)

set CMAKE_ARGS=
set CLEAN_BUILD=false

:parse_args
if "%1"=="" goto end_parse
if "%1"=="--test" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DENABLE_DISPLAY_TEST=ON
    shift & goto parse_args
)
if "%1"=="--dither-fast" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DENABLE_BW_DITHER=ON -DDITHER_MODE=FAST
    shift & goto parse_args
)
if "%1"=="--dither-best" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DENABLE_BW_DITHER=ON -DDITHER_MODE=BEST
    shift & goto parse_args
)
if "%1"=="--no-palette" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DDISABLE_PALETTE_SELECTION=ON
    shift & goto parse_args
)
if "%1"=="--spi-40" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DSPI_SPEED=40
    shift & goto parse_args
)
if "%1"=="--spi-62.5" (
    set CMAKE_ARGS=!CMAKE_ARGS! -DSPI_SPEED=62.5
    shift & goto parse_args
)
if "%1"=="--clean" (
    set CLEAN_BUILD=true
    shift & goto parse_args
)
REM Handle offset parameters with values
if "%~1" NEQ "" (
    echo %1 | findstr /C:"--offset-x=" >nul
    if !errorlevel! equ 0 (
        for /f "tokens=2 delims==" %%a in ("%1") do (
            set CMAKE_ARGS=!CMAKE_ARGS! -DOFFSET_X_ADJUST=%%a
        )
        shift & goto parse_args
    )
    echo %1 | findstr /C:"--offset-y=" >nul
    if !errorlevel! equ 0 (
        for /f "tokens=2 delims==" %%a in ("%1") do (
            set CMAKE_ARGS=!CMAKE_ARGS! -DOFFSET_Y_ADJUST=%%a
        )
        shift & goto parse_args
    )
)
echo Unknown option: %1
exit /b 1

:end_parse

echo Building DMG Boy Display - All Variants
echo ======================================

if "!CLEAN_BUILD!"=="true" (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
)

if not exist build mkdir build
cd build

echo Configuring project with options: !CMAKE_ARGS!
cmake .. -G Ninja !CMAKE_ARGS!

echo Building both variants...
ninja

echo.
echo Build Results:
echo ==============
dir /b *.uf2 | findstr /R "v1_[01]"

echo.
echo Files ready for flashing:
echo   dmg_boy_display_v1_0.uf2   - For v1.0 hardware
echo   dmg_boy_display_v1_1.uf2   - For v1.1 hardware