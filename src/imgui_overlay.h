#ifndef IMGUI_OVERLAY_H
#define IMGUI_OVERLAY_H

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "chip8.h"
#include "palette.h"
#include "audio.h"
#include "rom_browser.h"
#include "disassembler.h"
#include "debugger.h"
#include "editor_ui.h"
#include "keymap_ui.h"

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <filesystem>

inline std::string get_savestate_path(const std::string& rom_path, int slot) {
    std::string base = rom_path.empty() ? "norom" : rom_path;
    size_t slash = base.find_last_of("/\\");
    if (slash != std::string::npos) {
        base = base.substr(slash + 1);
    }
    return "save_" + base + "_slot" + std::to_string(slot) + ".sav";
}

struct OverlayState {
    bool show_overlay = true;           
    bool show_control_panel = true;     
    bool show_inspector = true;         
    bool show_keypad = true;            
    bool show_audio_panel = true;       
    bool show_rom_browser = false;      
    bool show_shortcuts = false;        
    bool show_demo = false;             
    
    std::string status_msg = "Ready";
    bool status_is_error = false;
    Uint32 status_timestamp = 0;

    float custom_bg[3] = {0.0f, 0.0f, 0.0f};
    float custom_fg[3] = {1.0f, 1.0f, 1.0f};
    bool use_custom_colors = false;

