#ifndef GUI_H
#define GUI_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <algorithm>
#include "font.h"
#include "palette.h"

struct Button {
    std::string id;
    SDL_Rect rect;
    std::string label;
    bool hovered = false;
    bool pressed = false;

    bool contains(int x, int y) const {
        return (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h);
    }

    void draw(SDL_Renderer* renderer, SDL_Color custom_border = {0, 0, 0, 0}) const {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        if (pressed) {
            SDL_SetRenderDrawColor(renderer, 20, 28, 45, 240);
        } else if (hovered) {
            SDL_SetRenderDrawColor(renderer, 55, 68, 95, 240);
        } else {
            SDL_SetRenderDrawColor(renderer, 35, 42, 60, 230);
        }
        SDL_RenderFillRect(renderer, &rect);

        
        if (custom_border.a > 0) {
            SDL_SetRenderDrawColor(renderer, custom_border.r, custom_border.g, custom_border.b, custom_border.a);
        } else if (hovered) {
            SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 70, 82, 110, 200);
        }
        SDL_RenderDrawRect(renderer, &rect);

        
        int text_w = get_string_width(label, 1);
        int text_x = rect.x + std::max(2, (rect.w - text_w) / 2);
        int text_y = rect.y + std::max(2, (rect.h - 7) / 2);
        SDL_Color text_color = hovered ? SDL_Color{255, 255, 255, 255} : SDL_Color{220, 230, 245, 240};
        draw_string(renderer, label, text_x, text_y, text_color, 1);
    }
};

struct ToastNotification {
    std::string message;
    Uint32 expire_time = 0;
    bool is_error = false;

    void set(const std::string& msg, Uint32 duration_ms = 2000, bool error = false) {
        message = msg;
        expire_time = SDL_GetTicks() + duration_ms;
        is_error = error;
    }

    bool is_active() const {
        return SDL_GetTicks() < expire_time && !message.empty();
    }

    void draw(SDL_Renderer* renderer, int window_width) const {
        if (!is_active()) return;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        int text_w = get_string_width(message, 1);
        int box_w = text_w + 24;
        int box_h = 24;
        int box_x = (window_width - box_w) / 2;
        int box_y = 12;

        SDL_Rect bg_rect = {box_x, box_y, box_w, box_h};
        
        SDL_SetRenderDrawColor(renderer, 12, 16, 26, 230);
        SDL_RenderFillRect(renderer, &bg_rect);

        
        if (is_error) {
            SDL_SetRenderDrawColor(renderer, 255, 75, 75, 240);
        } else {
            SDL_SetRenderDrawColor(renderer, 0, 255, 200, 240);
        }
        SDL_RenderDrawRect(renderer, &bg_rect);

        
        int text_x = box_x + 12;
        int text_y = box_y + 8;
        SDL_Color text_color = is_error ? SDL_Color{255, 120, 120, 255} : SDL_Color{230, 255, 250, 255};
        draw_string(renderer, message, text_x, text_y, text_color, 1);
    }
};

inline void draw_pause_indicator(SDL_Renderer* renderer, int x, int y) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    
    SDL_Rect badge = {x, y, 76, 22};
    SDL_SetRenderDrawColor(renderer, 15, 20, 30, 200);
    SDL_RenderFillRect(renderer, &badge);
    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 220);
    SDL_RenderDrawRect(renderer, &badge);

    
    SDL_Rect bar1 = {x + 8, y + 5, 4, 12};
    SDL_Rect bar2 = {x + 15, y + 5, 4, 12};
    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 255);
    SDL_RenderFillRect(renderer, &bar1);
    SDL_RenderFillRect(renderer, &bar2);

    
    draw_string(renderer, "PAUSED", x + 25, y + 7, {255, 215, 60, 255}, 1);
}

