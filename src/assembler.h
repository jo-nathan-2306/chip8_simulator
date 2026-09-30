#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

struct AssemblerError {
    int line_number;
    std::string message;
};

struct AssemblyResult {
    bool success = false;
    std::vector<uint8_t> bytecode;
    std::vector<AssemblerError> errors;
    std::string summary;
};

class Chip8Assembler {
public:
    static AssemblyResult assemble(const std::string& source_code) {
        AssemblyResult result;
        std::istringstream stream(source_code);
        std::string raw_line;
        int line_num = 0;

        struct ParsedLine {
            int line_num;
            std::string label;
            std::string mnemonic;
            std::vector<std::string> args;
            bool is_data = false;
            std::vector<uint8_t> raw_bytes;
        };

        std::vector<ParsedLine> parsed_lines;
        std::unordered_map<std::string, uint16_t> label_table;
        uint16_t current_address = 0x0200;

        
        while (std::getline(stream, raw_line)) {
            line_num++;
            std::string line = strip_comments(raw_line);
            line = trim(line);
            if (line.empty()) continue;

            ParsedLine pl;
            pl.line_num = line_num;

            
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos && !is_inside_quotes(line, colon_pos)) {
                std::string lbl = trim(line.substr(0, colon_pos));
                if (!lbl.empty()) {
                    std::string upper_lbl = to_upper(lbl);
                    if (label_table.find(upper_lbl) != label_table.end()) {
                        result.errors.push_back({line_num, "duplicate label definition: '" + lbl + "'"});
                    } else {
                        label_table[upper_lbl] = current_address;
                    }
                    pl.label = upper_lbl;
                }
                line = trim(line.substr(colon_pos + 1));
                if (line.empty()) {
                    parsed_lines.push_back(pl);
                    continue;
                }
            }

            
            std::string mnem;
            std::string rest;
            size_t space_pos = line.find_first_of(" \t");
            if (space_pos != std::string::npos) {
                mnem = to_upper(trim(line.substr(0, space_pos)));
                rest = trim(line.substr(space_pos + 1));
            } else {
                mnem = to_upper(trim(line));
            }

            pl.mnemonic = mnem;

            
            if (!rest.empty()) {
                std::stringstream ss(rest);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    item = trim(item);
                    if (!item.empty()) {
                        pl.args.push_back(item);
                    }
                }
            }

            
            if (mnem == "db" || mnem == "byte") {
                pl.is_data = true;
                for (const auto& a : pl.args) {
                    uint32_t val = 0;
                    if (parse_number(a, val)) {
                        pl.raw_bytes.push_back(static_cast<uint8_t>(val & 0xFF));
                    } else {
                        result.errors.push_back({line_num, "invalid byte data value: '" + a + "'"});
                    }
                }
                current_address += static_cast<uint16_t>(pl.raw_bytes.size());
            } else if (mnem == "dw" || mnem == "word") {
                pl.is_data = true;
                for (const auto& a : pl.args) {
                    uint32_t val = 0;
                    if (parse_number(a, val)) {
                        pl.raw_bytes.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
                        pl.raw_bytes.push_back(static_cast<uint8_t>(val & 0xFF));
                    } else {
                        result.errors.push_back({line_num, "invalid word data value: '" + a + "'"});
                    }
                }
                current_address += static_cast<uint16_t>(pl.raw_bytes.size());
            } else {
                
                current_address += 2;
            }