    void set_status(const std::string& msg, bool error = false) {
        status_msg = msg;
        status_is_error = error;
        status_timestamp = SDL_GetTicks();
    }
};
inline void setup_imgui_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 4.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 3.0f;
    style.PopupRounding     = 3.0f;
    style.ScrollbarRounding = 2.0f;
    style.GrabRounding      = 2.0f;
    style.TabRounding       = 3.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.WindowPadding     = ImVec2(10.0f, 8.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

    
    colors[ImGuiCol_Text]                  = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.00f, 0.00f, 0.00f, 0.98f); 
    colors[ImGuiCol_ChildBg]               = ImVec4(0.03f, 0.03f, 0.03f, 0.90f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.04f, 0.04f, 0.04f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.20f, 0.20f, 0.20f, 0.90f); 
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); 
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.18f, 0.18f, 0.18f, 0.90f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.15f, 0.15f, 0.15f, 0.60f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.35f, 0.35f, 0.35f, 0.90f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
}
inline void render_imgui_overlay_ui(
    Chip8& chip8,
    OverlayState& state,
    AudioConfig& audio_config,
    RomBrowserState& rom_browser_state,
    DebuggerState& debugger_state,
    CodeEditorState& editor_state,
    KeymapUIState& keymap_ui_state,
    KeyMapping& keymap,
    int& cycles_per_frame,
    bool& paused,
    bool& step_one_frame,
    size_t& current_palette_idx,
    ColorPalette& active_palette,
    int& current_slot,
    std::string& rom_filename,
    bool& rom_load_requested,
    bool& running
) {
    if (state.show_demo) {
        ImGui::ShowDemoWindow(&state.show_demo);
    }

    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open ROM...", "Ctrl+O")) {
                state.show_rom_browser = true;
                rom_browser_state.refresh();
            }

            if (!rom_browser_state.recent_roms.empty()) {
                if (ImGui::BeginMenu("Recent ROMs")) {
                    for (const auto& rpath : rom_browser_state.recent_roms) {
                        std::string rname = std::filesystem::path(rpath).filename().string();
                        if (ImGui::MenuItem(rname.c_str())) {
                            rom_filename = rpath;
                            rom_load_requested = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Quick Save", "F5 / Ctrl+S", false, !rom_filename.empty())) {
                std::string path = get_savestate_path(rom_filename, current_slot);
                if (chip8.save_state(path)) {
                    state.set_status("Saved state to Slot " + std::to_string(current_slot));
                } else {
                    state.set_status("Failed to save state", true);
                }
            }
            if (ImGui::MenuItem("Quick Load", "F6 / Ctrl+L", false, !rom_filename.empty())) {
                std::string path = get_savestate_path(rom_filename, current_slot);
                if (chip8.load_state(path)) {
                    state.set_status("Loaded state from Slot " + std::to_string(current_slot));
                } else {
                    state.set_status("Slot " + std::to_string(current_slot) + " not found", true);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reload ROM", "Ctrl+R", false, !rom_filename.empty())) {
                chip8.reset();
                chip8.load_rom(rom_filename);
                state.set_status("VM Reset and ROM reloaded");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Esc")) {
                running = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulation")) {
            if (chip8.is_extended_mode()) {
                ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "Mode: Super-CHIP (128x64)");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "Mode: Standard CHIP-8 (64x32)");
            }
            ImGui::Separator();
            if (ImGui::MenuItem(paused ? "Resume" : "Pause", "Space / P", paused)) {
                paused = !paused;
                state.set_status(paused ? "Paused" : "Resumed");
            }
            if (ImGui::MenuItem("Step 1 Frame", ".", false, paused)) {
                step_one_frame = true;
                state.set_status("Stepped 1 Frame");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Speed: 120 Hz (Slow)")) { cycles_per_frame = 2; }
            if (ImGui::MenuItem("Speed: 300 Hz"))        { cycles_per_frame = 5; }
            if (ImGui::MenuItem("Speed: 600 Hz (Normal)")) { cycles_per_frame = 10; }
            if (ImGui::MenuItem("Speed: 1200 Hz (2x)"))  { cycles_per_frame = 20; }
            if (ImGui::MenuItem("Speed: 1800 Hz (SCHIP)")) { cycles_per_frame = 30; }
            if (ImGui::MenuItem("Speed: 3000 Hz (Turbo)")) { cycles_per_frame = 50; }
            ImGui::Separator();

            if (ImGui::BeginMenu("Compatibility & Quirks")) {
                if (ImGui::MenuItem("Preset: Super-CHIP 1.1 (SCHIP)")) {
                    chip8.set_quirk_shift_vx(true);
                    chip8.set_quirk_load_store_no_i(true);
                    chip8.set_quirk_jump_vx(true);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(false);
                    cycles_per_frame = 30;
                    state.set_status("Applied Super-CHIP 1.1 quirks (1800 Hz)");
                }
                if (ImGui::MenuItem("Preset: Modern XO-CHIP / Octo")) {
                    chip8.set_quirk_shift_vx(true);
                    chip8.set_quirk_load_store_no_i(true);
                    chip8.set_quirk_jump_vx(false);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(false);
                    cycles_per_frame = 20;
                    state.set_status("Applied modern XO-CHIP quirks (1200 Hz)");
                }
                if (ImGui::MenuItem("Preset: Classic COSMAC VIP (1977)")) {
                    chip8.set_quirk_shift_vx(false);
                    chip8.set_quirk_load_store_no_i(false);
                    chip8.set_quirk_jump_vx(false);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(true);
                    cycles_per_frame = 10;
                    state.set_status("Applied COSMAC VIP quirks (600 Hz)");
                }
                ImGui::Separator();

                bool q_shift = chip8.get_quirk_shift_vx();
                if (ImGui::MenuItem("Shift in-place (Vx >>= 1) [SCHIP]", nullptr, q_shift)) {
                    chip8.set_quirk_shift_vx(!q_shift);
                }
                bool q_load = chip8.get_quirk_load_store_no_i();
                if (ImGui::MenuItem("Memory I unchanged on Fx55/Fx65 [SCHIP]", nullptr, q_load)) {
                    chip8.set_quirk_load_store_no_i(!q_load);
                }
                bool q_jump = chip8.get_quirk_jump_vx();
                if (ImGui::MenuItem("Jump with offset Bxnn [SCHIP]", nullptr, q_jump)) {
                    chip8.set_quirk_jump_vx(!q_jump);
                }
                bool q_scroll = chip8.get_quirk_legacy_scroll();
                if (ImGui::MenuItem("Legacy half-line scrolling [HP-48]", nullptr, q_scroll)) {
                    chip8.set_quirk_legacy_scroll(!q_scroll);
                }
                bool q_vf = chip8.get_quirk_vf_reset();
                if (ImGui::MenuItem("Reset VF on 8xy1-3 bitwise ops [VIP]", nullptr, q_vf)) {
                    chip8.set_quirk_vf_reset(!q_vf);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show All UI Overlay", "Ctrl+U", &state.show_overlay);
            ImGui::Separator();
            ImGui::MenuItem("Control Panel", "F2", &state.show_control_panel);
            ImGui::MenuItem("CPU Registers & Timers Inspector", "F3", &state.show_inspector);
            ImGui::MenuItem("Virtual Keypad", "F4", &state.show_keypad);
            ImGui::MenuItem("Sound Synthesizer Panel", "F8", &state.show_audio_panel);
            ImGui::Separator();
            ImGui::MenuItem("Disassembler / Debugger Window", "F12 / Ctrl+D", &debugger_state.show_debugger);
            ImGui::MenuItem("CHIP-8 Code Editor & Assembler", "Ctrl+E", &editor_state.show_editor);
            ImGui::MenuItem("Custom Keyboard Mapping", "Ctrl+K", &keymap_ui_state.show_keymap_window);
            ImGui::MenuItem("ROM File Browser", "Ctrl+O", &state.show_rom_browser);
            ImGui::Separator();
            if (ImGui::MenuItem("Show All Floating Panels")) {
                state.show_control_panel = true;
                state.show_inspector = true;
                state.show_keypad = true;
                state.show_audio_panel = true;
            }
            if (ImGui::MenuItem("Hide All Floating Panels")) {
                state.show_control_panel = false;
                state.show_inspector = false;
                state.show_keypad = false;
                state.show_audio_panel = false;
                debugger_state.show_debugger = false;
                editor_state.show_editor = false;
                keymap_ui_state.show_keymap_window = false;
                state.show_rom_browser = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Disassembler / Debugger Window", "F12 / Ctrl+D", &debugger_state.show_debugger);
            ImGui::Separator();
            if (ImGui::MenuItem(paused ? "Continue Execution" : "Pause Execution", "Space / P")) {
                paused = !paused;
                state.set_status(paused ? "Paused" : "Resumed");
            }
            if (ImGui::MenuItem("Step Single Cycle", "F10", false, paused)) {
                debugger_state.step_instruction = true;
            }
            if (ImGui::MenuItem("Step Over Subroutine", "Shift+F10", false, paused)) {
                debugger_state.step_over = true;
            }
            if (ImGui::MenuItem("Step 1 Frame", "F11 / .", false, paused)) {
                step_one_frame = true;
                state.set_status("Stepped 1 Frame");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Toggle Breakpoint at PC", "F9")) {
                debugger_state.toggle_breakpoint(chip8.get_pc());
                char bmsg[64];
                std::snprintf(bmsg, sizeof(bmsg), "Toggled Breakpoint at 0x%04X", chip8.get_pc());
                state.set_status(bmsg);
            }
            if (ImGui::MenuItem("Clear All Breakpoints", nullptr, false, !debugger_state.breakpoints.empty())) {
                debugger_state.clear_breakpoints();
                state.set_status("Cleared all Breakpoints");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("CHIP-8 Code Editor & Assembler", "Ctrl+E", &editor_state.show_editor);
            ImGui::MenuItem("Custom Keyboard Mapping", "Ctrl+K", &keymap_ui_state.show_keymap_window);
            ImGui::Separator();
            ImGui::MenuItem("Sound Synthesizer Panel", "F8", &state.show_audio_panel);
            ImGui::MenuItem("Control Panel", "F2", &state.show_control_panel);
            ImGui::MenuItem("CPU Registers Inspector", "F3", &state.show_inspector);
            ImGui::MenuItem("Virtual Keypad", "F4", &state.show_keypad);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Color Themes")) {
            for (size_t i = 0; i < PALETTES.size(); i++) {
                bool is_selected = (!state.use_custom_colors && current_palette_idx == i);
                if (ImGui::MenuItem(PALETTES[i].name.c_str(), nullptr, is_selected)) {
                    current_palette_idx = i;
                    state.use_custom_colors = false;
                    active_palette = PALETTES[i];
                    state.custom_bg[0] = active_palette.bg.r / 255.0f;
                    state.custom_bg[1] = active_palette.bg.g / 255.0f;
                    state.custom_bg[2] = active_palette.bg.b / 255.0f;
                    state.custom_fg[0] = active_palette.fg.r / 255.0f;
                    state.custom_fg[1] = active_palette.fg.g / 255.0f;
                    state.custom_fg[2] = active_palette.fg.b / 255.0f;
                    state.set_status("Theme: " + active_palette.name);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Keyboard Controls & Shortcuts", "H / F1")) {
                state.show_shortcuts = true;
            }
            ImGui::EndMenu();
        }

        
        float right_indent = ImGui::GetWindowWidth() - 410.0f;
        if (right_indent > 250.0f) {
            ImGui::SameLine(right_indent);
            if (chip8.is_extended_mode()) {
                ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "[SCHIP 128x64]");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "[CHIP-8 64x32]");
            }
            ImGui::SameLine();
            ImGui::Text("| %d Hz", cycles_per_frame * 60);
            ImGui::SameLine();
            ImGui::Text("| Slot: %d", current_slot);
        }

        ImGui::EndMainMenuBar();
    }

    
    if (state.show_rom_browser) {
        std::string chosen_path;
        if (render_rom_browser_ui(&state.show_rom_browser, chosen_path, rom_browser_state)) {
            rom_filename = chosen_path;
            rom_load_requested = true;
        }
    }

    
    if (!state.show_overlay) {
        
        float disp_w = ImGui::GetIO().DisplaySize.x;
        float disp_h = ImGui::GetIO().DisplaySize.y;
        float scr_w = disp_w - 40.0f;
        float scr_h = disp_h - 45.0f;
        float scr_x = 20.0f;
        float scr_y = 35.0f;

        ImGui::SetNextWindowSize(ImVec2(scr_w, scr_h), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(scr_x, scr_y), ImGuiCond_Always);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

        std::string title = "CHIP-8 Emulation Display";
        if (!rom_filename.empty()) title += " - " + std::filesystem::path(rom_filename).filename().string();
        title += chip8.is_extended_mode() ? " [SCHIP 128x64]" : " [CHIP-8 64x32]";
        if (paused) title += " [Paused]";
        title += "###emulationdisplay";

        if (ImGui::Begin(title.c_str(), nullptr, flags)) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();

            float target_aspect = 2.0f;
            float draw_w = canvas_size.x;
            float draw_h = draw_w / target_aspect;
            if (draw_h > canvas_size.y) {
                draw_h = canvas_size.y;
                draw_w = draw_h * target_aspect;
            }

            float offset_x = canvas_pos.x + (canvas_size.x - draw_w) * 0.5f;
            float offset_y = canvas_pos.y + (canvas_size.y - draw_h) * 0.5f;

            ImU32 col_bg = state.use_custom_colors ? 
                IM_COL32((int)(state.custom_bg[0] * 255), (int)(state.custom_bg[1] * 255), (int)(state.custom_bg[2] * 255), 255) :
                IM_COL32(active_palette.bg.r, active_palette.bg.g, active_palette.bg.b, 255);
            ImU32 col_fg = state.use_custom_colors ? 
                IM_COL32((int)(state.custom_fg[0] * 255), (int)(state.custom_fg[1] * 255), (int)(state.custom_fg[2] * 255), 255) :
                IM_COL32(active_palette.fg.r, active_palette.fg.g, active_palette.fg.b, 255);
            ImU32 col_border = IM_COL32(60, 75, 95, 255);

            draw_list->AddRectFilled(ImVec2(offset_x, offset_y), ImVec2(offset_x + draw_w, offset_y + draw_h), col_bg);

            int dw = chip8.get_display_width();
            int dh = chip8.get_display_height();
            float pixel_w = draw_w / (float)dw;
            float pixel_h = draw_h / (float)dh;

            for (int y = 0; y < dh; y++) {
                for (int x = 0; x < dw; x++) {
                    if (chip8.display[x + y * dw] == 1) {
                        float px = offset_x + x * pixel_w;
                        float py = offset_y + y * pixel_h;
                        draw_list->AddRectFilled(ImVec2(px, py), ImVec2(px + pixel_w, py + pixel_h), col_fg);
                    }
                }
            }
            draw_list->AddRect(ImVec2(offset_x, offset_y), ImVec2(offset_x + draw_w, offset_y + draw_h), col_border, 2.0f);
        }
        ImGui::End();
    } else {
        float disp_w = ImGui::GetIO().DisplaySize.x;
        float disp_h = ImGui::GetIO().DisplaySize.y;

        float top_y = 35.0f;
        float left_x = 20.0f;
        float game_w = 640.0f;
        float game_h = 320.0f;
        float gap = 10.0f;

        float right_x = left_x + game_w + gap; 
        float right_w = std::max(320.0f, disp_w - right_x - 20.0f); 
        float right_h1 = 320.0f;
        float right_h2 = std::max(280.0f, disp_h - top_y - right_h1 - gap - 10.0f);

        float bottom_y = top_y + game_h + gap; 
        float bottom_h = std::max(260.0f, disp_h - bottom_y - 10.0f); 
        float keypad_w = 295.0f;
        float sound_x = left_x + keypad_w + gap; 
        float sound_w = (left_x + game_w) - sound_x; 

        
        {
            ImGui::SetNextWindowSize(ImVec2(game_w, game_h), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(left_x, top_y), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | 
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

            std::string title = "Emulation Screen";
            if (!rom_filename.empty()) title += " - " + std::filesystem::path(rom_filename).filename().string();
            title += chip8.is_extended_mode() ? " [SCHIP 128x64]" : " [CHIP-8 64x32]";
            if (paused) title += " [Paused]";
            title += "###emulationdisplay";

            if (ImGui::Begin(title.c_str(), nullptr, flags)) {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                ImVec2 canvas_size = ImGui::GetContentRegionAvail();

                if (canvas_size.x > 20 && canvas_size.y > 20) {
                    float target_aspect = 2.0f;
                    float draw_w = canvas_size.x;
                    float draw_h = draw_w / target_aspect;
                    if (draw_h > canvas_size.y) {
                        draw_h = canvas_size.y;
                        draw_w = draw_h * target_aspect;
                    }

                    float offset_x = canvas_pos.x + (canvas_size.x - draw_w) * 0.5f;
                    float offset_y = canvas_pos.y + (canvas_size.y - draw_h) * 0.5f;

                    ImU32 col_bg = state.use_custom_colors ? 
                        IM_COL32((int)(state.custom_bg[0] * 255), (int)(state.custom_bg[1] * 255), (int)(state.custom_bg[2] * 255), 255) :
                        IM_COL32(active_palette.bg.r, active_palette.bg.g, active_palette.bg.b, 255);
                    ImU32 col_fg = state.use_custom_colors ? 
                        IM_COL32((int)(state.custom_fg[0] * 255), (int)(state.custom_fg[1] * 255), (int)(state.custom_fg[2] * 255), 255) :
                        IM_COL32(active_palette.fg.r, active_palette.fg.g, active_palette.fg.b, 255);
                    ImU32 col_border = IM_COL32(50, 65, 85, 255);

                    
                    draw_list->AddRectFilled(ImVec2(offset_x, offset_y), ImVec2(offset_x + draw_w, offset_y + draw_h), col_bg);

                    int dw = chip8.get_display_width();
                    int dh = chip8.get_display_height();
                    float pixel_w = draw_w / (float)dw;
                    float pixel_h = draw_h / (float)dh;

                    
                    for (int y = 0; y < dh; y++) {
                        for (int x = 0; x < dw; x++) {
                            if (chip8.display[x + y * dw] == 1) {
                                float px = offset_x + x * pixel_w;
                                float py = offset_y + y * pixel_h;
                                draw_list->AddRectFilled(ImVec2(px, py), ImVec2(px + pixel_w, py + pixel_h), col_fg);
                            }
                        }
                    }

                    
                    draw_list->AddRect(ImVec2(offset_x, offset_y), ImVec2(offset_x + draw_w, offset_y + draw_h), col_border, 1.0f);

                    if (rom_filename.empty()) {
                        const char* msg = "No ROM loaded. Press Ctrl+O to open ROM browser.";
                        ImVec2 txt_sz = ImGui::CalcTextSize(msg);
                        ImVec2 txt_pos(offset_x + (draw_w - txt_sz.x) * 0.5f, offset_y + (draw_h - txt_sz.y) * 0.5f);
                        draw_list->AddRectFilled(ImVec2(txt_pos.x - 8, txt_pos.y - 4), ImVec2(txt_pos.x + txt_sz.x + 8, txt_pos.y + txt_sz.y + 4), IM_COL32(0, 0, 0, 200), 4.0f);
                        draw_list->AddText(txt_pos, IM_COL32(0, 230, 210, 255), msg);
                    } else if (paused) {
                        const char* pmsg = "PAUSED";
                        ImVec2 psz = ImGui::CalcTextSize(pmsg);
                        ImVec2 ppos(offset_x + draw_w - psz.x - 12.0f, offset_y + 8.0f);
                        draw_list->AddRectFilled(ImVec2(ppos.x - 5, ppos.y - 2), ImVec2(ppos.x + psz.x + 5, ppos.y + psz.y + 2), IM_COL32(200, 120, 20, 220), 3.0f);
                        draw_list->AddText(ppos, IM_COL32(255, 255, 255, 255), pmsg);
                    }
                }
            }
            ImGui::End();
        }

        
        if (state.show_control_panel) {
            ImGui::SetNextWindowSize(ImVec2(right_w, right_h1), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(right_x, top_y), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

            if (ImGui::Begin("CHIP-8 Controls##overlay", &state.show_control_panel, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
                ImGui::TextDisabled("Status & Speed");
                if (paused) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.60f, 0.30f, 0.9f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.75f, 0.38f, 1.0f));
                    if (ImGui::Button("  Resume Execution  ", ImVec2(-1, 24))) {
                        paused = false;
                        state.set_status("Resumed");
                    }
                    ImGui::PopStyleColor(2);

                    if (ImGui::Button("Step 1 Frame (.)", ImVec2(-1, 20))) {
                        step_one_frame = true;
                        state.set_status("Stepped 1 frame");
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.45f, 0.10f, 0.9f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.55f, 0.15f, 1.0f));
                    if (ImGui::Button("Pause Execution", ImVec2(-1, 24))) {
                        paused = true;
                        state.set_status("Paused");
                    }
                    ImGui::PopStyleColor(2);
                }

                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##speed", &cycles_per_frame, 1, 100, "Speed: %d cyc/f (60 Hz)");

                ImGui::Separator();

                
                ImGui::TextDisabled("Savestate Slots (1 - 5):");
                float slot_w = (ImGui::GetContentRegionAvail().x - 4 * 6.0f) / 5.0f;
                for (int s = 1; s <= 5; s++) {
                    if (s > 1) ImGui::SameLine(0.0f, 6.0f);
                    char slot_lbl[16];
                    std::snprintf(slot_lbl, sizeof(slot_lbl), current_slot == s ? "[Slot %d]" : "Slot %d", s);
                    if (current_slot == s) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.80f, 1.0f));
                    }
                    if (ImGui::Button(slot_lbl, ImVec2(slot_w, 22))) {
                        current_slot = s;
                        state.set_status("Selected Slot " + std::to_string(current_slot));
                    }
                    if (current_slot == s) {
                        ImGui::PopStyleColor();
                    }
                }

                std::string slot_filename = get_savestate_path(rom_filename, current_slot);
                float half_w = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;

                if (ImGui::Button(" Save State ", ImVec2(half_w, 24))) {
                    if (chip8.save_state(slot_filename)) {
                        state.set_status("State saved to " + slot_filename);
                    } else {
                        state.set_status("Error saving state", true);
                    }
                }
                ImGui::SameLine(0.0f, 6.0f);
                if (ImGui::Button(" Load State ", ImVec2(half_w, 24))) {
                    if (chip8.load_state(slot_filename)) {
                        state.set_status("State loaded from " + slot_filename);
                    } else {
                        state.set_status("Slot " + std::to_string(current_slot) + " not found", true);
                    }
                }

                ImGui::Separator();
                ImGui::TextDisabled("Color Theme:");
                const char* current_theme_name = state.use_custom_colors ? "Custom Color Palette" : PALETTES[current_palette_idx].name.c_str();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##preset", current_theme_name)) {
                    for (size_t i = 0; i < PALETTES.size(); i++) {
                        bool is_selected = (!state.use_custom_colors && current_palette_idx == i);
                        if (ImGui::Selectable(PALETTES[i].name.c_str(), is_selected)) {
                            current_palette_idx = i;
                            state.use_custom_colors = false;
                            active_palette = PALETTES[i];
                            state.custom_bg[0] = active_palette.bg.r / 255.0f;
                            state.custom_bg[1] = active_palette.bg.g / 255.0f;
                            state.custom_bg[2] = active_palette.bg.b / 255.0f;
                            state.custom_fg[0] = active_palette.fg.r / 255.0f;
                            state.custom_fg[1] = active_palette.fg.g / 255.0f;
                            state.custom_fg[2] = active_palette.fg.b / 255.0f;
                            state.set_status("Applied theme: " + active_palette.name);
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                
                float color_w = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;
                ImGui::SetNextItemWidth(color_w - 28.0f);
                if (ImGui::ColorEdit3("BG##custom", state.custom_bg, ImGuiColorEditFlags_NoInputs)) {
                    state.use_custom_colors = true;
                    active_palette.bg = {
                        (uint8_t)(state.custom_bg[0] * 255),
                        (uint8_t)(state.custom_bg[1] * 255),
                        (uint8_t)(state.custom_bg[2] * 255)
                    };
                }
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetNextItemWidth(color_w - 28.0f);
                if (ImGui::ColorEdit3("FG##custom", state.custom_fg, ImGuiColorEditFlags_NoInputs)) {
                    state.use_custom_colors = true;
                    active_palette.fg = {
                        (uint8_t)(state.custom_fg[0] * 255),
                        (uint8_t)(state.custom_fg[1] * 255),
                        (uint8_t)(state.custom_fg[2] * 255)
                    };
                }
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }

        
        if (state.show_audio_panel) {
            ImGui::SetNextWindowSize(ImVec2(sound_w, bottom_h), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(sound_x, bottom_y), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

            if (ImGui::Begin("Sound Synthesizer##overlay", &state.show_audio_panel, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
                
                ImGui::Checkbox("Master Audio", &audio_config.sound_enabled);
                ImGui::SameLine();
                if (audio_config.sound_enabled) {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "[Active]");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[Muted]");
                }

                
                ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##waveform", WAVEFORM_NAMES[audio_config.waveform_type])) {
                    for (int i = 0; i < WAVEFORM_COUNT; i++) {
                        bool is_sel = (audio_config.waveform_type == i);
                        if (ImGui::Selectable(WAVEFORM_NAMES[i], is_sel)) {
                            audio_config.waveform_type = i;
                            state.set_status(std::string("Waveform: ") + WAVEFORM_NAMES[i]);
                        }
                        if (is_sel) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                
                float wave_btn_w = (ImGui::GetContentRegionAvail().x - 4 * 4.0f) / 5.0f;
                auto draw_wave_btn = [&](const char* label, int wtype, int idx) {
                    if (idx > 0) ImGui::SameLine(0.0f, 4.0f);
                    if (audio_config.waveform_type == wtype) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.85f, 1.0f));
                    }
                    if (ImGui::Button(label, ImVec2(wave_btn_w, 20))) {
                        audio_config.waveform_type = wtype;
                    }
                    if (audio_config.waveform_type == wtype) {
                        ImGui::PopStyleColor();
                    }
                };

                draw_wave_btn("Sine", WAVEFORM_SINE, 0);
                draw_wave_btn("Square", WAVEFORM_SQUARE, 1);
                draw_wave_btn("Saw", WAVEFORM_SAWTOOTH, 2);
                draw_wave_btn("Tri", WAVEFORM_TRIANGLE, 3);
                draw_wave_btn("Noise", WAVEFORM_NOISE, 4);

                
                float preview_points[100];
                generate_waveform_preview(audio_config, preview_points, 100);
                char overlay_plot_text[64];
                std::snprintf(overlay_plot_text, sizeof(overlay_plot_text), "%s (%.0f Hz)", WAVEFORM_NAMES[audio_config.waveform_type], audio_config.waveform_freq);
                ImGui::PlotLines("##oscilloscope", preview_points, 100, 0, overlay_plot_text, -1.0f, 1.0f, ImVec2(-1, 48));

                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##freq", &audio_config.waveform_freq, 60.0f, 2000.0f, "Freq: %.0f Hz");
                
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##vol", &audio_config.volume, 0.0f, 1.0f, "Vol: %.0f%%");

                
                ImGui::PushStyleColor(ImGuiCol_Button, audio_config.test_tone_active ? ImVec4(0.85f, 0.20f, 0.20f, 1.0f) : ImVec4(0.18f, 0.45f, 0.70f, 1.0f));
                ImGui::Button("  Hold mouse to test audio tone  ", ImVec2(-1, 24));
                audio_config.test_tone_active = ImGui::IsItemActive();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }

        
        if (state.show_inspector) {
            ImGui::SetNextWindowSize(ImVec2(right_w, right_h2), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(right_x, top_y + right_h1 + gap), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

            if (ImGui::Begin("CPU Registers & Timers##overlay", &state.show_inspector, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 3.0f));
                
                ImGui::Text("PC: 0x%03X  |  I: 0x%03X  |  SP: %d", chip8.get_pc(), chip8.get_index(), chip8.get_sp());
                ImGui::Text("Opcode: 0x%04X", chip8.get_opcode());

                
                float delay_pct = chip8.get_delay_timer() / 255.0f;
                char delay_str[32];
                std::snprintf(delay_str, sizeof(delay_str), "Delay: %d", chip8.get_delay_timer());
                ImGui::ProgressBar(delay_pct, ImVec2(-1, 12), delay_str);

                float sound_pct = chip8.get_sound_timer() / 255.0f;
                char sound_str[32];
                std::snprintf(sound_str, sizeof(sound_str), "Sound: %d %s", chip8.get_sound_timer(), (chip8.get_sound_timer() > 0 ? "[BEEP]" : ""));
                if (chip8.get_sound_timer() > 0) {
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.4f, 0.2f, 1.0f));
                }
                ImGui::ProgressBar(sound_pct, ImVec2(-1, 12), sound_str);
                if (chip8.get_sound_timer() > 0) {
                    ImGui::PopStyleColor();
                }

                ImGui::Separator();
                ImGui::TextDisabled("Registers V0 - VF:");
                if (ImGui::BeginTable("reg_table", 4, ImGuiTableFlags_BordersInnerV)) {
                    for (int i = 0; i < 16; i++) {
                        if (i % 4 == 0) ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(i % 4);
                        uint8_t val = chip8.get_v(i);
                        if (i == 0xF && val > 0) {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "V%X:%02X", i, val);
                        } else {
                            ImGui::Text("V%X:%02X", i, val);
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Separator();
                ImGui::TextDisabled("RPL User Flags R0 - RF:");
                if (ImGui::BeginTable("rpl_table", 4, ImGuiTableFlags_BordersInnerV)) {
                    for (int i = 0; i < 16; i++) {
                        if (i % 4 == 0) ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(i % 4);
                        uint8_t val = chip8.get_rpl_flag(i);
                        ImGui::Text("R%X:%02X", i, val);
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }

        
        if (state.show_keypad) {
            ImGui::SetNextWindowSize(ImVec2(keypad_w, bottom_h), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(left_x, bottom_y), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

            if (ImGui::Begin("Virtual Keypad##overlay", &state.show_keypad, flags)) {
                static const int keypad_layout[4][4] = {
                    {0x1, 0x2, 0x3, 0xC},
                    {0x4, 0x5, 0x6, 0xD},
                    {0x7, 0x8, 0x9, 0xE},
                    {0xA, 0x0, 0xB, 0xF}
                };

                static const char* key_labels[16] = {
                    "0\n(X)", "1\n(1)", "2\n(2)", "3\n(3)",
                    "4\n(Q)", "5\n(W)", "6\n(E)", "7\n(A)",
                    "8\n(S)", "9\n(D)", "A\n(Z)", "B\n(C)",
                    "C\n(4)", "D\n(R)", "E\n(F)", "F\n(V)"
                };

                float avail_w = ImGui::GetContentRegionAvail().x;
                float avail_h = ImGui::GetContentRegionAvail().y;
                float btn_gap = 5.0f;
                float btn_w = (avail_w - 3 * btn_gap) / 4.0f;
                float btn_h = (avail_h - 3 * btn_gap) / 4.0f;

                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        if (c > 0) ImGui::SameLine(0.0f, btn_gap);
                        int key_val = keypad_layout[r][c];
                        bool is_down = (chip8.key[key_val] != 0);

                        if (is_down) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.85f, 0.65f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        }

                        char btn_id[32];
                        std::snprintf(btn_id, sizeof(btn_id), "%s##k%d", key_labels[key_val], key_val);
                        ImGui::Button(btn_id, ImVec2(btn_w, btn_h));

                        
                        if (ImGui::IsItemActive()) {
                            chip8.key[key_val] = 1;
                        }

                        if (is_down) {
                            ImGui::PopStyleColor(2);
                        }
                    }
                }
            }
            ImGui::End();
        }
    }

    
    render_debugger_ui(chip8, debugger_state, paused, step_one_frame, cycles_per_frame);

    
    render_code_editor_ui(chip8, editor_state, paused, debugger_state.show_debugger, rom_filename);

    
    render_keymap_ui(keymap, keymap_ui_state);

    
    if (state.show_shortcuts) {
        float disp_w = ImGui::GetIO().DisplaySize.x;
        float disp_h = ImGui::GetIO().DisplaySize.y;
        float hw = std::min(disp_w - 40.0f, 780.0f);
        float hh = std::min(disp_h - 45.0f, 600.0f);
        float hx = (disp_w - hw) * 0.5f;
        float hy = 35.0f;

        ImGui::SetNextWindowSize(ImVec2(hw, hh), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(hx, hy), ImGuiCond_Always);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        if (ImGui::Begin("Keyboard Controls & Shortcuts##modal", &state.show_shortcuts, flags)) {
            ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "CHIP-8 & Super-CHIP Emulator Guide");
            ImGui::Separator();

            if (ImGui::BeginTabBar("helptabs")) {
                if (ImGui::BeginTabItem("Keyboard Shortcuts")) {
                    ImGui::TextDisabled("Emulation Hotkeys:");
                    if (ImGui::BeginTable("helptable1", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Shortcut Key", ImGuiTableColumnFlags_WidthFixed, 180);
                        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto add_row = [](const char* key, const char* desc) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", key);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", desc);
                        };

                        add_row("Space / P", "Pause or resume emulation");
                        add_row(". / N", "Step exactly 1 frame (60 Hz) when paused");
                        add_row("+ / Up / ]", "Increase emulation speed (+60 Hz / +1 cycle)");
                        add_row("- / Down / [", "Decrease emulation speed (-60 Hz / -1 cycle)");
                        add_row("PageUp / PageDn", "Fast speed jump (+/- 300 Hz)");
                        add_row("0 / Backspace", "Reset speed to default 600 Hz (10 cycles/frame)");
                        add_row("Tab / T", "Cycle next color theme palette");
                        add_row("Shift + Tab", "Cycle previous color theme palette");
                        add_row("F5 / Ctrl + S", "Quick save state to selected slot (1-5)");
                        add_row("F6 / Ctrl + L", "Quick load state from selected slot");
                        add_row("F7", "Switch savestate slot (1 to 5)");
                        add_row("Ctrl + R", "Reset virtual machine & reload ROM");
                        add_row("Ctrl + U", "Toggle full UI overlay on / off");
                        add_row("Esc", "Close active dialog or exit emulator");
                        ImGui::EndTable();
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextDisabled("Debugger & Tools Hotkeys:");
                    if (ImGui::BeginTable("helptable2", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Shortcut Key", ImGuiTableColumnFlags_WidthFixed, 180);
                        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto add_row = [](const char* key, const char* desc) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "%s", key);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", desc);
                        };

                        add_row("F12 / Ctrl + D", "Open / close debugger & disassembler");
                        add_row("F10", "Step single CPU instruction (cycle)");
                        add_row("Shift + F10", "Step over subroutine (CALL / 2nnn)");
                        add_row("F11", "Step single frame");
                        add_row("F9", "Toggle breakpoint at current program counter");
                        add_row("Ctrl + E", "Open / close CHIP-8 code editor & assembler");
                        add_row("Ctrl + K", "Open / close custom keypad mapping");
                        add_row("Ctrl + O", "Open ROM file browser");
                        add_row("F2", "Toggle CHIP-8 controls panel");
                        add_row("F3", "Toggle CPU registers & timers inspector");
                        add_row("F4", "Toggle virtual keypad panel");
                        add_row("F8", "Toggle sound synthesizer panel");
                        add_row("H / F1", "Toggle this help & controls window");
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Hex Keypad Layout")) {
                    ImGui::TextDisabled("CHIP-8 Hex Keypad (16 keys) vs PC Keyboard Mapping:");
                    ImGui::Spacing();

                    ImGui::Text("Standard COSMAC VIP Keypad:");
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                        "    +---+---+---+---+\n"
                        "    | 1 | 2 | 3 | C |\n"
                        "    +---+---+---+---+\n"
                        "    | 4 | 5 | 6 | D |\n"
                        "    +---+---+---+---+\n"
                        "    | 7 | 8 | 9 | E |\n"
                        "    +---+---+---+---+\n"
                        "    | A | 0 | B | F |\n"
                        "    +---+---+---+---+"
                    );

                    ImGui::Spacing();
                    ImGui::Text("Default PC Keyboard Mapping (QWERTY):");
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f),
                        "    +---+---+---+---+\n"
                        "    | 1 | 2 | 3 | 4 |\n"
                        "    +---+---+---+---+\n"
                        "    | Q | W | E | R |\n"
                        "    +---+---+---+---+\n"
                        "    | A | S | D | F |\n"
                        "    +---+---+---+---+\n"
                        "    | Z | X | C | V |\n"
                        "    +---+---+---+---+"
                    );

                    ImGui::Spacing();
                    ImGui::TextDisabled("Tip: Customize any key mapping in Tools -> Custom Keyboard Mapping (Ctrl+K).");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Super-CHIP Guide")) {
                    ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "Super-CHIP 1.1 Extensions (SCHIP)");
                    ImGui::Separator();
                    ImGui::BulletText("HIGH (00FF): Switches display to 128x64 high resolution.");
                    ImGui::BulletText("LOW  (00FE): Reverts display to 64x32 standard resolution.");
                    ImGui::BulletText("DRW Vx, Vy, 0 (DXY0): Draws a large 16x16 sprite (32 bytes).");
                    ImGui::BulletText("SCD N (00CN): Hardware scrolls the display down by N lines.");
                    ImGui::BulletText("SCR   (00FB): Hardware scrolls the display right by 4 pixels.");
                    ImGui::BulletText("SCL   (00FC): Hardware scrolls the display left by 4 pixels.");
                    ImGui::BulletText("EXIT  (00FD): Halts the program execution gracefully.");
                    ImGui::BulletText("LD HF, Vx (Fx30): Sets index I to 10-byte high-res font digit.");
                    ImGui::BulletText("LD R, Vx  (Fx75): Saves V0..Vx to HP-48 RPL user flags.");
                    ImGui::BulletText("LD Vx, R  (Fx85): Restores V0..Vx from HP-48 RPL user flags.");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Close Help (Esc)", ImVec2(140, 30))) {
                state.show_shortcuts = false;
            }
        }
        ImGui::End();
    }
}

#endif
