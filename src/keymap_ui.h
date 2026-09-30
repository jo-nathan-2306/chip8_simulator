#ifndef KEYMAP_UI_H
#define KEYMAP_UI_H

#include "imgui.h"
#include "keymap.h"
#include <cstdio>
#include <string>

struct KeymapUIState {
    bool show_keymap_window = false;
    int rebinding_key_idx = -1; 
    std::string status_info = "click any key to rebind to your keyboard.";
};

inline void render_keymap_ui(
    KeyMapping& keymap,
    KeymapUIState& state
) {
    if (!state.show_keymap_window) return;

    float disp_w = ImGui::GetIO().DisplaySize.x;
    float disp_h = ImGui::GetIO().DisplaySize.y;
    float kw = std::min(disp_w - 40.0f, 680.0f);
    float kh = std::min(disp_h - 50.0f, 600.0f);
    float kx = (disp_w - kw) * 0.5f;
    float ky = 35.0f;

    ImGui::SetNextWindowSize(ImVec2(kw, kh), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(kx, ky), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("keyboard mapping configuration##window", &state.show_keymap_window, flags)) {
        ImGui::TextDisabled("presets:");
        if (ImGui::Button("classic qwerty")) {
            keymap.set_preset_qwerty();
            keymap.save_to_file("keymap.cfg");
            state.status_info = "applied classic qwerty layout (1-4, q-r, a-f, z-v).";
        }
        ImGui::SameLine();
        if (ImGui::Button("numeric keypad")) {
            keymap.set_preset_numpad();
            keymap.save_to_file("keymap.cfg");
            state.status_info = "applied numpad layout.";
        }
        ImGui::SameLine();
        if (ImGui::Button("wasd gamer")) {
            keymap.set_preset_wasd();
            keymap.save_to_file("keymap.cfg");
            state.status_info = "applied wasd layout (w=up, a=left, s=down, d=right).";
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (state.rebinding_key_idx != -1) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.2f, 1.0f));
            char hex_char = (state.rebinding_key_idx < 10) ? ('0' + state.rebinding_key_idx) : ('a' + (state.rebinding_key_idx - 10));
            ImGui::Text(">> press any key on your keyboard to bind to key '%c' <<", hex_char);
            ImGui::PopStyleColor();
            ImGui::SameLine();
            if (ImGui::SmallButton("cancel")) {
                state.rebinding_key_idx = -1;
                keymap.awaiting_rebind_idx = -1;
            }
        } else {
            ImGui::TextDisabled("click any key below to rebind to a custom key:");
        }

        ImGui::Spacing();

        
        static const int keypad_grid[4][4] = {
            {0x1, 0x2, 0x3, 0xC},
            {0x4, 0x5, 0x6, 0xD},
            {0x7, 0x8, 0x9, 0xE},
            {0xA, 0x0, 0xB, 0xF}
        };

        float btn_w = (ImGui::GetContentRegionAvail().x - 3 * 8.0f) / 4.0f;

        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                if (col > 0) ImGui::SameLine(0.0f, 8.0f);
                int k = keypad_grid[row][col];
                char hex_c = (k < 10) ? ('0' + k) : ('a' + (k - 10));

                std::string bound_name = KeyMapping::get_key_name(keymap.keys[k]);
                char btn_text[64];
                bool is_rebinding = (state.rebinding_key_idx == k);
                if (is_rebinding) {
                    std::snprintf(btn_text, sizeof(btn_text), "[ %c ]\n<press>", hex_c);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
                } else {
                    std::snprintf(btn_text, sizeof(btn_text), "key %c\n(%s)", hex_c, bound_name.c_str());
                }

                if (ImGui::Button(btn_text, ImVec2(btn_w, 48))) {
                    state.rebinding_key_idx = k;
                    keymap.awaiting_rebind_idx = k;
                    state.status_info = "press any key on your keyboard now...";
                }

                if (is_rebinding) {
                    ImGui::PopStyleColor();
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        
        ImGui::TextDisabled("current key mappings table:");
        if (ImGui::BeginTable("keymaplisttable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("chip-8 key", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("bound pc key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableHeadersRow();

            for (int i = 0; i < 16; i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                char hex_c = (i < 10) ? ('0' + i) : ('a' + (i - 10));
                ImGui::Text("key 0x%c", hex_c);

                ImGui::TableNextColumn();
                std::string name = KeyMapping::get_key_name(keymap.keys[i]);
                if (state.rebinding_key_idx == i) {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "listening for keypress...");
                } else {
                    ImGui::Text("%s", name.c_str());
                }

                ImGui::TableNextColumn();
                char btn_id[32];
                std::snprintf(btn_id, sizeof(btn_id), "rebind##%d", i);
                if (ImGui::SmallButton(btn_id)) {
                    state.rebinding_key_idx = i;
                    keymap.awaiting_rebind_idx = i;
                }
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.5f, 1.0f), "status: %s", state.status_info.c_str());
    }
    ImGui::End();
}

#endif 
