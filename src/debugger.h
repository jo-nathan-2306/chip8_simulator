#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "imgui.h"
#include "chip8.h"
#include "disassembler.h"

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>

struct Breakpoint {
    uint16_t address = 0;
    bool enabled = true;
    uint32_t hit_count = 0;
};

struct TraceEntry {
    uint32_t step_num = 0;
    uint16_t pc = 0;
    uint16_t opcode = 0;
    std::string disasm = "";
    uint16_t index_reg = 0;
    uint8_t sp = 0;
    uint8_t dt = 0;
    uint8_t st = 0;
    uint8_t v[16] = {0};
};

struct DebuggerState {
    bool show_debugger = false;          
    bool show_memory_editor = false;     
    bool show_trace_log = false;         

    
    bool step_instruction = false;       
    bool step_over = false;              
    uint16_t step_over_target_pc = 0;    
    uint8_t step_over_sp = 0;            

    
    std::vector<Breakpoint> breakpoints;
    bool bp_table[65536] = {false};
    bool break_on_schip_exit = true;     
    bool break_on_collision = false;     
    bool break_on_unknown = false;
    bool breakpoint_hit = false;
    uint16_t last_hit_pc = 0;
    std::string break_reason = "";

    
    static const size_t MAX_TRACE = 128;
    TraceEntry trace_log[MAX_TRACE];
    size_t trace_count = 0;
    size_t trace_head = 0;
    uint32_t total_cycles = 0;

    
    bool auto_follow_pc = true;
    uint16_t jump_to_address = 0x0200;
    bool request_scroll = false;
    char addr_input_buf[16] = "0200";
    char bp_input_buf[16] = "0200";
    char mem_goto_buf[16] = "0200";
    int disasm_range_mode = 0; 

    
    uint8_t prev_v[16] = {0};
    uint16_t prev_index = 0;
    uint16_t prev_pc = 0;
    bool has_prev_state = false;

    bool has_breakpoint(uint16_t addr) const {
        return bp_table[addr];
    }

    void toggle_breakpoint(uint16_t addr) {
        if (bp_table[addr]) {
            remove_breakpoint(addr);
        } else {
            add_breakpoint(addr);
        }
    }

    void add_breakpoint(uint16_t addr) {
        if (!bp_table[addr]) {
            bp_table[addr] = true;
            Breakpoint bp;
            bp.address = addr;
            bp.enabled = true;
            bp.hit_count = 0;
            breakpoints.push_back(bp);
        }
    }

    void remove_breakpoint(uint16_t addr) {
        bp_table[addr] = false;
        breakpoints.erase(
            std::remove_if(breakpoints.begin(), breakpoints.end(),
                [addr](const Breakpoint& b) { return b.address == addr; }),
            breakpoints.end()
        );
    }

    void clear_breakpoints() {
        std::fill(std::begin(bp_table), std::end(bp_table), false);
        breakpoints.clear();
    }

    void record_trace(const Chip8& chip8) {
        total_cycles++;
        TraceEntry& e = trace_log[trace_head];
        e.step_num = total_cycles;
        e.pc = chip8.get_pc();
        e.opcode = chip8.get_opcode();
        DisasmResult dis = disassemble_instruction(e.pc, e.opcode, chip8.get_quirk_jump_vx());
        e.disasm = dis.full_text;
        e.index_reg = chip8.get_index();
        e.sp = chip8.get_sp();
        e.dt = chip8.get_delay_timer();
        e.st = chip8.get_sound_timer();
        for (int i = 0; i < 16; i++) {
            e.v[i] = chip8.get_v(i);
        }
        trace_head = (trace_head + 1) % MAX_TRACE;
        if (trace_count < MAX_TRACE) trace_count++;

        
        for (int i = 0; i < 16; i++) {
            prev_v[i] = chip8.get_v(i);
        }
        prev_index = chip8.get_index();
        prev_pc = chip8.get_pc();
        has_prev_state = true;
    }
};

