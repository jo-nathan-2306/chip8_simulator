#include "chip8.h"
#include "palette.h"
#include "audio.h"
#include "rom_browser.h"
#include "keymap.h"
#include "disassembler.h"
#include "debugger.h"
#include "editor_ui.h"
#include "keymap_ui.h"
#include "imgui_overlay.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

const int BASE_DISPLAY_WIDTH = 64;
const int BASE_DISPLAY_HEIGHT = 32;
const int DEFAULT_SCALE = 10; 
const int WINDOW_WIDTH = 1030;
const int WINDOW_HEIGHT = 680;


void update_window_title(SDL_Window* window, int cycles_per_frame, bool paused, const std::string& palette_name, int slot, bool is_schip) {
    int hz = cycles_per_frame * 60;
    std::string mode_str = is_schip ? "schip 128x64 | " : "64x32 | ";
    std::string title = "chip-8 emulator [" + mode_str + (paused ? std::string("paused | ") : "") +
                        std::to_string(hz) + " hz (" + std::to_string(cycles_per_frame) + " c/f) | " +
                        palette_name + " | slot " + std::to_string(slot) + " | f2: controls | f1: help]";
    SDL_SetWindowTitle(window, title.c_str());
}


void draw_graphics(SDL_Renderer* renderer, const Chip8& chip8, const ColorPalette& palette, int start_x, int start_y, int base_scale) {
    int dw = chip8.get_display_width();
    int dh = chip8.get_display_height();

    
    int pixel_scale = chip8.is_extended_mode() ? (base_scale / 2) : base_scale;
    if (pixel_scale < 1) pixel_scale = 1;

    int screen_w = dw * pixel_scale;
    int screen_h = dh * pixel_scale;

    
    SDL_Rect game_area = {start_x, start_y, screen_w, screen_h};
    SDL_SetRenderDrawColor(renderer, palette.bg.r, palette.bg.g, palette.bg.b, 255);
    SDL_RenderFillRect(renderer, &game_area);

    
    SDL_SetRenderDrawColor(renderer, palette.fg.r, palette.fg.g, palette.fg.b, 255);
    for (int y = 0; y < dh; y++) {
        for (int x = 0; x < dw; x++) {
            if (chip8.display[x + (y * dw)] == 1) {
                SDL_Rect rect = {start_x + x * pixel_scale, start_y + y * pixel_scale, pixel_scale, pixel_scale};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    
    SDL_SetRenderDrawColor(renderer, 50, 65, 85, 255);
    SDL_RenderDrawRect(renderer, &game_area);
}

int main(int argc, char** argv) {
    std::string rom_filename = "";
    if (argc >= 2) {
        rom_filename = argv[1];
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "sdl error: " << SDL_GetError() << std::endl;
        return 1;
    }

    
    AudioConfig audio_config;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &audio_config;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        std::cerr << "failed to open audio: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    SDL_Window* window = SDL_CreateWindow(
        "chip-8 / super-chip emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "window error: " << SDL_GetError() << std::endl;
        if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
        SDL_Quit();
        return 1;
    }

    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer) {
        std::cerr << "renderer error: " << SDL_GetError() << std::endl;
        if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Chip8 chip8;
    RomBrowserState rom_browser_state;
    KeyMapping keymap;
    DebuggerState debugger_state;
    CodeEditorState editor_state;
    KeymapUIState keymap_ui_state;

    
    int cycles_per_frame = 10; 
    bool paused = false;
    bool step_one_frame = false;
    size_t current_palette_idx = 0;
    ColorPalette active_palette = PALETTES[0];
    int current_slot = 1; 
    OverlayState overlay_state;
    bool rom_load_requested = false;

    if (!rom_filename.empty()) {
        if (chip8.load_rom(rom_filename)) {
            rom_browser_state.add_recent(rom_filename);
            overlay_state.set_status("loaded rom: " + fs::path(rom_filename).filename().string());
        } else {
            overlay_state.set_status("failed to load initial rom: " + rom_filename, true);
        }
    } else {
        
        overlay_state.show_rom_browser = true;
        overlay_state.set_status("welcome! select a rom from the browser to play.");
    }

    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    static const char* mono_font_paths[] = {
        "/usr/share/fonts/truetype/ubuntu/ubuntumono[wght].ttf",
        "/usr/share/fonts/truetype/ubuntu/ubuntumono-regular.ttf",
        "/usr/share/fonts/truetype/dejavu/dejavusansmono.ttf",
        nullptr
    };

    ImFont* mono_font = nullptr;
    for (int i = 0; mono_font_paths[i] != nullptr; ++i) {
        std::ifstream font_file(mono_font_paths[i]);
        if (font_file.good()) {
            mono_font = io.Fonts->AddFontFromFileTTF(mono_font_paths[i], 16.0f);
            if (mono_font != nullptr) {
                break;
            }
        }
    }
    if (mono_font == nullptr) {
        io.Fonts->AddFontDefault();
    }

    setup_imgui_style();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    overlay_state.custom_bg[0] = active_palette.bg.r / 255.0f;
    overlay_state.custom_bg[1] = active_palette.bg.g / 255.0f;
    overlay_state.custom_bg[2] = active_palette.bg.b / 255.0f;
    overlay_state.custom_fg[0] = active_palette.fg.r / 255.0f;
    overlay_state.custom_fg[1] = active_palette.fg.g / 255.0f;
    overlay_state.custom_fg[2] = active_palette.fg.b / 255.0f;

    update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());

    bool running = true;
    while (running) {
        Uint32 frame_start = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                running = false;
            }

            
            if (keymap.awaiting_rebind_idx != -1) {
                if (event.type == SDL_KEYDOWN) {
                    SDL_Keycode sym = event.key.keysym.sym;
                    if (sym != SDLK_ESCAPE) {
                        keymap.rebind(keymap.awaiting_rebind_idx, sym);
                        keymap_ui_state.status_info = "bound key to " + KeyMapping::get_key_name(sym);
                    } else {
                        keymap_ui_state.status_info = "rebind cancelled.";
                    }
                    keymap.awaiting_rebind_idx = -1;
                    keymap_ui_state.rebinding_key_idx = -1;
                }
                continue;
            }
            if (event.type == SDL_KEYDOWN) {
                SDL_Keycode sym = event.key.keysym.sym;
                uint16_t mod = event.key.keysym.mod;
                bool ctrl = (mod & KMOD_CTRL) != 0;
                bool shift = (mod & KMOD_SHIFT) != 0;
                if (sym == SDLK_F2) {
                    overlay_state.show_control_panel = !overlay_state.show_control_panel;
                    overlay_state.set_status(overlay_state.show_control_panel ? "control panel opened" : "control panel closed");
                }
                
                else if (sym == SDLK_F3) {
                    overlay_state.show_inspector = !overlay_state.show_inspector;
                    overlay_state.set_status(overlay_state.show_inspector ? "cpu registers inspector opened" : "cpu registers inspector closed");
                }
                
                else if (sym == SDLK_F4) {
                    overlay_state.show_keypad = !overlay_state.show_keypad;
                    overlay_state.set_status(overlay_state.show_keypad ? "virtual keypad opened" : "virtual keypad closed");
                }
                
                else if (sym == SDLK_F8) {
                    overlay_state.show_audio_panel = !overlay_state.show_audio_panel;
                    overlay_state.set_status(overlay_state.show_audio_panel ? "sound synthesizer opened" : "sound synthesizer closed");
                }

                
                else if (sym == SDLK_ESCAPE) {
                    if (overlay_state.show_shortcuts) {
                        overlay_state.show_shortcuts = false;
                    } else if (overlay_state.show_rom_browser) {
                        overlay_state.show_rom_browser = false;
                    } else if (debugger_state.show_debugger) {
                        debugger_state.show_debugger = false;
                    } else if (editor_state.show_editor) {
                        editor_state.show_editor = false;
                    } else if (keymap_ui_state.show_keymap_window) {
                        keymap_ui_state.show_keymap_window = false;
                    } else {
                        running = false;
                    }
                }

                
                else if (sym == SDLK_h || sym == SDLK_F1) {
                    overlay_state.show_shortcuts = true;
                }
                if (!io.WantTextInput) {
                    
                    if (ctrl && sym == SDLK_u) {
                        overlay_state.show_overlay = !overlay_state.show_overlay;
                        overlay_state.set_status(overlay_state.show_overlay ? "ui overlay visible" : "ui overlay hidden");
                    }

                    
                    else if (ctrl && sym == SDLK_o) {
                        overlay_state.show_rom_browser = !overlay_state.show_rom_browser;
                        if (overlay_state.show_rom_browser) rom_browser_state.refresh();
                    }

                    
                    else if (sym == SDLK_F12 || (ctrl && sym == SDLK_d)) {
                        debugger_state.show_debugger = !debugger_state.show_debugger;
                        overlay_state.set_status(debugger_state.show_debugger ? "debugger opened" : "debugger closed");
                    }

                    
                    else if (ctrl && sym == SDLK_e) {
                        editor_state.show_editor = !editor_state.show_editor;
                        overlay_state.set_status(editor_state.show_editor ? "code editor opened" : "code editor closed");
                    }

                    
                    else if (ctrl && sym == SDLK_k) {
                        keymap_ui_state.show_keymap_window = !keymap_ui_state.show_keymap_window;
                        overlay_state.set_status(keymap_ui_state.show_keymap_window ? "keyboard config opened" : "keyboard config closed");
                    }

                    
                    else if (ctrl && sym == SDLK_r) {
                        if (!rom_filename.empty()) {
                            chip8.reset();
                            chip8.load_rom(rom_filename);
                            overlay_state.set_status("vm reset and rom reloaded");
                            update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                        }
                    }

                    
                    else if (sym == SDLK_F10) {
                        if (shift) {
                            debugger_state.step_over = true;
                        } else {
                            debugger_state.step_instruction = true;
                        }
                    }
                    else if (sym == SDLK_F9) {
                        debugger_state.toggle_breakpoint(chip8.get_pc());
                        char bmsg[64];
                        std::snprintf(bmsg, sizeof(bmsg), "toggled breakpoint at 0x%04x", chip8.get_pc());
                        overlay_state.set_status(bmsg);
                    }
                    else if (sym == SDLK_F11) {
                        step_one_frame = true;
                    }

                    
                    else if (sym == SDLK_EQUALS || sym == SDLK_PLUS || sym == SDLK_KP_PLUS || sym == SDLK_UP || sym == SDLK_RIGHTBRACKET) {
                        int delta = shift ? 5 : 1;
                        cycles_per_frame = std::min(100, cycles_per_frame + delta);
                        overlay_state.set_status("speed: " + std::to_string(cycles_per_frame * 60) + " hz");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                    else if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS || sym == SDLK_DOWN || sym == SDLK_LEFTBRACKET) {
                        int delta = shift ? 5 : 1;
                        cycles_per_frame = std::max(1, cycles_per_frame - delta);
                        overlay_state.set_status("speed: " + std::to_string(cycles_per_frame * 60) + " hz");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                    else if (sym == SDLK_PAGEUP) {
                        cycles_per_frame = std::min(100, cycles_per_frame + 5);
                        overlay_state.set_status("speed: " + std::to_string(cycles_per_frame * 60) + " hz");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                    else if (sym == SDLK_PAGEDOWN) {
                        cycles_per_frame = std::max(1, cycles_per_frame - 5);
                        overlay_state.set_status("speed: " + std::to_string(cycles_per_frame * 60) + " hz");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                    else if (sym == SDLK_0 || sym == SDLK_KP_0 || sym == SDLK_BACKSPACE) {
                        cycles_per_frame = 10;
                        overlay_state.set_status("speed reset: 600 hz");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }

                    
                    else if (sym == SDLK_SPACE || sym == SDLK_p) {
                        paused = !paused;
                        overlay_state.set_status(paused ? "paused" : "resumed");
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                    else if (sym == SDLK_PERIOD || sym == SDLK_n) {
                        if (paused) {
                            step_one_frame = true;
                            overlay_state.set_status("stepped 1 frame");
                        }
                    }

                    
                    else if (sym == SDLK_TAB || sym == SDLK_t) {
                        if (shift) {
                            current_palette_idx = (current_palette_idx + PALETTES.size() - 1) % PALETTES.size();
                        } else {
                            current_palette_idx = (current_palette_idx + 1) % PALETTES.size();
                        }
                        overlay_state.use_custom_colors = false;
                        active_palette = PALETTES[current_palette_idx];
                        overlay_state.custom_bg[0] = active_palette.bg.r / 255.0f;
                        overlay_state.custom_bg[1] = active_palette.bg.g / 255.0f;
                        overlay_state.custom_bg[2] = active_palette.bg.b / 255.0f;
                        overlay_state.custom_fg[0] = active_palette.fg.r / 255.0f;
                        overlay_state.custom_fg[1] = active_palette.fg.g / 255.0f;
                        overlay_state.custom_fg[2] = active_palette.fg.b / 255.0f;
                        overlay_state.set_status("theme: " + active_palette.name);
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }

                    
                    else if (sym == SDLK_F5 || (ctrl && sym == SDLK_s)) {
                        std::string savefile = get_savestate_path(rom_filename, current_slot);
                        if (chip8.save_state(savefile)) {
                            overlay_state.set_status("saved state to slot " + std::to_string(current_slot));
                        } else {
                            overlay_state.set_status("error saving state", true);
                        }
                    }
                    else if (sym == SDLK_F6 || (ctrl && sym == SDLK_l)) {
                        std::string savefile = get_savestate_path(rom_filename, current_slot);
                        if (chip8.load_state(savefile)) {
                            overlay_state.set_status("loaded state from slot " + std::to_string(current_slot));
                            update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                        } else {
                            overlay_state.set_status("slot " + std::to_string(current_slot) + " not found", true);
                        }
                    }
                    else if (sym == SDLK_F7) {
                        current_slot = (current_slot % 5) + 1;
                        overlay_state.set_status("selected slot " + std::to_string(current_slot));
                        update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                    }
                }

                
                int ck = keymap.get_chip8_key(sym);
                if (ck >= 0) chip8.key[ck] = 1;
            }

            
            if (event.type == SDL_KEYUP) {
                SDL_Keycode sym = event.key.keysym.sym;
                int ck = keymap.get_chip8_key(sym);
                if (ck >= 0) chip8.key[ck] = 0;
            }
        }

        
        if (rom_load_requested) {
            rom_load_requested = false;
            if (!rom_filename.empty()) {
                if (chip8.load_rom(rom_filename)) {
                    rom_browser_state.add_recent(rom_filename);
                    paused = false;
                    overlay_state.set_status("loaded rom: " + fs::path(rom_filename).filename().string());
                    update_window_title(window, cycles_per_frame, paused, active_palette.name, current_slot, chip8.is_extended_mode());
                } else {
                    overlay_state.set_status("failed to load rom: " + rom_filename, true);
                }
            }
        }

        
        
        if (debugger_state.step_instruction && !rom_filename.empty() && !chip8.is_halted()) {
            debugger_state.record_trace(chip8);
            chip8.emulate_cycle();
            debugger_state.step_instruction = false;
            char smsg[64];
            std::snprintf(smsg, sizeof(smsg), "stepped instruction to pc 0x%04x", chip8.get_pc());
            overlay_state.set_status(smsg);
        }
        
        else if (debugger_state.step_over && !rom_filename.empty() && !chip8.is_halted()) {
            uint16_t cur_pc = chip8.get_pc();
            uint16_t op = chip8.get_opcode();
            if ((op & 0xF000) == 0x2000) {
                uint16_t target_pc = cur_pc + 2;
                uint8_t start_sp = chip8.get_sp();
                int steps = 0;
                while (chip8.get_pc() != target_pc && chip8.get_sp() >= start_sp && steps++ < 20000 && !chip8.is_halted()) {
                    if (debugger_state.has_breakpoint(chip8.get_pc())) {
                        paused = true;
                        break;
                    }
                    debugger_state.record_trace(chip8);
                    chip8.emulate_cycle();
                }
            } else {
                debugger_state.record_trace(chip8);
                chip8.emulate_cycle();
            }
            debugger_state.step_over = false;
            paused = true;
        }
        
        else if ((!paused || step_one_frame) && !rom_filename.empty() && !chip8.is_halted()) {
            for (int i = 0; i < cycles_per_frame; i++) {
                
                if (debugger_state.has_breakpoint(chip8.get_pc())) {
                    paused = true;
                    debugger_state.breakpoint_hit = true;
                    debugger_state.last_hit_pc = chip8.get_pc();
                    for (auto& bp : debugger_state.breakpoints) {
                        if (bp.address == chip8.get_pc()) bp.hit_count++;
                    }
                    char bmsg[64];
                    std::snprintf(bmsg, sizeof(bmsg), "hit breakpoint at 0x%04x", chip8.get_pc());
                    overlay_state.set_status(bmsg);
                    break;
                }

                
                if (debugger_state.break_on_schip_exit && chip8.get_opcode() == 0x00FD) {
                    paused = true;
                    overlay_state.set_status("hit super-chip 00fd halt instruction");
                    break;
                }

                debugger_state.record_trace(chip8);
                chip8.emulate_cycle();

                
                if (debugger_state.break_on_collision && chip8.get_v(0xF) == 1) {
                    paused = true;
                    overlay_state.set_status("hit sprite collision (vf=1)");
                    break;
                }

                if (chip8.is_halted()) break;
            }
            chip8.update_timers();
            step_one_frame = false;
        }

        
        audio_config.beep = (!paused && chip8.get_sound_timer() > 0);

        
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        
        render_imgui_overlay_ui(
            chip8,
            overlay_state,
            audio_config,
            rom_browser_state,
            debugger_state,
            editor_state,
            keymap_ui_state,
            keymap,
            cycles_per_frame,
            paused,
            step_one_frame,
            current_palette_idx,
            active_palette,
            current_slot,
            rom_filename,
            rom_load_requested,
            running
        );

        
        int current_win_w = WINDOW_WIDTH;
        int current_win_h = WINDOW_HEIGHT;
        SDL_GetWindowSize(window, &current_win_w, &current_win_h);

        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);

        
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
    }

    
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}