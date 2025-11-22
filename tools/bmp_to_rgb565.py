#!/usr/bin/env python3
"""
Convert BMP image to C/C++ RGB565 array header file
Usage: python bmp_to_rgb565.py input.bmp output.h
"""

import sys
from PIL import Image
import os

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 to RGB565 format"""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    return (r5 << 11) | (g6 << 5) | b5

def generate_header(bmp_path, output_path=None):
    """Generate C header file from BMP image"""
    
    # Load image
    try:
        img = Image.open(bmp_path)
    except Exception as e:
        print(f"Error loading image: {e}")
        return False
    
    # Convert to RGB if necessary
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    width, height = img.size
    pixels = img.load()
    
    # Generate output filename if not provided
    if output_path is None:
        base_name = os.path.splitext(os.path.basename(bmp_path))[0]
        output_path = f"{base_name}.h"
    
    # Determine variable name from filename
    base_name = os.path.splitext(os.path.basename(bmp_path))[0]
    var_name = base_name.replace('-', '_').replace(' ', '_')
    
    # Detect background color (use top-left pixel)
    bg_r, bg_g, bg_b = pixels[0, 0]
    bg_color = rgb888_to_rgb565(bg_r, bg_g, bg_b)
    
    # Generate header file
    with open(output_path, 'w') as f:
        # Write header comment
        f.write(f"// Generated from {os.path.basename(bmp_path)}\n")
        f.write(f"// Image size: {width}x{height} pixels\n")
        f.write(f"// Color format: RGB565\n\n")
        
        # Write header guard and includes
        f.write("#pragma once\n")
        f.write("#include <cstdint>\n\n")
        
        # Write size macros
        f.write(f"#define {var_name.upper()}_WIDTH {width}\n")
        f.write(f"#define {var_name.upper()}_HEIGHT {height}\n")
        f.write(f"#define {var_name.upper()}_BACKGROUND 0x{bg_color:04X}\n\n")
        
        # Write pixel data array
        f.write(f"static const uint16_t {var_name}_data[] = {{\n")
        
        # Convert and write pixel data
        for y in range(height):
            f.write("    ")
            for x in range(width):
                r, g, b = pixels[x, y][:3]  # Get RGB, ignore alpha if present
                rgb565 = rgb888_to_rgb565(r, g, b)
                
                f.write(f"0x{rgb565:04X}")
                
                # Add comma except for last pixel
                if not (y == height - 1 and x == width - 1):
                    f.write(",")
                
                # Add space between values
                if x < width - 1:
                    f.write(" ")
            
            f.write("\n")
        
        f.write("};\n")
    
    print(f"Successfully generated {output_path}")
    print(f"Image size: {width}x{height}")
    print(f"Total pixels: {width * height}")
    print(f"Background color: 0x{bg_color:04X}")
    
    return True

def main():
    if len(sys.argv) < 2:
        print("Usage: python bmp_to_rgb565.py input.bmp [output.h]")
        print("\nExample:")
        print("  python bmp_to_rgb565.py logo.bmp logo.h")
        print("  python bmp_to_rgb565.py my_image.bmp  (generates my_image.h)")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found")
        sys.exit(1)
    
    success = generate_header(input_file, output_file)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
