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
    {"classic monochrome", {0, 0, 0}, {255, 255, 255}},
    {"classic green screen", {10, 26, 12}, {50, 255, 50}},
    {"amber crt", {28, 14, 0}, {255, 176, 0}},
    {"neon high-contrast", {15, 8, 28}, {0, 255, 235}},
    {"cyberpunk magenta", {20, 5, 25}, {255, 30, 140}},
    {"game boy lcd", {15, 56, 15}, {155, 188, 15}},
    {"solarized dark", {0, 43, 54}, {38, 139, 210}},
    {"paper white", {245, 245, 240}, {20, 20, 20}}
};

#endif 
