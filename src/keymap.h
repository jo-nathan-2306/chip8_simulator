#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL2/SDL.h>
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <array>







struct KeyMapping {
    std::array<SDL_Keycode, 16> keys;
    int awaiting_rebind_idx = -1; 

    KeyMapping() {
        set_preset_qwerty();
        load_from_file("keymap.cfg");
    }

    void set_preset_qwerty() {
        keys[0x0] = SDLK_x;
        keys[0x1] = SDLK_1;
        keys[0x2] = SDLK_2;
        keys[0x3] = SDLK_3;
        keys[0x4] = SDLK_q;
        keys[0x5] = SDLK_w;
        keys[0x6] = SDLK_e;
        keys[0x7] = SDLK_a;
        keys[0x8] = SDLK_s;
        keys[0x9] = SDLK_d;
        keys[0xA] = SDLK_z;
        keys[0xB] = SDLK_c;
        keys[0xC] = SDLK_4;
        keys[0xD] = SDLK_r;
        keys[0xE] = SDLK_f;
        keys[0xF] = SDLK_v;
    }

    void set_preset_numpad() {
        keys[0x0] = SDLK_KP_0;
        keys[0x1] = SDLK_KP_7;
        keys[0x2] = SDLK_KP_8;
        keys[0x3] = SDLK_KP_9;
        keys[0x4] = SDLK_KP_4;
        keys[0x5] = SDLK_KP_5;
        keys[0x6] = SDLK_KP_6;
        keys[0x7] = SDLK_KP_1;
        keys[0x8] = SDLK_KP_2;
        keys[0x9] = SDLK_KP_3;
        keys[0xA] = SDLK_KP_PERIOD;
        keys[0xB] = SDLK_KP_ENTER;
        keys[0xC] = SDLK_KP_DIVIDE;
        keys[0xD] = SDLK_KP_MULTIPLY;
        keys[0xE] = SDLK_KP_MINUS;
        keys[0xF] = SDLK_KP_PLUS;
    }

    void set_preset_wasd() {
        
        keys[0x0] = SDLK_SPACE;
        keys[0x1] = SDLK_1;
        keys[0x2] = SDLK_w; 
        keys[0x3] = SDLK_3;
        keys[0x4] = SDLK_a; 
        keys[0x5] = SDLK_s; 
        keys[0x6] = SDLK_d; 
        keys[0x7] = SDLK_q;
        keys[0x8] = SDLK_x; 
        keys[0x9] = SDLK_e;
        keys[0xA] = SDLK_z;
        keys[0xB] = SDLK_c;
        keys[0xC] = SDLK_j;
        keys[0xD] = SDLK_k;
        keys[0xE] = SDLK_l;
        keys[0xF] = SDLK_i;
    }

    int get_chip8_key(SDL_Keycode sym) const {
        SDL_Keycode norm_sym = (sym >= 'A' && sym <= 'Z') ? (sym + 32) : sym;
        for (int i = 0; i < 16; i++) {
            SDL_Keycode k = (keys[i] >= 'A' && keys[i] <= 'Z') ? (keys[i] + 32) : keys[i];
            if (k == norm_sym) return i;
        }
        // Universal Arrow Keys support for directional scrolling and movement
        if (sym == SDLK_UP) return 0x2;     // Key 2 (Up / Scroll Up)
        if (sym == SDLK_DOWN) return 0x8;   // Key 8 (Down / Scroll Down)
        if (sym == SDLK_LEFT) return 0x4;   // Key 4 (Left / Scroll Left)
        if (sym == SDLK_RIGHT) return 0x6;  // Key 6 (Right / Scroll Right)
        return -1;
    }

    void rebind(int chip8_key, SDL_Keycode new_sym) {
        if (chip8_key >= 0 && chip8_key < 16) {
            keys[chip8_key] = new_sym;
            save_to_file("keymap.cfg");
        }
    }

    bool save_to_file(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return false;
        for (int i = 0; i < 16; i++) {
            out << i << "=" << static_cast<int>(keys[i]) << "\n";
        }
        return true;
    }

    bool load_from_file(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                try {
                    int idx = std::stoi(line.substr(0, eq));
                    int sym = std::stoi(line.substr(eq + 1));
                    if (idx >= 0 && idx < 16) {
                        keys[idx] = static_cast<SDL_Keycode>(sym);
                    }
                } catch (...) {}
            }
        }
        return true;
    }

    static std::string get_key_name(SDL_Keycode sym) {
        const char* name = SDL_GetKeyName(sym);
        if (name && name[0] != '\0') return std::string(name);
        return "Unknown";
    }
};

#endif 