            parsed_lines.push_back(pl);
        }

        if (!result.errors.empty()) {
            result.success = false;
            result.summary = "pass 1 failed with " + std::to_string(result.errors.size()) + " errors.";
            return result;
        }

        
        current_address = 0x0200;
        for (const auto& pl : parsed_lines) {
            if (pl.mnemonic.empty()) continue;

            if (pl.is_data) {
                for (uint8_t b : pl.raw_bytes) {
                    result.bytecode.push_back(b);
                }
                current_address += static_cast<uint16_t>(pl.raw_bytes.size());
                continue;
            }

            uint16_t op = 0;
            bool ok = assemble_single_instruction(pl.line_num, pl.mnemonic, pl.args, label_table, current_address, op, result.errors);
            if (ok) {
                result.bytecode.push_back(static_cast<uint8_t>((op >> 8) & 0xFF));
                result.bytecode.push_back(static_cast<uint8_t>(op & 0xFF));
            }
            current_address += 2;
        }

        if (result.errors.empty()) {
            result.success = true;
            result.summary = "assembly succeeded! generated " + std::to_string(result.bytecode.size()) + " bytes.";
        } else {
            result.success = false;
            result.summary = "assembly failed with " + std::to_string(result.errors.size()) + " errors.";
        }

        return result;
    }

    static std::string get_template_code(int template_idx) {
        switch (template_idx) {
            case 0: 
                return
                    "; ==========================================\n"
                    "; super-chip (schip) 16x16 sprite demo\n"
                    "; ==========================================\n"
                    "start:\n"
                    "    high            ; enable schip 128x64 high-res mode\n"
                    "    cls             ; clear screen\n"
                    "    ld v0, 56       ; x position (centered in 128px)\n"
                    "    ld v1, 24       ; y position (centered in 64px)\n"
                    "    ld i, sprite_box\n"
                    "    drw v0, v1, 0   ; draw 16x16 sprite (n=0 in schip)\n"
                    "\n"
                    "main_loop:\n"
                    "    ld v2, 10       ; delay counter\n"
                    "    ld dt, v2\n"
                    "wait_timer:\n"
                    "    ld v3, dt\n"
                    "    se v3, 0\n"
                    "    jp wait_timer\n"
                    "\n"
                    "    scr             ; scroll display right 4 pixels\n"
                    "    scd 1           ; scroll display down 1 line\n"
                    "    jp main_loop\n"
                    "\n"
                    "; 16x16 sprite data (32 bytes: 2 bytes per row * 16 rows)\n"
                    "sprite_box:\n"
                    "    db 0xff, 0xff,  0x80, 0x01,  0x80, 0x01,  0x80, 0x01\n"
                    "    db 0x8f, 0xf1,  0x88, 0x11,  0x88, 0x11,  0x88, 0x11\n"
                    "    db 0x88, 0x11,  0x88, 0x11,  0x88, 0x11,  0x8f, 0xf1\n"
                    "    db 0x80, 0x01,  0x80, 0x01,  0x80, 0x01,  0xff, 0xff\n";

            case 1: 
                return
                    "; ==========================================\n"
                    "; bouncing square animation\n"
                    "; ==========================================\n"
                    "init:\n"
                    "    cls\n"
                    "    ld v0, 10       ; x = 10\n"
                    "    ld v1, 10       ; y = 10\n"
                    "    ld v2, 1        ; dx = +1\n"
                    "    ld v3, 1        ; dy = +1\n"
                    "    ld i, square\n"
                    "\n"
                    "game_loop:\n"
                    "    drw v0, v1, 4   ; draw square\n"
                    "    ld v4, 2\n"
                    "    ld dt, v4\n"
                    "delay:\n"
                    "    ld v5, dt\n"
                    "    se v5, 0\n"
                    "    jp delay\n"
                    "\n"
                    "    drw v0, v1, 4   ; erase square\n"
                    "\n"
                    "    ; move x\n"
                    "    add v0, v2\n"
                    "    se v0, 60\n"
                    "    jp check_x_left\n"
                    "    ld v2, 0xff     ; dx = -1\n"
                    "check_x_left:\n"
                    "    se v0, 0\n"
                    "    jp update_y\n"
                    "    ld v2, 1        ; dx = +1\n"
                    "\n"
                    "update_y:\n"
                    "    add v1, v3\n"
                    "    se v1, 28\n"
                    "    jp check_y_top\n"
                    "    ld v3, 0xff     ; dy = -1\n"
                    "check_y_top:\n"
                    "    se v1, 0\n"
                    "    jp game_loop\n"
                    "    ld v3, 1        ; dy = +1\n"
                    "    jp game_loop\n"
                    "\n"
                    "square:\n"
                    "    db 0xf0, 0x90, 0x90, 0xf0\n";

            case 2: 
                return
                    "; ==========================================\n"
                    "; interactive keypad & font display\n"
                    "; ==========================================\n"
                    "start:\n"
                    "    cls\n"
                    "    ld v0, 28       ; x position\n"
                    "    ld v1, 14       ; y position\n"
                    "wait_key:\n"
                    "    ld v2, k        ; wait for keypress 0-f\n"
                    "    cls\n"
                    "    ld f, v2        ; point i to 5-byte font digit in v2\n"
                    "    drw v0, v1, 5   ; draw digit\n"
                    "    ld v4, 5\n"
                    "    ld st, v4       ; short beep\n"
                    "    jp wait_key\n";

            case 3: 
                return
                    "; ==========================================\n"
                    "; super-chip rpl flags test\n"
                    "; ==========================================\n"
                    "start:\n"
                    "    ld v0, 0x11\n"
                    "    ld v1, 0x22\n"
                    "    ld v2, 0x33\n"
                    "    ld v3, 0x44\n"
                    "    ld r, v3        ; store v0..v3 to rpl user flags\n"
                    "    ld v0, 0\n"
                    "    ld v1, 0\n"
                    "    ld v2, 0\n"
                    "    ld v3, 0\n"
                    "    ld v3, r        ; restore v0..v3 from rpl user flags\n"
                    "    exit            ; halt program (00fd)\n";

            default:
                return "";
        }
    }

private:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    static std::string strip_comments(const std::string& line) {
        size_t sc = line.find(';');
        size_t sl = line.find("//");
        size_t pos = std::min(sc, sl);
        if (pos != std::string::npos) {
            return line.substr(0, pos);
        }
        return line;
    }

    static bool is_inside_quotes(const std::string& line, size_t pos) {
        bool in_q = false;
        for (size_t i = 0; i < pos; i++) {
            if (line[i] == '"') in_q = !in_q;
        }
        return in_q;
    }

    static bool parse_number(const std::string& str, uint32_t& out_val) {
        std::string s = trim(str);
        if (s.empty()) return false;

        try {
            if (s.rfind("0x", 0) == 0 || s.rfind("0x", 0) == 0 || s.rfind("$", 0) == 0) {
                size_t prefix = (s[0] == '$') ? 1 : 2;
                out_val = std::stoul(s.substr(prefix), nullptr, 16);
                return true;
            } else if (s.rfind("0b", 0) == 0 || s.rfind("0b", 0) == 0 || s.rfind("%", 0) == 0) {
                size_t prefix = (s[0] == '%') ? 1 : 2;
                out_val = std::stoul(s.substr(prefix), nullptr, 2);
                return true;
            } else {
                out_val = std::stoul(s, nullptr, 10);
                return true;
            }
        } catch (...) {
            return false;
        }
    }

    static bool parse_register(const std::string& str, uint8_t& out_reg) {
        std::string s = to_upper(trim(str));
        if (s.length() >= 2 && s[0] == 'v') {
            char c = s[1];
            if (c >= '0' && c <= '9') {
                out_reg = static_cast<uint8_t>(c - '0');
                return true;
            } else if (c >= 'a' && c <= 'f') {
                out_reg = static_cast<uint8_t>(10 + (c - 'a'));
                return true;
            }
        }
        return false;
    }

    static bool resolve_address(
        int line_num,
        const std::string& arg,
        const std::unordered_map<std::string, uint16_t>& labels,
        uint16_t& out_addr,
        std::vector<AssemblerError>& errors
    ) {
        uint32_t val = 0;
        if (parse_number(arg, val)) {
            out_addr = static_cast<uint16_t>(val & 0xFFF);
            return true;
        }

        std::string upper_arg = to_upper(trim(arg));
        auto it = labels.find(upper_arg);
        if (it != labels.end()) {
            out_addr = it->second;
            return true;
        }

        errors.push_back({line_num, "unknown label or invalid address: '" + arg + "'"});
        return false;
    }

    static bool assemble_single_instruction(
        int line_num,
        const std::string& mnem,
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, uint16_t>& labels,
        uint16_t current_pc,
        uint16_t& out_op,
        std::vector<AssemblerError>& errors
    ) {
        (void)current_pc;
        
        if (mnem == "cls")  { out_op = 0x00E0; return true; }
        if (mnem == "ret")  { out_op = 0x00EE; return true; }
        if (mnem == "scr")  { out_op = 0x00FB; return true; } 
        if (mnem == "scl")  { out_op = 0x00FC; return true; } 
        if (mnem == "exit" || mnem == "quit" || mnem == "hlt") { out_op = 0x00FD; return true; } 
        if (mnem == "low")  { out_op = 0x00FE; return true; } 
        if (mnem == "high") { out_op = 0x00FF; return true; } 

        
        if (mnem == "scd") {
            if (args.size() != 1) {
                errors.push_back({line_num, "scd expects 1 argument (number of lines, 0-15)"});
                return false;
            }
            uint32_t lines = 0;
            if (!parse_number(args[0], lines) || lines > 15) {
                errors.push_back({line_num, "scd line count must be between 0 and 15"});
                return false;
            }
            out_op = 0x00C0 | (lines & 0x0F);
            return true;
        }

        
        if (mnem == "scu") {
            if (args.size() != 1) {
                errors.push_back({line_num, "scu expects 1 argument (number of lines, 0-15)"});
                return false;
            }
            uint32_t lines = 0;
            if (!parse_number(args[0], lines) || lines > 15) {
                errors.push_back({line_num, "scu line count must be between 0 and 15"});
                return false;
            }
            out_op = 0x00D0 | (lines & 0x0F);
            return true;
        }

        
        if (mnem == "jp") {
            if (args.size() == 1) {
                uint16_t addr = 0;
                if (!resolve_address(line_num, args[0], labels, addr, errors)) return false;
                out_op = 0x1000 | (addr & 0x0FFF);
                return true;
            } else if (args.size() == 2) {
                std::string reg = to_upper(trim(args[0]));
                if (reg == "v0") {
                    uint16_t addr = 0;
                    if (!resolve_address(line_num, args[1], labels, addr, errors)) return false;
                    out_op = 0xB000 | (addr & 0x0FFF);
                    return true;
                } else {
                    uint8_t vx = 0;
                    if (parse_register(reg, vx)) {
                        
                        uint16_t addr = 0;
                        if (!resolve_address(line_num, args[1], labels, addr, errors)) return false;
                        out_op = 0xB000 | (static_cast<uint16_t>(vx) << 8) | (addr & 0x00FF);
                        return true;
                    }
                    errors.push_back({line_num, "first argument of jp v0, addr must be v0 or a register"});
                    return false;
                }
            }
        }

        if (mnem == "call") {
            if (args.size() != 1) {
                errors.push_back({line_num, "call expects 1 argument (subroutine address or label)"});
                return false;
            }
            uint16_t addr = 0;
            if (!resolve_address(line_num, args[0], labels, addr, errors)) return false;
            out_op = 0x2000 | (addr & 0x0FFF);
            return true;
        }

        
        if (mnem == "se") {
            if (args.size() != 2) {
                errors.push_back({line_num, "se expects 2 arguments: se vx, byte or se vx, vy"});
                return false;
            }
            uint8_t vx = 0;
            if (!parse_register(args[0], vx)) {
                errors.push_back({line_num, "first argument of se must be register v0-vf"});
                return false;
            }
            uint8_t vy = 0;
            if (parse_register(args[1], vy)) {
                out_op = 0x5000 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
                return true;
            } else {
                uint32_t val = 0;
                if (!parse_number(args[1], val)) {
                    errors.push_back({line_num, "second argument of se must be register vy or byte literal"});
                    return false;
                }
                out_op = 0x3000 | (static_cast<uint16_t>(vx) << 8) | (val & 0xFF);
                return true;
            }
        }

        if (mnem == "sne") {
            if (args.size() != 2) {
                errors.push_back({line_num, "sne expects 2 arguments: sne vx, byte or sne vx, vy"});
                return false;
            }
            uint8_t vx = 0;
            if (!parse_register(args[0], vx)) {
                errors.push_back({line_num, "first argument of sne must be register v0-vf"});
                return false;
            }
            uint8_t vy = 0;
            if (parse_register(args[1], vy)) {
                out_op = 0x9000 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
                return true;
            } else {
                uint32_t val = 0;
                if (!parse_number(args[1], val)) {
                    errors.push_back({line_num, "second argument of sne must be register vy or byte literal"});
                    return false;
                }
                out_op = 0x4000 | (static_cast<uint16_t>(vx) << 8) | (val & 0xFF);
                return true;
            }
        }

        
        if (mnem == "drw") {
            if (args.size() != 3) {
                errors.push_back({line_num, "drw expects 3 arguments: drw vx, vy, nibble (or 0 for schip 16x16)"});
                return false;
            }
            uint8_t vx = 0, vy = 0;
            uint32_t height = 0;
            if (!parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "first two arguments of drw must be registers"});
                return false;
            }
            if (!parse_number(args[2], height) || height > 15) {
                errors.push_back({line_num, "drw sprite height must be 0-15"});
                return false;
            }
            out_op = 0xD000 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4) | (height & 0x0F);
            return true;
        }

        
        if (mnem == "skp") {
            uint8_t vx = 0;
            if (args.size() != 1 || !parse_register(args[0], vx)) {
                errors.push_back({line_num, "skp expects 1 register argument: skp vx"});
                return false;
            }
            out_op = 0xE09E | (static_cast<uint16_t>(vx) << 8);
            return true;
        }
        if (mnem == "sknp") {
            uint8_t vx = 0;
            if (args.size() != 1 || !parse_register(args[0], vx)) {
                errors.push_back({line_num, "sknp expects 1 register argument: sknp vx"});
                return false;
            }
            out_op = 0xE0A1 | (static_cast<uint16_t>(vx) << 8);
            return true;
        }

        
        if (mnem == "add") {
            if (args.size() != 2) {
                errors.push_back({line_num, "add expects 2 arguments"});
                return false;
            }
            std::string arg0 = to_upper(trim(args[0]));
            if (arg0 == "i") {
                uint8_t vx = 0;
                if (!parse_register(args[1], vx)) {
                    errors.push_back({line_num, "add i, vx expects register vx"});
                    return false;
                }
                out_op = 0xF01E | (static_cast<uint16_t>(vx) << 8);
                return true;
            }
            uint8_t vx = 0;
            if (!parse_register(args[0], vx)) {
                errors.push_back({line_num, "first argument of add must be register or i"});
                return false;
            }
            uint8_t vy = 0;
            if (parse_register(args[1], vy)) {
                out_op = 0x8004 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
                return true;
            } else {
                uint32_t val = 0;
                if (!parse_number(args[1], val)) {
                    errors.push_back({line_num, "second argument of add must be register or byte"});
                    return false;
                }
                out_op = 0x7000 | (static_cast<uint16_t>(vx) << 8) | (val & 0xFF);
                return true;
            }
        }

        
        if (mnem == "sub") {
            uint8_t vx = 0, vy = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "sub expects 2 registers: sub vx, vy"});
                return false;
            }
            out_op = 0x8005 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "subn") {
            uint8_t vx = 0, vy = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "subn expects 2 registers: subn vx, vy"});
                return false;
            }
            out_op = 0x8007 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "or") {
            uint8_t vx = 0, vy = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "or expects 2 registers: or vx, vy"});
                return false;
            }
            out_op = 0x8001 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "and") {
            uint8_t vx = 0, vy = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "and expects 2 registers: and vx, vy"});
                return false;
            }
            out_op = 0x8002 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "xor") {
            uint8_t vx = 0, vy = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_register(args[1], vy)) {
                errors.push_back({line_num, "xor expects 2 registers: xor vx, vy"});
                return false;
            }
            out_op = 0x8003 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "shr") {
            uint8_t vx = 0;
            if (args.empty() || !parse_register(args[0], vx)) {
                errors.push_back({line_num, "shr expects register: shr vx {, vy}"});
                return false;
            }
            uint8_t vy = vx;
            if (args.size() >= 2) parse_register(args[1], vy);
            out_op = 0x8006 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }
        if (mnem == "shl") {
            uint8_t vx = 0;
            if (args.empty() || !parse_register(args[0], vx)) {
                errors.push_back({line_num, "shl expects register: shl vx {, vy}"});
                return false;
            }
            uint8_t vy = vx;
            if (args.size() >= 2) parse_register(args[1], vy);
            out_op = 0x800E | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
            return true;
        }

        
        if (mnem == "rnd") {
            uint8_t vx = 0;
            uint32_t val = 0;
            if (args.size() != 2 || !parse_register(args[0], vx) || !parse_number(args[1], val)) {
                errors.push_back({line_num, "rnd expects 2 arguments: rnd vx, byte"});
                return false;
            }
            out_op = 0xC000 | (static_cast<uint16_t>(vx) << 8) | (val & 0xFF);
            return true;
        }

        
        if (mnem == "ld") {
            if (args.size() != 2) {
                errors.push_back({line_num, "ld expects 2 arguments"});
                return false;
            }
            std::string dest = to_upper(trim(args[0]));
            std::string src  = to_upper(trim(args[1]));

            
            if (dest == "i") {
                uint16_t addr = 0;
                if (!resolve_address(line_num, src, labels, addr, errors)) return false;
                out_op = 0xA000 | (addr & 0x0FFF);
                return true;
            }

            
            if (dest == "dt") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld dt, vx expects register vx"});
                    return false;
                }
                out_op = 0xF015 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "st") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld st, vx expects register vx"});
                    return false;
                }
                out_op = 0xF018 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "f") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld f, vx expects register vx"});
                    return false;
                }
                out_op = 0xF029 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "hf") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld hf, vx expects register vx"});
                    return false;
                }
                out_op = 0xF030 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "b") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld b, vx expects register vx"});
                    return false;
                }
                out_op = 0xF033 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "[i]") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld [i], vx expects register vx"});
                    return false;
                }
                out_op = 0xF055 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            if (dest == "r") {
                uint8_t vx = 0;
                if (!parse_register(src, vx)) {
                    errors.push_back({line_num, "ld r, vx expects register vx"});
                    return false;
                }
                out_op = 0xF075 | (static_cast<uint16_t>(vx) << 8);
                return true;
            }

            
            uint8_t vx = 0;
            if (parse_register(dest, vx)) {
                
                if (src == "dt") {
                    out_op = 0xF007 | (static_cast<uint16_t>(vx) << 8);
                    return true;
                }
                
                if (src == "k") {
                    out_op = 0xF00A | (static_cast<uint16_t>(vx) << 8);
                    return true;
                }
                
                if (src == "[i]") {
                    out_op = 0xF065 | (static_cast<uint16_t>(vx) << 8);
                    return true;
                }
                
                if (src == "r") {
                    out_op = 0xF085 | (static_cast<uint16_t>(vx) << 8);
                    return true;
                }
                
                uint8_t vy = 0;
                if (parse_register(src, vy)) {
                    out_op = 0x8000 | (static_cast<uint16_t>(vx) << 8) | (static_cast<uint16_t>(vy) << 4);
                    return true;
                }
                
                uint32_t val = 0;
                if (parse_number(src, val)) {
                    out_op = 0x6000 | (static_cast<uint16_t>(vx) << 8) | (val & 0xFF);
                    return true;
                }
            }
        }

        errors.push_back({line_num, "unrecognized instruction mnemonic: '" + mnem + "'"});
        return false;
    }
};

#endif 
