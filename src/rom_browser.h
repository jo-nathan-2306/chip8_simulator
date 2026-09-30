#ifndef ROM_BROWSER_H
#define ROM_BROWSER_H

#include "imgui.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct RomFileInfo {
    std::string filename;
    std::string full_path;
    uintmax_t file_size = 0;
    bool is_directory = false;
    bool is_schip = false;
    std::string format_badge; 
    std::string size_str;
    std::string modified_str;
};


inline bool detect_schip_rom(const std::string& path) {
    
    std::string ext = path.substr(path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == "sc8" || ext == "schip") return true;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    std::vector<uint8_t> buffer(4096);
    file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    std::streamsize bytes_read = file.gcount();
    file.close();

    if (bytes_read < 2) return false;

    
    for (std::streamsize i = 0; i + 1 < bytes_read; i += 2) {
        uint16_t op = (static_cast<uint16_t>(buffer[i]) << 8) | buffer[i + 1];

        
        if ((op & 0xFFF0) >= 0x00C0 && (op & 0xFFF0) <= 0x00CF) return true;
        
        if (op == 0x00FB) return true;
        
        if (op == 0x00FC) return true;
        
        if (op == 0x00FD) return true;
        
        if (op == 0x00FE) return true;
        
        if (op == 0x00FF) return true;
        
        if ((op & 0xF00F) == 0xD000) return true;
        
        if ((op & 0xF0FF) == 0xF030) return true;
        
        if ((op & 0xF0FF) == 0xF075 || (op & 0xF0FF) == 0xF085) return true;
    }

    return false;
}

struct RomBrowserState {
    fs::path current_dir = "roms";
    std::vector<RomFileInfo> entries;
    int selected_idx = -1;
    char search_filter[128] = "";
    int type_filter_idx = 0; 
    std::vector<std::string> recent_roms;
    std::string error_message = "";
    bool initialized = false;

    void add_recent(const std::string& path) {
        
        recent_roms.erase(std::remove(recent_roms.begin(), recent_roms.end(), path), recent_roms.end());
        recent_roms.insert(recent_roms.begin(), path);
        if (recent_roms.size() > 8) {
            recent_roms.resize(8);
        }
    }

