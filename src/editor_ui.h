#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include "imgui.h"
#include "chip8.h"
#include "assembler.h"
#include <string>
#include <vector>
#include <cstring>

struct CodeEditorState {
    bool show_editor = false;
    char text_buffer[65536];
    int selected_template = 0;
    std::string status_msg = "Ready. Write or select a template, then click Assemble & Run.";
    bool status_is_error = false;
    std::vector<AssemblerError> last_errors;

    CodeEditorState() {
        std::memset(text_buffer, 0, sizeof(text_buffer));
        std::string initial_code = Chip8Assembler::get_template_code(0);
        std::strncpy(text_buffer, initial_code.c_str(), sizeof(text_buffer) - 1);
    }

    void load_template(int idx) {
        selected_template = idx;
        std::string code = Chip8Assembler::get_template_code(idx);
        std::strncpy(text_buffer, code.c_str(), sizeof(text_buffer) - 1);
        status_msg = "Loaded template: " + std::string(get_template_name(idx));
        status_is_error = false;
        last_errors.clear();
    }

    static const char* get_template_name(int idx) {
        switch (idx) {
            case 0: return "Super-CHIP 16x16 Sprite Demo";
            case 1: return "Bouncing Square Animation";
            case 2: return "Interactive Keypad & Font Display";
            case 3: return "Super-CHIP RPL Flags Test";
            default: return "Custom";
        }
    }
};

inline void render_code_editor_ui(
    Chip8& chip8,
    CodeEditorState& editor_state,
    bool& paused,
    bool& debugger_open,
    std::string& current_rom_name
) {
    float disp_w = ImGui::GetIO().DisplaySize.x;
    float disp_h = ImGui::GetIO().DisplaySize.y;
    float ew = std::max(600.0f, disp_w - 40.0f);
    float eh = std::max(400.0f, disp_h - 45.0f);
    float ex = (disp_w - ew) * 0.5f;
    float ey = 35.0f;

    ImGui::SetNextWindowSize(ImVec2(ew, eh), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(ex, ey), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if (ImGui::Begin("CHIP-8 & Super-CHIP Code Editor / Assembler##window", &editor_state.show_editor, flags)) {
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f));
        if (ImGui::Button("  Assemble & Run  ")) {
            AssemblyResult res = Chip8Assembler::assemble(editor_state.text_buffer);
            editor_state.last_errors = res.errors;
            if (res.success) {
                chip8.load_from_bytes(res.bytecode.data(), res.bytecode.size(), "editor_program.ch8");
                current_rom_name = "editor_program.ch8";
                paused = false;
                editor_state.show_editor = false;
                editor_state.status_msg = "Program assembled (" + std::to_string(res.bytecode.size()) + " bytes) and running!";
                editor_state.status_is_error = false;
            } else {
                editor_state.status_msg = res.summary;
                editor_state.status_is_error = true;
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.75f, 1.0f));
        if (ImGui::Button("Assemble & Step into Debugger")) {
            AssemblyResult res = Chip8Assembler::assemble(editor_state.text_buffer);
            editor_state.last_errors = res.errors;
            if (res.success) {
                chip8.load_from_bytes(res.bytecode.data(), res.bytecode.size(), "editor_program.ch8");
                current_rom_name = "editor_program.ch8";
                paused = true;
                editor_state.show_editor = false;
                debugger_open = true;
                editor_state.status_msg = "Program loaded into VM at PC 0x0200. Debugger opened!";
                editor_state.status_is_error = false;
            } else {
                editor_state.status_msg = res.summary;
                editor_state.status_is_error = true;
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        const char* template_names[] = {
            "0: SCHIP 16x16 Sprite Demo",
            "1: Bouncing Square",
            "2: Keypad & Font Display",
            "3: SCHIP RPL Flags Test"
        };
        if (ImGui::Combo("##template_select", &editor_state.selected_template, template_names, 4)) {
            editor_state.load_template(editor_state.selected_template);
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            editor_state.text_buffer[0] = '\0';
            editor_state.last_errors.clear();
            editor_state.status_msg = "Editor cleared.";
            editor_state.status_is_error = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("Close (Esc)")) {
            editor_state.show_editor = false;
        }

        ImGui::Separator();

        float footer_height = editor_state.last_errors.empty() ? 40.0f : 120.0f;
        ImVec2 text_size(-1.0f, ImGui::GetContentRegionAvail().y - footer_height);

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        ImGui::InputTextMultiline("##editor_text", editor_state.text_buffer, sizeof(editor_state.text_buffer), text_size, flags);

        ImGui::Separator();
        if (editor_state.status_is_error) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", editor_state.status_msg.c_str());
            if (!editor_state.last_errors.empty()) {
                ImGui::BeginChild("errorlist", ImVec2(0, 70), true);
                for (const auto& err : editor_state.last_errors) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "  Line %d: %s", err.line_number, err.message.c_str());
                }
                ImGui::EndChild();
            }
        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "Status: %s", editor_state.status_msg.c_str());
        }
    }
    ImGui::End();
}

#endif 
