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
    
    std::string status_msg = "ready";
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
        if (ImGui::BeginMenu("file")) {
            if (ImGui::MenuItem("open", "ctrl+o")) {
                state.show_rom_browser = true;
                rom_browser_state.refresh();
            }

            if (!rom_browser_state.recent_roms.empty()) {
                if (ImGui::BeginMenu("recent roms")) {
                    for (const auto& rpath : rom_browser_state.recent_roms) {
                        std::string rname = fs::path(rpath).filename().string();
                        if (ImGui::MenuItem(rname.c_str())) {
                            rom_filename = rpath;
                            rom_load_requested = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("quick save", "f5 / ctrl+s", false, !rom_filename.empty())) {
                std::string path = get_savestate_path(rom_filename, current_slot);
                if (chip8.save_state(path)) {
                    state.set_status("saved state to slot " + std::to_string(current_slot));
                } else {
                    state.set_status("failed to save state", true);
                }
            }
            if (ImGui::MenuItem("quick load", "f6 / ctrl+l", false, !rom_filename.empty())) {
                std::string path = get_savestate_path(rom_filename, current_slot);
                if (chip8.load_state(path)) {
                    state.set_status("loaded state from slot " + std::to_string(current_slot));
                } else {
                    state.set_status("slot " + std::to_string(current_slot) + " not found", true);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("reload rom", "ctrl+r", false, !rom_filename.empty())) {
                chip8.reset();
                chip8.load_rom(rom_filename);
                state.set_status("vm reset and rom reloaded");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("quit", "esc")) {
                running = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("emulation")) {
            if (chip8.is_extended_mode()) {
                ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "mode: super-chip (128x64)");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "mode: standard chip-8 (64x32)");
            }
            ImGui::Separator();
            if (ImGui::MenuItem(paused ? "resume" : "pause", "space / p", paused)) {
                paused = !paused;
                state.set_status(paused ? "paused" : "resumed");
            }
            if (ImGui::MenuItem("step 1 frame", ".", false, paused)) {
                step_one_frame = true;
                state.set_status("stepped 1 frame");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("speed: 120 hz (slow)")) { cycles_per_frame = 2; }
            if (ImGui::MenuItem("speed: 300 hz"))        { cycles_per_frame = 5; }
            if (ImGui::MenuItem("speed: 600 hz (normal)")) { cycles_per_frame = 10; }
            if (ImGui::MenuItem("speed: 1200 hz (2x)"))  { cycles_per_frame = 20; }
            if (ImGui::MenuItem("speed: 1800 hz (schip)")) { cycles_per_frame = 30; }
            if (ImGui::MenuItem("speed: 3000 hz (turbo)")) { cycles_per_frame = 50; }
            ImGui::Separator();

            if (ImGui::BeginMenu("compatibility & quirks")) {
                if (ImGui::MenuItem("preset: super-chip 1.1 (schip)")) {
                    chip8.set_quirk_shift_vx(true);
                    chip8.set_quirk_load_store_no_i(true);
                    chip8.set_quirk_jump_vx(true);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(false);
                    cycles_per_frame = 30;
                    state.set_status("applied super-chip 1.1 quirks (1800 hz)");
                }
                if (ImGui::MenuItem("preset: modern xo-chip / octo")) {
                    chip8.set_quirk_shift_vx(true);
                    chip8.set_quirk_load_store_no_i(true);
                    chip8.set_quirk_jump_vx(false);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(false);
                    cycles_per_frame = 20;
                    state.set_status("applied modern xo-chip quirks (1200 hz)");
                }
                if (ImGui::MenuItem("preset: classic cosmac vip (1977)")) {
                    chip8.set_quirk_shift_vx(false);
                    chip8.set_quirk_load_store_no_i(false);
                    chip8.set_quirk_jump_vx(false);
                    chip8.set_quirk_legacy_scroll(false);
                    chip8.set_quirk_vf_reset(true);
                    cycles_per_frame = 10;
                    state.set_status("applied cosmac vip quirks (600 hz)");
                }
                ImGui::Separator();

                bool q_shift = chip8.get_quirk_shift_vx();
                if (ImGui::MenuItem("shift in-place (vx >>= 1) [schip]", nullptr, q_shift)) {
                    chip8.set_quirk_shift_vx(!q_shift);
                }
                bool q_load = chip8.get_quirk_load_store_no_i();
                if (ImGui::MenuItem("memory i unchanged on fx55/fx65 [schip]", nullptr, q_load)) {
                    chip8.set_quirk_load_store_no_i(!q_load);
                }
                bool q_jump = chip8.get_quirk_jump_vx();
                if (ImGui::MenuItem("jump with offset bxnn [schip]", nullptr, q_jump)) {
                    chip8.set_quirk_jump_vx(!q_jump);
                }
                bool q_scroll = chip8.get_quirk_legacy_scroll();
                if (ImGui::MenuItem("legacy half-line scrolling [hp-48]", nullptr, q_scroll)) {
                    chip8.set_quirk_legacy_scroll(!q_scroll);
                }
                bool q_vf = chip8.get_quirk_vf_reset();
                if (ImGui::MenuItem("reset vf on 8xy1-3 bitwise ops [vip]", nullptr, q_vf)) {
                    chip8.set_quirk_vf_reset(!q_vf);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("view")) {
            ImGui::MenuItem("show all ui overlay", "ctrl+u", &state.show_overlay);
            ImGui::Separator();
            ImGui::MenuItem("control panel", "f2", &state.show_control_panel);
            ImGui::MenuItem("cpu registers & timers inspector", "f3", &state.show_inspector);
            ImGui::MenuItem("virtual keypad", "f4", &state.show_keypad);
            ImGui::MenuItem("sound synthesizer panel", "f8", &state.show_audio_panel);
            ImGui::Separator();
            ImGui::MenuItem("disassembler / debugger window", "f12 / ctrl+d", &debugger_state.show_debugger);
            ImGui::MenuItem("chip-8 code editor & assembler", "ctrl+e", &editor_state.show_editor);
            ImGui::MenuItem("custom keyboard mapping", "ctrl+k", &keymap_ui_state.show_keymap_window);
            ImGui::MenuItem("rom file browser", "ctrl+o", &state.show_rom_browser);
            ImGui::Separator();
            if (ImGui::MenuItem("show all floating panels")) {
                state.show_control_panel = true;
                state.show_inspector = true;
                state.show_keypad = true;
                state.show_audio_panel = true;
            }
            if (ImGui::MenuItem("hide all floating panels")) {
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

        if (ImGui::BeginMenu("debug")) {
            ImGui::MenuItem("disassembler/debugger window", "f12 / ctrl+d", &debugger_state.show_debugger);
            ImGui::Separator();
            if (ImGui::MenuItem(paused ? "continue execution" : "pause execution", "space / p")) {
                paused = !paused;
                state.set_status(paused ? "paused" : "resumed");
            }
            if (ImGui::MenuItem("step single cycle", "f10", false, paused)) {
                debugger_state.step_instruction = true;
            }
            if (ImGui::MenuItem("step over subroutine", "shift+f10", false, paused)) {
                debugger_state.step_over = true;
            }
            if (ImGui::MenuItem("step 1 frame", "f11 / .", false, paused)) {
                step_one_frame = true;
                state.set_status("stepped 1 frame");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("toggle breakpoint at pc", "f9")) {
                debugger_state.toggle_breakpoint(chip8.get_pc());
                char bmsg[64];
                std::snprintf(bmsg, sizeof(bmsg), "toggled breakpoint at 0x%04x", chip8.get_pc());
                state.set_status(bmsg);
            }
            if (ImGui::MenuItem("clear all breakpoints", nullptr, false, !debugger_state.breakpoints.empty())) {
                debugger_state.clear_breakpoints();
                state.set_status("cleared all breakpoints");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("tools")) {
            ImGui::MenuItem("chip-8 code editor & assembler", "ctrl+e", &editor_state.show_editor);
            ImGui::MenuItem("custom keyboard mapping", "ctrl+k", &keymap_ui_state.show_keymap_window);
            ImGui::Separator();
            ImGui::MenuItem("sound synthesizer panel", "f8", &state.show_audio_panel);
            ImGui::MenuItem("control panel", "f2", &state.show_control_panel);
            ImGui::MenuItem("cpu registers inspector", "f3", &state.show_inspector);
            ImGui::MenuItem("virtual keypad", "f4", &state.show_keypad);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("color themes")) {
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
                    state.set_status("theme: " + active_palette.name);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("help")) {
            if (ImGui::MenuItem("keyboard controls & shortcuts", "h / f1")) {
                state.show_shortcuts = true;
            }
            ImGui::EndMenu();
        }

        
        float right_indent = ImGui::GetWindowWidth() - 410.0f;
        if (right_indent > 250.0f) {
            ImGui::SameLine(right_indent);
            if (chip8.is_extended_mode()) {
                ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "[schip 128x64]");
            } else {
                ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "[chip-8 64x32]");
            }
            ImGui::SameLine();
            ImGui::Text("| %d hz", cycles_per_frame * 60);
            ImGui::SameLine();
            ImGui::Text("| slot:%d", current_slot);
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

    
    bool modal_or_tool_active = debugger_state.show_debugger || editor_state.show_editor || 
                                state.show_rom_browser || keymap_ui_state.show_keymap_window || 
                                state.show_shortcuts;

    if (modal_or_tool_active) {
        
        
    } else if (!state.show_overlay) {
        
        float disp_w = ImGui::GetIO().DisplaySize.x;
        float disp_h = ImGui::GetIO().DisplaySize.y;
        float scr_w = disp_w - 40.0f;
        float scr_h = disp_h - 45.0f;
        float scr_x = 20.0f;
        float scr_y = 35.0f;

        ImGui::SetNextWindowSize(ImVec2(scr_w, scr_h), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(scr_x, scr_y), ImGuiCond_Always);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar;

        std::string title = "chip-8 emulation display";
        if (!rom_filename.empty()) title += " - " + fs::path(rom_filename).filename().string();
        title += chip8.is_extended_mode() ? " [schip 128x64]" : " [chip-8 64x32]";
        if (paused) title += " [paused]";
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

            ImU32 col_bg = IM_COL32(active_palette.bg.r, active_palette.bg.g, active_palette.bg.b, 255);
            ImU32 col_fg = IM_COL32(active_palette.fg.r, active_palette.fg.g, active_palette.fg.b, 255);
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

            std::string title = "emulation screen";
            if (!rom_filename.empty()) title += " - " + fs::path(rom_filename).filename().string();
            title += chip8.is_extended_mode() ? " [schip 128x64]" : " [chip-8 64x32]";
            if (paused) title += " [paused]";
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

                    ImU32 col_bg = IM_COL32(active_palette.bg.r, active_palette.bg.g, active_palette.bg.b, 255);
                    ImU32 col_fg = IM_COL32(active_palette.fg.r, active_palette.fg.g, active_palette.fg.b, 255);
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
                        const char* msg = "no rom loaded. press ctrl+o to open rom browser.";
                        ImVec2 txt_sz = ImGui::CalcTextSize(msg);
                        ImVec2 txt_pos(offset_x + (draw_w - txt_sz.x) * 0.5f, offset_y + (draw_h - txt_sz.y) * 0.5f);
                        draw_list->AddRectFilled(ImVec2(txt_pos.x - 8, txt_pos.y - 4), ImVec2(txt_pos.x + txt_sz.x + 8, txt_pos.y + txt_sz.y + 4), IM_COL32(0, 0, 0, 200), 4.0f);
                        draw_list->AddText(txt_pos, IM_COL32(0, 230, 210, 255), msg);
                    } else if (paused) {
                        const char* pmsg = "paused";
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

            if (ImGui::Begin("chip-8 controls##overlay", &state.show_control_panel, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
                ImGui::TextDisabled("status & speed");
                if (paused) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.60f, 0.30f, 0.9f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.75f, 0.38f, 1.0f));
                    if (ImGui::Button("  resume execution  ", ImVec2(-1, 24))) {
                        paused = false;
                        state.set_status("resumed");
                    }
                    ImGui::PopStyleColor(2);

                    if (ImGui::Button("step 1 frame (.)", ImVec2(-1, 20))) {
                        step_one_frame = true;
                        state.set_status("stepped 1 frame");
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.45f, 0.10f, 0.9f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.55f, 0.15f, 1.0f));
                    if (ImGui::Button("pause execution", ImVec2(-1, 24))) {
                        paused = true;
                        state.set_status("paused");
                    }
                    ImGui::PopStyleColor(2);
                }

                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##speed", &cycles_per_frame, 1, 100, "speed: %d cyc/f (60hz)");

                ImGui::Separator();

                
                ImGui::TextDisabled("savestate slots (1 - 5):");
                float slot_w = (ImGui::GetContentRegionAvail().x - 4 * 6.0f) / 5.0f;
                for (int s = 1; s <= 5; s++) {
                    if (s > 1) ImGui::SameLine(0.0f, 6.0f);
                    char slot_lbl[16];
                    std::snprintf(slot_lbl, sizeof(slot_lbl), current_slot == s ? "[slot %d]" : "slot %d", s);
                    if (current_slot == s) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.80f, 1.0f));
                    }
                    if (ImGui::Button(slot_lbl, ImVec2(slot_w, 22))) {
                        current_slot = s;
                        state.set_status("selected slot " + std::to_string(current_slot));
                    }
                    if (current_slot == s) {
                        ImGui::PopStyleColor();
                    }
                }

                std::string slot_filename = get_savestate_path(rom_filename, current_slot);
                float half_w = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;

                if (ImGui::Button(" save state ", ImVec2(half_w, 24))) {
                    if (chip8.save_state(slot_filename)) {
                        state.set_status("state saved to " + slot_filename);
                    } else {
                        state.set_status("error saving state", true);
                    }
                }
                ImGui::SameLine(0.0f, 6.0f);
                if (ImGui::Button(" load state ", ImVec2(half_w, 24))) {
                    if (chip8.load_state(slot_filename)) {
                        state.set_status("state loaded from " + slot_filename);
                    } else {
                        state.set_status("slot " + std::to_string(current_slot) + " not found", true);
                    }
                }

                ImGui::Separator();
                ImGui::TextDisabled("color theme:");
                const char* current_theme_name = state.use_custom_colors ? "custom color palette" : PALETTES[current_palette_idx].name.c_str();
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
                            state.set_status("applied theme: " + active_palette.name);
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                
                float color_w = (ImGui::GetContentRegionAvail().x - 6.0f) * 0.5f;
                ImGui::SetNextItemWidth(color_w - 28.0f);
                if (ImGui::ColorEdit3("bg##custom", state.custom_bg, ImGuiColorEditFlags_NoInputs)) {
                    state.use_custom_colors = true;
                    active_palette.bg = {
                        (uint8_t)(state.custom_bg[0] * 255),
                        (uint8_t)(state.custom_bg[1] * 255),
                        (uint8_t)(state.custom_bg[2] * 255)
                    };
                }
                ImGui::SameLine(0.0f, 6.0f);
                ImGui::SetNextItemWidth(color_w - 28.0f);
                if (ImGui::ColorEdit3("fg##custom", state.custom_fg, ImGuiColorEditFlags_NoInputs)) {
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

            if (ImGui::Begin("sound editor##overlay", &state.show_audio_panel, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
                
                ImGui::Checkbox("master audio", &audio_config.sound_enabled);
                ImGui::SameLine();
                if (audio_config.sound_enabled) {
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "[active]");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[muted]");
                }

                
                ImGui::SetNextItemWidth(-1);
                if (ImGui::BeginCombo("##waveform", WAVEFORM_NAMES[audio_config.waveform_type])) {
                    for (int i = 0; i < WAVEFORM_COUNT; i++) {
                        bool is_sel = (audio_config.waveform_type == i);
                        if (ImGui::Selectable(WAVEFORM_NAMES[i], is_sel)) {
                            audio_config.waveform_type = i;
                            state.set_status(std::string("waveform: ") + WAVEFORM_NAMES[i]);
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

                draw_wave_btn("sine", WAVEFORM_SINE, 0);
                draw_wave_btn("square", WAVEFORM_SQUARE, 1);
                draw_wave_btn("saw", WAVEFORM_SAWTOOTH, 2);
                draw_wave_btn("tri", WAVEFORM_TRIANGLE, 3);
                draw_wave_btn("noise", WAVEFORM_NOISE, 4);

                
                float preview_points[100];
                generate_waveform_preview(audio_config, preview_points, 100);
                char overlay_plot_text[64];
                std::snprintf(overlay_plot_text, sizeof(overlay_plot_text), "%s (%.0f hz)", WAVEFORM_NAMES[audio_config.waveform_type], audio_config.waveform_freq);
                ImGui::PlotLines("##oscilloscope", preview_points, 100, 0, overlay_plot_text, -1.0f, 1.0f, ImVec2(-1, 48));

                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##freq", &audio_config.waveform_freq, 60.0f, 2000.0f, "freq: %.0f hz");
                
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##vol", &audio_config.volume, 0.0f, 1.0f, "vol: %.0f%%");

                
                ImGui::PushStyleColor(ImGuiCol_Button, audio_config.test_tone_active ? ImVec4(0.85f, 0.20f, 0.20f, 1.0f) : ImVec4(0.18f, 0.45f, 0.70f, 1.0f));
                ImGui::Button("  hold mouse to test audio tone  ", ImVec2(-1, 24));
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

            if (ImGui::Begin("cpu registers & timers##overlay", &state.show_inspector, flags)) {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 3.0f));
                
                ImGui::Text("pc: 0x%03x  |  i: 0x%03x  |  sp: %d", chip8.get_pc(), chip8.get_index(), chip8.get_sp());
                ImGui::Text("opcode: 0x%04x", chip8.get_opcode());

                
                float delay_pct = chip8.get_delay_timer() / 255.0f;
                char delay_str[32];
                std::snprintf(delay_str, sizeof(delay_str), "delay: %d", chip8.get_delay_timer());
                ImGui::ProgressBar(delay_pct, ImVec2(-1, 12), delay_str);

                float sound_pct = chip8.get_sound_timer() / 255.0f;
                char sound_str[32];
                std::snprintf(sound_str, sizeof(sound_str), "sound: %d %s", chip8.get_sound_timer(), (chip8.get_sound_timer() > 0 ? "[beep]" : ""));
                if (chip8.get_sound_timer() > 0) {
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.4f, 0.2f, 1.0f));
                }
                ImGui::ProgressBar(sound_pct, ImVec2(-1, 12), sound_str);
                if (chip8.get_sound_timer() > 0) {
                    ImGui::PopStyleColor();
                }

                ImGui::Separator();
                ImGui::TextDisabled("registers v0 - vf:");
                if (ImGui::BeginTable("reg_table", 4, ImGuiTableFlags_BordersInnerV)) {
                    for (int i = 0; i < 16; i++) {
                        if (i % 4 == 0) ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(i % 4);
                        uint8_t val = chip8.get_v(i);
                        if (i == 0xF && val > 0) {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "v%x:%02x", i, val);
                        } else {
                            ImGui::Text("v%x:%02x", i, val);
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Separator();
                ImGui::TextDisabled("rpl user flags r0 - rf:");
                if (ImGui::BeginTable("rpl_table", 4, ImGuiTableFlags_BordersInnerV)) {
                    for (int i = 0; i < 16; i++) {
                        if (i % 4 == 0) ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(i % 4);
                        uint8_t val = chip8.get_rpl_flag(i);
                        ImGui::Text("r%x:%02x", i, val);
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

            if (ImGui::Begin("keypad##overlay", &state.show_keypad, flags)) {
                static const int keypad_layout[4][4] = {
                    {0x1, 0x2, 0x3, 0xC},
                    {0x4, 0x5, 0x6, 0xD},
                    {0x7, 0x8, 0x9, 0xE},
                    {0xA, 0x0, 0xB, 0xF}
                };

                static const char* key_labels[16] = {
                    "0\n(x)", "1\n(1)", "2\n(2)", "3\n(3)",
                    "4\n(q)", "5\n(w)", "6\n(e)", "7\n(a)",
                    "8\n(s)", "9\n(d)", "a\n(z)", "b\n(c)",
                    "c\n(4)", "d\n(r)", "e\n(f)", "f\n(v)"
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
        if (ImGui::Begin("keyboard controls & shortcuts##modal", &state.show_shortcuts, flags)) {
            ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "chip-8 & super-chip emulator guide");
            ImGui::Separator();

            if (ImGui::BeginTabBar("helptabs")) {
                if (ImGui::BeginTabItem("keyboard shortcuts")) {
                    ImGui::TextDisabled("emulation hotkeys:");
                    if (ImGui::BeginTable("helptable1", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("shortcut key", ImGuiTableColumnFlags_WidthFixed, 180);
                        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto add_row = [](const char* key, const char* desc) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", key);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", desc);
                        };

                        add_row("space / p", "pause or resume emulation");
                        add_row(". / n", "step exactly 1 frame (60hz) when paused");
                        add_row("+ / up / ]", "increase emulation speed (+60 hz / +1 cycle)");
                        add_row("- / down / [", "decrease emulation speed (-60 hz / -1 cycle)");
                        add_row("pageup / pagedn", "fast speed jump (+/- 300 hz)");
                        add_row("0 / backspace", "reset speed to default 600 hz (10 cycles/frame)");
                        add_row("tab / t", "cycle next color theme palette");
                        add_row("shift + tab", "cycle previous color theme palette");
                        add_row("f5 / ctrl + s", "quick save state to selected slot (1-5)");
                        add_row("f6 / ctrl + l", "quick load state from selected slot");
                        add_row("f7", "switch savestate slot (1 to 5)");
                        add_row("ctrl + r", "reset virtual machine & reload rom");
                        add_row("ctrl + u", "toggle full ui overlay on / off");
                        add_row("esc", "close active dialog or exit emulator");
                        ImGui::EndTable();
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextDisabled("debugger & tools hotkeys:");
                    if (ImGui::BeginTable("helptable2", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("shortcut key", ImGuiTableColumnFlags_WidthFixed, 180);
                        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        auto add_row = [](const char* key, const char* desc) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "%s", key);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", desc);
                        };

                        add_row("f12 / ctrl + d", "open / close debugger & disassembler");
                        add_row("f10", "step single cpu instruction (cycle)");
                        add_row("shift + f10", "step over subroutine (call / 2nnn)");
                        add_row("f11", "step single frame");
                        add_row("f9", "toggle breakpoint at current program counter");
                        add_row("ctrl + e", "open / close chip-8 code editor & assembler");
                        add_row("ctrl + k", "open / close custom keypad mapping");
                        add_row("ctrl + o", "open rom file browser");
                        add_row("f2", "toggle chip-8 controls panel");
                        add_row("f3", "toggle cpu registers & timers inspector");
                        add_row("f4", "toggle virtual keypad panel");
                        add_row("f8", "toggle sound synthesizer panel");
                        add_row("h / f1", "toggle this help & controls window");
                        ImGui::EndTable();
                    }
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("hex keypad layout")) {
                    ImGui::TextDisabled("chip-8 hex keypad (16 keys) vs pc keyboard mapping:");
                    ImGui::Spacing();

                    ImGui::Text("standard cosmac vip keypad:");
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
                        "    +---+---+---+---+\n"
                        "    | 1 | 2 | 3 | c |\n"
                        "    +---+---+---+---+\n"
                        "    | 4 | 5 | 6 | d |\n"
                        "    +---+---+---+---+\n"
                        "    | 7 | 8 | 9 | e |\n"
                        "    +---+---+---+---+\n"
                        "    | a | 0 | b | f |\n"
                        "    +---+---+---+---+"
                    );

                    ImGui::Spacing();
                    ImGui::Text("default pc keyboard mapping (qwerty):");
                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f),
                        "    +---+---+---+---+\n"
                        "    | 1 | 2 | 3 | 4 |\n"
                        "    +---+---+---+---+\n"
                        "    | q | w | e | r |\n"
                        "    +---+---+---+---+\n"
                        "    | a | s | d | f |\n"
                        "    +---+---+---+---+\n"
                        "    | z | x | c | v |\n"
                        "    +---+---+---+---+"
                    );

                    ImGui::Spacing();
                    ImGui::TextDisabled("tip: customize any key mapping in tools -> custom keyboard mapping (ctrl+k).");
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("super-chip guide")) {
                    ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "super-chip 1.1 extensions (schip)");
                    ImGui::Separator();
                    ImGui::BulletText("high (00ff): switches display to 128x64 high resolution.");
                    ImGui::BulletText("low  (00fe): reverts display to 64x32 standard resolution.");
                    ImGui::BulletText("drw vx, vy, 0 (dxy0): draws a large 16x16 sprite (32 bytes).");
                    ImGui::BulletText("scd n (00cn): hardware scrolls the display down by n lines.");
                    ImGui::BulletText("scr   (00fb): hardware scrolls the display right by 4 pixels.");
                    ImGui::BulletText("scl   (00fc): hardware scrolls the display left by 4 pixels.");
                    ImGui::BulletText("exit  (00fd): halts the program execution gracefully.");
                    ImGui::BulletText("ld hf, vx (fx30): sets index i to 10-byte high-res font digit.");
                    ImGui::BulletText("ld r, vx  (fx75): saves v0..vx to hp-48 rpl user flags.");
                    ImGui::BulletText("ld vx, r  (fx85): restores v0..vx from hp-48 rpl user flags.");
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("close help (esc)", ImVec2(140, 30))) {
                state.show_shortcuts = false;
            }
        }
        ImGui::End();
    }
}

#endif 
