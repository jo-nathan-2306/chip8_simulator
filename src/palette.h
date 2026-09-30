#ifndef PALETTE_H
#define PALETTE_H

#include <cstdint>
#include <string>
#include <vector>

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct ColorPalette {
    std::string name;
    Color bg;
    Color fg;
};
inline const std::vector<ColorPalette> PALETTES = {
    {"Classic Monochrome", {0, 0, 0}, {255, 255, 255}},
    {"Classic Green Screen", {10, 26, 12}, {50, 255, 50}},
    {"Amber CRT", {28, 14, 0}, {255, 176, 0}},
    {"Neon High-Contrast", {15, 8, 28}, {0, 255, 235}},
    {"Cyberpunk Magenta", {20, 5, 25}, {255, 30, 140}},
    {"Game Boy LCD", {15, 56, 15}, {155, 188, 15}},
    {"Solarized Dark", {0, 43, 54}, {38, 139, 210}},
    {"Paper White", {245, 245, 240}, {20, 20, 20}}
};

#endif 