    void refresh() {
        entries.clear();
        selected_idx = -1;
        error_message = "";

        std::error_code ec;
        if (!fs::exists(current_dir, ec) || !fs::is_directory(current_dir, ec)) {
            
            current_dir = fs::current_path(ec);
        }

        try {
            
            std::vector<RomFileInfo> dirs;
            std::vector<RomFileInfo> files;

            for (const auto& entry : fs::directory_iterator(current_dir, ec)) {
                if (ec) continue;

                RomFileInfo info;
                info.filename = entry.path().filename().string();
                info.full_path = entry.path().string();

                if (entry.is_directory(ec)) {
                    info.is_directory = true;
                    info.format_badge = "[DIR]";
                    info.size_str = "<DIR>";
                    dirs.push_back(info);
                } else if (entry.is_regular_file(ec)) {
                    info.is_directory = false;
                    info.file_size = entry.file_size(ec);

                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                    bool is_rom_ext = (ext == ".ch8" || ext == ".sc8" || ext == ".rom" || ext == ".bin" || ext == ".chip8" || ext == ".c8");

                    if (type_filter_idx != 3 && !is_rom_ext) {
                        continue;
                    }

                    info.is_schip = detect_schip_rom(info.full_path);
                    info.format_badge = info.is_schip ? "[SCHIP]" : "[CHIP8]";

                    if (info.file_size < 1024) {
                        info.size_str = std::to_string(info.file_size) + " B";
                    } else {
                        std::ostringstream ss;
                        ss << std::fixed << std::setprecision(1) << (info.file_size / 1024.0) << " KB";
                        info.size_str = ss.str();
                    }

                    files.push_back(info);
                }
            }

            
            auto sort_fn = [](const RomFileInfo& a, const RomFileInfo& b) {
                std::string sa = a.filename;
                std::string sb = b.filename;
                std::transform(sa.begin(), sa.end(), sa.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                std::transform(sb.begin(), sb.end(), sb.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return sa < sb;
            };
            std::sort(dirs.begin(), dirs.end(), sort_fn);
            std::sort(files.begin(), files.end(), sort_fn);

            
            entries.insert(entries.end(), dirs.begin(), dirs.end());
            entries.insert(entries.end(), files.begin(), files.end());

        } catch (const std::exception& e) {
            error_message = e.what();
        }
    }
};



inline bool render_rom_browser_ui(bool* p_open, std::string& out_selected_path, RomBrowserState& state) {
    if (!*p_open) return false;

    if (!state.initialized) {
        state.refresh();
        state.initialized = true;
    }

    bool rom_chosen = false;

    float disp_w = ImGui::GetIO().DisplaySize.x;
    float disp_h = ImGui::GetIO().DisplaySize.y;
    float bw = std::min(disp_w - 40.0f, 880.0f);
    float bh = std::min(disp_h - 45.0f, 600.0f);
    float bx = (disp_w - bw) * 0.5f;
    float by = 35.0f;

    ImGui::SetNextWindowSize(ImVec2(bw, bh), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(bx, by), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (ImGui::Begin("ROM File Browser", p_open, flags)) {

        ImGui::TextDisabled("Location:");
        ImGui::SameLine();
        std::string current_dir_str = state.current_dir.string();
        ImGui::TextColored(ImVec4(0.0f, 0.88f, 0.82f, 1.0f), "%s", current_dir_str.c_str());

        if (ImGui::Button(".. Up One Folder")) {
            std::error_code ec;
            fs::path parent = state.current_dir.parent_path();
            if (!parent.empty() && fs::exists(parent, ec)) {
                state.current_dir = parent;
                state.refresh();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("ROMs/ Folder")) {
            std::error_code ec;
            if (fs::exists("roms", ec)) {
                state.current_dir = "roms";
                state.refresh();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            state.refresh();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 240);
        ImGui::SetNextItemWidth(220);
        ImGui::InputTextWithHint("##search", "Search ROMs...", state.search_filter, sizeof(state.search_filter));

        ImGui::Separator();

        if (!state.recent_roms.empty()) {
            ImGui::TextDisabled("Recent ROMs:");
            ImGui::SameLine();
            for (size_t i = 0; i < state.recent_roms.size() && i < 5; i++) {
                if (i > 0) ImGui::SameLine();
                std::string r_name = fs::path(state.recent_roms[i]).filename().string();
                if (ImGui::SmallButton(r_name.c_str())) {
                    out_selected_path = state.recent_roms[i];
                    state.add_recent(out_selected_path);
                    *p_open = false;
                    rom_chosen = true;
                }
            }
            ImGui::Separator();
        }

        float details_pane_width = 240.0f;
        float list_width = ImGui::GetContentRegionAvail().x - details_pane_width - 10.0f;

        ImGui::BeginChild("##rom_list_child", ImVec2(list_width, -38.0f), true);

        const char* const filter_options[] = { "All Supported ROMs", "Super-CHIP (SCHIP) Only", "CHIP-8 Standard Only", "All Files (*.*)" };
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo("Filter", &state.type_filter_idx, filter_options, 4)) {
            state.refresh();
        }

        if (!state.error_message.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", state.error_message.c_str());
        }

        ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("romtable", 3, table_flags)) {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 65.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableHeadersRow();

            std::string search_query = state.search_filter;
            std::transform(search_query.begin(), search_query.end(), search_query.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            for (size_t i = 0; i < state.entries.size(); i++) {
                const auto& item = state.entries[i];

                if (!search_query.empty()) {
                    std::string lower_name = item.filename;
                    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (lower_name.find(search_query) == std::string::npos) {
                        continue;
                    }
                }

                if (!item.is_directory) {
                    if (state.type_filter_idx == 1 && !item.is_schip) continue;
                    if (state.type_filter_idx == 2 && item.is_schip) continue;
                }

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                if (item.is_directory) {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "[DIR]");
                } else if (item.is_schip) {
                    ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "[SCHIP]");
                } else {
                    ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "[CHIP8]");
                }

                ImGui::TableSetColumnIndex(1);
                bool is_selected = (state.selected_idx == (int)i);
                if (ImGui::Selectable(item.filename.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    state.selected_idx = (int)i;
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    if (item.is_directory) {
                        state.current_dir = item.full_path;
                        state.refresh();
                        break;
                    } else {
                        out_selected_path = item.full_path;
                        state.add_recent(out_selected_path);
                        *p_open = false;
                        rom_chosen = true;
                        break;
                    }
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(item.size_str.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("##rom_details_child", ImVec2(details_pane_width, -38.0f), true);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.24f, 1.0f), "ROM Details");
        ImGui::Separator();

        if (state.selected_idx >= 0 && state.selected_idx < (int)state.entries.size()) {
            const auto& sel = state.entries[state.selected_idx];
            if (sel.is_directory) {
                ImGui::Text("Directory: %s", sel.filename.c_str());
                ImGui::Spacing();
                if (ImGui::Button("Open Directory", ImVec2(-1, 30))) {
                    state.current_dir = sel.full_path;
                    state.refresh();
                }
            } else {
                ImGui::TextWrapped("File: %s", sel.filename.c_str());
                ImGui::Spacing();
                ImGui::Text("Size: %s", sel.size_str.c_str());

                ImGui::Spacing();
                ImGui::Text("Architecture:");
                if (sel.is_schip) {
                    ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "Super-CHIP (SCHIP)");
                    ImGui::BulletText("Extended 128x64 Mode");
                    ImGui::BulletText("16x16 / 8x10 Sprites");
                    ImGui::BulletText("Hardware Scrolling");
                } else {
                    ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "Standard CHIP-8");
                    ImGui::BulletText("Monochrome 64x32");
                    ImGui::BulletText("35 Standard Opcodes");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.55f, 0.32f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.70f, 0.40f, 1.0f));
                if (ImGui::Button("  Load ROM  ", ImVec2(-1, 36))) {
                    out_selected_path = sel.full_path;
                    state.add_recent(out_selected_path);
                    *p_open = false;
                    rom_chosen = true;
                }
                ImGui::PopStyleColor(2);
            }
        } else {
            ImGui::TextDisabled("Select a ROM file from the list to view info and launch.");
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(100, 0))) {
            *p_open = false;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Tip: Double-click any file to launch immediately. SCHIP ROMs are auto-detected!");
    }
    ImGui::End();

    return rom_chosen;
}

#endif