inline void draw_help_modal(SDL_Renderer* renderer, int win_w, int win_h) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Rect backdrop = {0, 0, win_w, win_h};
    SDL_SetRenderDrawColor(renderer, 5, 8, 15, 215);
    SDL_RenderFillRect(renderer, &backdrop);

    int modal_w = 580;
    int modal_h = 326;
    int modal_x = (win_w - modal_w) / 2;
    int modal_y = (win_h - modal_h) / 2;

    SDL_Rect modal_rect = {modal_x, modal_y, modal_w, modal_h};
    SDL_SetRenderDrawColor(renderer, 20, 24, 34, 250);
    SDL_RenderFillRect(renderer, &modal_rect);

    SDL_SetRenderDrawColor(renderer, 0, 230, 210, 255);
    SDL_RenderDrawRect(renderer, &modal_rect);

    SDL_Rect header_rect = {modal_x, modal_y, modal_w, 24};
    SDL_SetRenderDrawColor(renderer, 30, 40, 58, 255);
    SDL_RenderFillRect(renderer, &header_rect);
    SDL_SetRenderDrawColor(renderer, 0, 230, 210, 255);
    SDL_RenderDrawLine(renderer, modal_x, modal_y + 24, modal_x + modal_w, modal_y + 24);

    draw_string(renderer, "CHIP-8 Emulator  |  Controls & Features", modal_x + 12, modal_y + 8, {0, 255, 220, 255}, 1);
    draw_string(renderer, "[X] / ESC to close", modal_x + modal_w - 120, modal_y + 8, {180, 200, 220, 255}, 1);

    int cur_y = modal_y + 32;

    auto print_section = [&](const std::string& title) {
        draw_string(renderer, title, modal_x + 14, cur_y, {255, 215, 60, 255}, 1);
        cur_y += 12;
    };

    auto print_item = [&](const std::string& key, const std::string& desc) {
        draw_string(renderer, key, modal_x + 22, cur_y, {100, 210, 255, 255}, 1);
        draw_string(renderer, ": " + desc, modal_x + 155, cur_y, {210, 220, 235, 255}, 1);
        cur_y += 11;
    };

    print_section("-- Emulation Speed --");
    print_item("[+] / [UP] / [ ] ]", "Increase speed (+60 Hz / +1 cycle per frame)");
    print_item("[-] / [DOWN] / [ [ ]", "Decrease speed (-60 Hz / -1 cycle per frame)");
    print_item("[PageUp] / [PageDown]", "Fast speed change (+/- 300 Hz / +/- 5 cycles)");
    print_item("[0] / [Backspace]", "Reset speed to standard 600 Hz (10 cycles/frame)");
    print_item("[Space] / [P]", "Toggle Pause / Resume");
    print_item("[.] / [N]", "Step single frame (when paused)");
    cur_y += 3;

    print_section("-- Color Schemes --");
    print_item("[Tab] / [T]", "Next retro palette (Green CRT, Amber, Neon, etc.)");
    print_item("[Shift + Tab]", "Previous retro palette");
    cur_y += 3;

    print_section("-- Savestate Management --");
    print_item("[F5] / [Ctrl + S]", "Save emulator state to current slot file");
    print_item("[F6] / [F8] / [Ctrl+L]", "Load emulator state from current slot file");
    print_item("[F7]", "Cycle savestate slot (slots 1 to 5)");
    cur_y += 3;

    print_section("-- CHIP-8 Keypad Mapping --");
    draw_string(renderer, "CHIP-8 Keypad:  1 2 3 C   4 5 6 D   7 8 9 E   A 0 B F", modal_x + 22, cur_y, {160, 180, 200, 255}, 1);
    cur_y += 11;
    draw_string(renderer, "PC Keyboard:    1 2 3 4   Q W E R   A S D F   Z X C V", modal_x + 22, cur_y, {100, 230, 160, 255}, 1);
    cur_y += 14;

    SDL_SetRenderDrawColor(renderer, 50, 60, 80, 200);
    SDL_RenderDrawLine(renderer, modal_x + 10, cur_y, modal_x + modal_w - 10, cur_y);
    cur_y += 6;
    draw_string(renderer, "* All buttons on the bottom toolbar are mouse-clickable!", modal_x + 22, cur_y, {200, 210, 225, 240}, 1);
}

#endif 