inline void render_debugger_ui(
    Chip8& chip8,
    DebuggerState& dbg,
    bool& paused,
    bool& step_one_frame,
    int& cycles_per_frame
) {
    if (!dbg.show_debugger) return;

    float disp_w = ImGui::GetIO().DisplaySize.x;
    float disp_h = ImGui::GetIO().DisplaySize.y;
    float dw = std::max(600.0f, disp_w - 40.0f);
    float dh = std::max(400.0f, disp_h - 45.0f);
    float dx = (disp_w - dw) * 0.5f;
    float dy = 35.0f;

    ImGui::SetNextWindowSize(ImVec2(dw, dh), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(dx, dy), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("chip-8 / super-chip debugger & disassembler##window", &dbg.show_debugger, flags)) {
        
        
        
        if (paused) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f));
            if (ImGui::Button("  continue (space / f5)  ")) {
                paused = false;
            }
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.50f, 0.10f, 1.0f));
            if (ImGui::Button("  pause (space / f5)  ")) {
                paused = true;
            }
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.75f, 1.0f));
        if (ImGui::Button("step cycle (f10)")) {
            dbg.step_instruction = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.30f, 0.70f, 1.0f));
        if (ImGui::Button("step over (shift+f10)")) {
            dbg.step_over = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.55f, 1.0f));
        if (ImGui::Button("step frame (f11)")) {
            step_one_frame = true;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("reset vm")) {
            chip8.reset();
            if (!chip8.get_rom_filename().empty()) {
                chip8.load_rom(chip8.get_rom_filename());
            }
        }

        ImGui::SameLine();
        ImGui::Checkbox("follow pc", &dbg.auto_follow_pc);

        ImGui::SameLine();
        if (ImGui::Button("go to pc")) {
            dbg.jump_to_address = chip8.get_pc();
            dbg.request_scroll = true;
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        ImGui::SliderInt("##dbgspeed", &cycles_per_frame, 1, 100, "%d cyc/f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("cpu cycles executed per 60hz frame");

        
        ImGui::SameLine();
        if (chip8.is_halted()) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[halted: 00fd]");
            ImGui::SameLine();
            if (ImGui::SmallButton("resume")) {
                chip8.set_halted(false);
            }
        } else if (paused) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[paused]");
        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "[running]");
        }

        ImGui::SameLine();
        if (chip8.is_extended_mode()) {
            ImGui::TextColored(ImVec4(0.0f, 0.95f, 0.95f, 1.0f), "[schip 128x64]");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "[chip-8 64x32]");
        }

        ImGui::Separator();

        
        
        
        if (ImGui::BeginTabBar("debuggertabs")) {
            
            if (ImGui::BeginTabItem("disassembly")) {
                
                float content_w = ImGui::GetContentRegionAvail().x;
                float disasm_w = content_w * 0.64f;
                float regs_w   = content_w - disasm_w - 12.0f;

                ImGui::BeginChild("disasmchild", ImVec2(disasm_w, 0), true);

                
                ImGui::TextDisabled("view range:");
                ImGui::SameLine();
                ImGui::RadioButton("rom space##rng", &dbg.disasm_range_mode, 0);
                ImGui::SameLine();
                ImGui::RadioButton("around pc##rng", &dbg.disasm_range_mode, 1);
                ImGui::SameLine();
                ImGui::RadioButton("full 64kb##rng", &dbg.disasm_range_mode, 2);

                ImGui::SameLine();
                ImGui::SetNextItemWidth(70);
                ImGui::InputText("##gotobig", dbg.addr_input_buf, sizeof(dbg.addr_input_buf), ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::SameLine();
                if (ImGui::SmallButton("jump")) {
                    try {
                        dbg.jump_to_address = static_cast<uint16_t>(std::stoul(dbg.addr_input_buf, nullptr, 16) & 0xFFFF);
                        dbg.request_scroll = true;
                    } catch (...) {}
                }

                ImGui::Separator();

                
                uint16_t start_addr = 0x0200;
                uint16_t end_addr = 0x0200 + static_cast<uint16_t>(chip8.get_rom_size());
                if (end_addr <= start_addr) end_addr = 0x0FFF;

                if (dbg.disasm_range_mode == 1) { 
                    uint16_t cur_pc = chip8.get_pc();
                    start_addr = (cur_pc >= 128) ? (cur_pc - 128) : 0x0000;
                    end_addr = (cur_pc <= 65536 - 128) ? (cur_pc + 128) : 0xFFFF;
                } else if (dbg.disasm_range_mode == 2) { 
                    start_addr = 0x0000;
                    end_addr = 0xFFFF;
                }

                
                start_addr &= ~1; 
                end_addr &= ~1;
                if (end_addr < start_addr) end_addr = start_addr + 2;
                int total_instructions = (end_addr - start_addr) / 2 + 1;

                static const ImGuiTableFlags table_flags =
                    ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

                if (ImGui::BeginTable("disasmtable", 6, table_flags)) {
                    ImGui::TableSetupColumn("bp", ImGuiTableColumnFlags_WidthFixed, 28.0f);
                    ImGui::TableSetupColumn("address", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                    ImGui::TableSetupColumn("bytes", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                    ImGui::TableSetupColumn("instruction", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("description", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableHeadersRow();

                    const uint8_t* mem = chip8.get_memory();
                    uint16_t current_pc = chip8.get_pc();

                    ImGuiListClipper clipper;
                    clipper.Begin(total_instructions);

                    while (clipper.Step()) {
                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                            uint16_t addr = start_addr + (i * 2);
                            uint16_t op = (static_cast<uint16_t>(mem[addr]) << 8) | mem[addr + 1];
                            DisasmResult dis = disassemble_instruction(addr, op, chip8.get_quirk_jump_vx());

                            bool is_pc = (addr == current_pc);
                            bool has_bp = dbg.has_breakpoint(addr);

                            ImGui::TableNextRow();

                            
                            if (is_pc) {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(25, 75, 95, 180));
                            } else if (has_bp) {
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(85, 25, 25, 140));
                            }

                            
                            ImGui::TableSetColumnIndex(0);
                            char bp_btn_id[32];
                            std::snprintf(bp_btn_id, sizeof(bp_btn_id), "%s##bp%04x", has_bp ? "(o)" : " . ", addr);
                            if (has_bp) {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                            }
                            if (ImGui::SmallButton(bp_btn_id)) {
                                dbg.toggle_breakpoint(addr);
                            }
                            if (has_bp) {
                                ImGui::PopStyleColor();
                            }

                            
                            ImGui::TableSetColumnIndex(1);
                            if (is_pc) {
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.85f, 1.0f), "=> %04x", addr);
                            } else {
                                ImGui::Text("   %04x", addr);
                            }

                            
                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("%02x %02x", dis.byte0, dis.byte1);

                            
                            ImGui::TableSetColumnIndex(3);
                            if (is_pc) {
                                ImGui::TextColored(ImVec4(1.0f, 0.95f, 0.40f, 1.0f), "%s", dis.full_text.c_str());
                            } else if (dis.is_schip) {
                                ImGui::TextColored(ImVec4(0.0f, 0.85f, 0.95f, 1.0f), "%s", dis.full_text.c_str());
                            } else {
                                ImGui::Text("%s", dis.full_text.c_str());
                            }

                            
                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextDisabled("%s", dis.description.c_str());

                            
                            ImGui::TableSetColumnIndex(5);
                            if (dis.is_schip) {
                                ImGui::TextColored(ImVec4(0.0f, 0.88f, 0.82f, 1.0f), "schip");
                            } else {
                                ImGui::TextDisabled("chip8");
                            }

                            
                            if (is_pc && (dbg.request_scroll || dbg.auto_follow_pc)) {
                                ImGui::SetScrollHereY(0.35f);
                                dbg.request_scroll = false;
                            }
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndChild();

                
                ImGui::SameLine();
                ImGui::BeginChild("regschild", ImVec2(regs_w, 0), true);

                ImGui::TextDisabled("cpu registers (v0 - vf)");
                ImGui::Separator();

                
                if (ImGui::BeginTable("regtable", 2, ImGuiTableFlags_BordersInnerV)) {
                    for (int i = 0; i < 16; i++) {
                        ImGui::TableNextColumn();
                        uint8_t val = chip8.get_v(i);
                        bool changed = (dbg.has_prev_state && val != dbg.prev_v[i]);

                        if (changed) {
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "*v%x: %02x (%3u)", i, val, val);
                        } else if (i == 0xF && val > 0) {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), " v%x: %02x (%3u)", i, val, val);
                        } else {
                            ImGui::Text(" v%x: %02x (%3u)", i, val, val);
                        }
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("pointers & timers");

                ImGui::Text("pc: 0x%04x   |   i: 0x%04x", chip8.get_pc(), chip8.get_index());
                ImGui::Text("sp: %d/16       |   op: 0x%04x", chip8.get_sp(), chip8.get_opcode());

                
                char dt_str[32], st_str[32];
                std::snprintf(dt_str, sizeof(dt_str), "delay: %u", chip8.get_delay_timer());
                std::snprintf(st_str, sizeof(st_str), "sound: %u", chip8.get_sound_timer());
                ImGui::ProgressBar(chip8.get_delay_timer() / 255.0f, ImVec2(-1, 14), dt_str);
                ImGui::ProgressBar(chip8.get_sound_timer() / 255.0f, ImVec2(-1, 14), st_str);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("hp-48 rpl flags (r0 - rf)");

                if (ImGui::BeginTable("rpltable", 4)) {
                    for (int i = 0; i < 16; i++) {
                        ImGui::TableNextColumn();
                        ImGui::Text("r%x:%02x", i, chip8.get_rpl_flag(i));
                    }
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("call stack");

                uint8_t sp = chip8.get_sp();
                if (sp == 0) {
                    ImGui::TextDisabled("stack is empty (depth 0)");
                } else {
                    for (int i = sp - 1; i >= 0; i--) {
                        ImGui::Text("[%d] ret: 0x%04x", i, chip8.get_stack(i));
                    }
                }

                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            
            if (ImGui::BeginTabItem("breakpoints")) {
                ImGui::TextDisabled("active breakpoints");
                ImGui::Spacing();

                
                ImGui::Text("add breakpoint at address:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(90);
                ImGui::InputText("0x##bpadd", dbg.bp_input_buf, sizeof(dbg.bp_input_buf), ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::SameLine();
                if (ImGui::Button("add bp")) {
                    try {
                        uint16_t a = static_cast<uint16_t>(std::stoul(dbg.bp_input_buf, nullptr, 16) & 0xFFFF);
                        dbg.add_breakpoint(a);
                    } catch (...) {}
                }
                ImGui::SameLine();
                if (ImGui::Button("clear all breakpoints")) {
                    dbg.clear_breakpoints();
                }

                ImGui::Spacing();
                ImGui::Separator();

                
                ImGui::TextDisabled("automatic trap / break conditions:");
                ImGui::Checkbox("break on super-chip 00fd exit / halt", &dbg.break_on_schip_exit);
                ImGui::Checkbox("break on sprite collision (vf becomes 1)", &dbg.break_on_collision);
                ImGui::Checkbox("break on unknown / illegal opcode", &dbg.break_on_unknown);

                ImGui::Spacing();
                ImGui::Separator();

                if (dbg.breakpoints.empty()) {
                    ImGui::TextDisabled("no active breakpoints set. click '( . )' in disassembly or enter address above.");
                } else {
                    if (ImGui::BeginTable("bplisttable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("address", ImGuiTableColumnFlags_WidthFixed, 80);
                        ImGui::TableSetupColumn("hits", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("instruction", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableSetupColumn("actions", ImGuiTableColumnFlags_WidthFixed, 80);
                        ImGui::TableHeadersRow();

                        const uint8_t* mem = chip8.get_memory();
                        for (auto& bp : dbg.breakpoints) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "0x%04x", bp.address);

                            ImGui::TableNextColumn();
                            ImGui::Text("%u", bp.hit_count);

                            ImGui::TableNextColumn();
                            uint16_t op = (static_cast<uint16_t>(mem[bp.address]) << 8) | mem[bp.address + 1];
                            DisasmResult dis = disassemble_instruction(bp.address, op);
                            ImGui::Text("%s", dis.full_text.c_str());

                            ImGui::TableNextColumn();
                            char del_id[32];
                            std::snprintf(del_id, sizeof(del_id), "delete##%04x", bp.address);
                            if (ImGui::SmallButton(del_id)) {
                                dbg.remove_breakpoint(bp.address);
                                break;
                            }
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            
            if (ImGui::BeginTabItem("memory hex viewer")) {
                ImGui::TextDisabled("64kb ram browser & editor");
                ImGui::SameLine();

                if (ImGui::SmallButton("jump to pc")) {
                    dbg.jump_to_address = chip8.get_pc();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("jump to i")) {
                    dbg.jump_to_address = chip8.get_index();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("rom start (0x200)")) {
                    dbg.jump_to_address = 0x0200;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("fonts (0x000)")) {
                    dbg.jump_to_address = 0x0000;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("schip fonts (0x050)")) {
                    dbg.jump_to_address = 0x0050;
                }

                ImGui::Separator();

                
                const uint8_t* mem = chip8.get_memory();
                uint16_t cur_pc = chip8.get_pc();
                uint16_t cur_i  = chip8.get_index();

                ImGuiListClipper mem_clipper;
                mem_clipper.Begin(4096); 

                if (ImGui::BeginTable("memtable", 18, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV)) {
                    ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthFixed, 60);
                    for (int c = 0; c < 16; c++) {
                        char col_name[8];
                        std::snprintf(col_name, sizeof(col_name), "%x", c);
                        ImGui::TableSetupColumn(col_name, ImGuiTableColumnFlags_WidthFixed, 24);
                    }
                    ImGui::TableSetupColumn("ascii", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    while (mem_clipper.Step()) {
                        for (int r = mem_clipper.DisplayStart; r < mem_clipper.DisplayEnd; r++) {
                            uint16_t row_addr = static_cast<uint16_t>(r * 16);
                            ImGui::TableNextRow();

                            
                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("%04x:", row_addr);

                            char ascii_buf[17];
                            for (int col = 0; col < 16; col++) {
                                uint16_t byte_addr = row_addr + col;
                                uint8_t byte_val = mem[byte_addr];
                                ascii_buf[col] = (byte_val >= 32 && byte_val <= 126) ? static_cast<char>(byte_val) : '.';

                                ImGui::TableSetColumnIndex(col + 1);
                                if (byte_addr == cur_pc || byte_addr == cur_pc + 1) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "%02x", byte_val);
                                } else if (byte_addr == cur_i) {
                                    ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "%02x", byte_val);
                                } else if (byte_val == 0) {
                                    ImGui::TextDisabled("00");
                                } else {
                                    ImGui::Text("%02x", byte_val);
                                }
                            }
                            ascii_buf[16] = '\0';

                            
                            ImGui::TableSetColumnIndex(17);
                            ImGui::TextUnformatted(ascii_buf);
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            
            if (ImGui::BeginTabItem("trace log")) {
                ImGui::TextDisabled("last 128 executed instructions (circular buffer)");
                ImGui::SameLine();
                if (ImGui::SmallButton("clear trace")) {
                    dbg.trace_count = 0;
                    dbg.trace_head = 0;
                }

                ImGui::Separator();

                if (dbg.trace_count == 0) {
                    ImGui::TextDisabled("trace buffer empty. run instructions to capture history.");
                } else {
                    if (ImGui::BeginTable("tracetable", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("step", ImGuiTableColumnFlags_WidthFixed, 60);
                        ImGui::TableSetupColumn("pc", ImGuiTableColumnFlags_WidthFixed, 55);
                        ImGui::TableSetupColumn("opcode", ImGuiTableColumnFlags_WidthFixed, 55);
                        ImGui::TableSetupColumn("instruction", ImGuiTableColumnFlags_WidthFixed, 140);
                        ImGui::TableSetupColumn("i", ImGuiTableColumnFlags_WidthFixed, 50);
                        ImGui::TableSetupColumn("key registers", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        size_t start = (dbg.trace_head + DebuggerState::MAX_TRACE - dbg.trace_count) % DebuggerState::MAX_TRACE;
                        for (size_t i = 0; i < dbg.trace_count; i++) {
                            size_t idx = (start + i) % DebuggerState::MAX_TRACE;
                            const TraceEntry& e = dbg.trace_log[idx];

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%u", e.step_num);

                            ImGui::TableNextColumn();
                            ImGui::Text("0x%04x", e.pc);

                            ImGui::TableNextColumn();
                            ImGui::Text("%04x", e.opcode);

                            ImGui::TableNextColumn();
                            ImGui::Text("%s", e.disasm.c_str());

                            ImGui::TableNextColumn();
                            ImGui::Text("%03x", e.index_reg);

                            ImGui::TableNextColumn();
                            ImGui::Text("v0:%02x v1:%02x v2:%02x vf:%x", e.v[0], e.v[1], e.v[2], e.v[15]);
                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

            
            if (ImGui::BeginTabItem("schip & quirks")) {
                ImGui::TextColored(ImVec4(0.0f, 0.88f, 0.82f, 1.0f), "super-chip (schip) hardware state");
                ImGui::Separator();

                bool ext_mode = chip8.is_extended_mode();
                ImGui::Text("resolution mode: %s", ext_mode ? "128x64 extended (high-res)" : "64x32 standard (low-res)");
                ImGui::SameLine();
                if (ImGui::Button(ext_mode ? "switch to 64x32 (00fe)" : "switch to 128x64 (00ff)")) {
                    chip8.set_extended_mode(!ext_mode);
                    chip8.clear_screen();
                }

                bool hlt = chip8.is_halted();
                ImGui::Text("execution status: %s", hlt ? "halted (00fd quit / exit)" : "normal / active");
                if (hlt) {
                    ImGui::SameLine();
                    if (ImGui::Button("resume / un-halt")) {
                        chip8.set_halted(false);
                    }
                }

                ImGui::Spacing();
                ImGui::TextDisabled("hp-48 rpl user flags (r0 - rf):");
                for (int i = 0; i < 16; i++) {
                    if (i % 4 != 0) ImGui::SameLine();
                    int val = chip8.get_rpl_flag(i);
                    char lbl[16];
                    std::snprintf(lbl, sizeof(lbl), "r%x##flg", i);
                    ImGui::SetNextItemWidth(45);
                    if (ImGui::InputInt(lbl, &val, 0, 0, ImGuiInputTextFlags_CharsHexadecimal)) {
                        chip8.set_rpl_flag(i, static_cast<uint8_t>(val & 0xFF));
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.0f, 0.88f, 0.82f, 1.0f), "compatibility quirks configuration");
                ImGui::Separator();

                bool q_shift = chip8.get_quirk_shift_vx();
                if (ImGui::Checkbox("bit shift quirk: shift vx in place (schip default)", &q_shift)) {
                    chip8.set_quirk_shift_vx(q_shift);
                }
                ImGui::TextDisabled("  enabled: 8xy6/8xye shifts vx directly (schip standard).\n  disabled: 8xy6/8xye copies vy into vx before shifting (cosmac vip standard).");

                bool q_vf = chip8.get_quirk_vf_reset();
                if (ImGui::Checkbox("vf reset quirk: reset vf=0 after 8xy0/1/2/3 (vip default)", &q_vf)) {
                    chip8.set_quirk_vf_reset(q_vf);
                }
                ImGui::TextDisabled("  enabled: 8xy0 (ld), 8xy1 (or), 8xy2 (and), 8xy3 (xor) reset vf=0 after the op (cosmac vip).\n  disabled: vf is only written for arithmetic ops with carry/borrow (schip standard).");

                bool q_ls = chip8.get_quirk_load_store_no_i();
                if (ImGui::Checkbox("load/store quirk: leave i unchanged (schip default)", &q_ls)) {
                    chip8.set_quirk_load_store_no_i(q_ls);
                }
                ImGui::TextDisabled("  enabled: fx55/fx65 leaves index register i unchanged (schip standard).\n  disabled: fx55/fx65 increments i = i + x + 1 (cosmac vip standard).");

                bool q_jmp = chip8.get_quirk_jump_vx();
                if (ImGui::Checkbox("jump quirk: bnnn jumps to xnn + vx (schip bxnn)", &q_jmp)) {
                    chip8.set_quirk_jump_vx(q_jmp);
                }
                ImGui::TextDisabled("  enabled: bxxx jumps to address xxx + vx (schip bxnn).\n  disabled: bxxx jumps to address xxx + v0 (chip-8 standard).");

                bool q_scroll = chip8.get_quirk_legacy_scroll();
                if (ImGui::Checkbox("legacy scrolling quirk: half-line scrolling in lores mode", &q_scroll)) {
                    chip8.set_quirk_legacy_scroll(q_scroll);
                }
                ImGui::TextDisabled("  enabled: 00cn / 00fb in 64x32 mode scrolls half amount (hp-48 calculator hardware).\n  disabled: modern full-pixel scrolling in all modes.");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

#endif 
