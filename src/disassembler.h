#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <cstdint>
#include <string>
#include <vector>
#include <cstdio>

struct DisasmResult {
    uint16_t address = 0;
    uint16_t opcode = 0;
    uint8_t byte0 = 0;
    uint8_t byte1 = 0;
    std::string mnemonic = "";
    std::string operands = "";
    std::string full_text = "";
    std::string description = "";
    bool is_schip = false;
    bool is_jump_or_call = false;
    uint16_t target_addr = 0;
};

inline DisasmResult disassemble_instruction(uint16_t address, uint16_t opcode, bool quirk_jump_vx = false) {
    DisasmResult res;
    res.address = address;
    res.opcode = opcode;
    res.byte0 = static_cast<uint8_t>((opcode >> 8) & 0xFF);
    res.byte1 = static_cast<uint8_t>(opcode & 0xFF);
    res.is_schip = false;
    res.is_jump_or_call = false;
    res.target_addr = 0;

    uint8_t nibble0 = (opcode >> 12) & 0x0F;
    uint8_t x       = (opcode >> 8)  & 0x0F;
    uint8_t y       = (opcode >> 4)  & 0x0F;
    uint8_t n       = opcode         & 0x0F;
    uint8_t kk      = opcode         & 0xFF;
    uint16_t nnn    = opcode         & 0x0FFF;

    char buf[64];

    switch (nibble0) {
        case 0x0:
            if ((opcode & 0xFFF0) == 0x00C0) {
                res.mnemonic = "SCD";
                std::snprintf(buf, sizeof(buf), "%u", n);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Scroll display down %u lines", n);
                res.description = buf;
                res.is_schip = true;
            } else if ((opcode & 0xFFF0) == 0x00D0) {
                res.mnemonic = "SCU";
                std::snprintf(buf, sizeof(buf), "%u", n);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Scroll display up %u lines", n);
                res.description = buf;
                res.is_schip = true;
            } else {
                switch (opcode & 0x00FF) {
                    case 0xE0:
                        res.mnemonic = "CLS";
                        res.operands = "";
                        res.description = "Clear screen display";
                        break;
                    case 0xEE:
                        res.mnemonic = "RET";
                        res.operands = "";
                        res.description = "Return from subroutine";
                        break;
                    case 0xFB:
                        res.mnemonic = "SCR";
                        res.operands = "";
                        res.description = "Scroll display right 4 pixels";
                        res.is_schip = true;
                        break;
                    case 0xFC:
                        res.mnemonic = "SCL";
                        res.operands = "";
                        res.description = "Scroll display left 4 pixels";
                        res.is_schip = true;
                        break;
                    case 0xFD:
                        res.mnemonic = "EXIT";
                        res.operands = "";
                        res.description = "Halt / exit interpreter";
                        res.is_schip = true;
                        break;
                    case 0xFE:
                        res.mnemonic = "LOW";
                        res.operands = "";
                        res.description = "Disable extended mode (set 64x32)";
                        res.is_schip = true;
                        break;
                    case 0xFF:
                        res.mnemonic = "HIGH";
                        res.operands = "";
                        res.description = "Enable extended mode (set 128x64)";
                        res.is_schip = true;
                        break;
                    default:
                        res.mnemonic = "SYS";
                        std::snprintf(buf, sizeof(buf), "0x%03X", nnn);
                        res.operands = buf;
                        res.description = "Call machine routine at " + res.operands;
                        break;
                }
            }
            break;

        case 0x1:
            res.mnemonic = "JP";
            std::snprintf(buf, sizeof(buf), "0x%03X", nnn);
            res.operands = buf;
            res.description = "Jump to address " + res.operands;
            res.is_jump_or_call = true;
            res.target_addr = nnn;
            break;

        case 0x2:
            res.mnemonic = "CALL";
            std::snprintf(buf, sizeof(buf), "0x%03X", nnn);
            res.operands = buf;
            res.description = "Call subroutine at " + res.operands;
            res.is_jump_or_call = true;
            res.target_addr = nnn;
            break;

        case 0x3:
            res.mnemonic = "SE";
            std::snprintf(buf, sizeof(buf), "V%X, 0x%02X", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "Skip next if V%X == 0x%02X (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x4:
            res.mnemonic = "SNE";
            std::snprintf(buf, sizeof(buf), "V%X, 0x%02X", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "Skip next if V%X != 0x%02X (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x5:
            if (n == 0) {
                res.mnemonic = "SE";
                std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Skip next if V%X == V%X", x, y);
                res.description = buf;
            } else {
                res.mnemonic = "DATA";
                std::snprintf(buf, sizeof(buf), "0x%04X", opcode);
                res.operands = buf;
                res.description = "Unknown / raw data";
            }
            break;

        case 0x6:
            res.mnemonic = "LD";
            std::snprintf(buf, sizeof(buf), "V%X, 0x%02X", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "Set V%X = 0x%02X (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x7:
            res.mnemonic = "ADD";
            std::snprintf(buf, sizeof(buf), "V%X, 0x%02X", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "Set V%X = V%X + 0x%02X (%u)", x, x, kk, kk);
            res.description = buf;
            break;

        case 0x8:
            switch (n) {
                case 0x0:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X", x, y);
                    res.description = buf;
                    break;
                case 0x1:
                    res.mnemonic = "OR";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X | V%X", x, x, y);
                    res.description = buf;
                    break;
                case 0x2:
                    res.mnemonic = "AND";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X & V%X", x, x, y);
                    res.description = buf;
                    break;
                case 0x3:
                    res.mnemonic = "XOR";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X ^ V%X", x, x, y);
                    res.description = buf;
                    break;
                case 0x4:
                    res.mnemonic = "ADD";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X + V%X, VF = carry", x, x, y);
                    res.description = buf;
                    break;
                case 0x5:
                    res.mnemonic = "SUB";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X - V%X, VF = !borrow", x, x, y);
                    res.description = buf;
                    break;
                case 0x6:
                    res.mnemonic = "SHR";
                    std::snprintf(buf, sizeof(buf), "V%X {, V%X}", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X >> 1, VF = lsb", x, x);
                    res.description = buf;
                    break;
                case 0x7:
                    res.mnemonic = "SUBN";
                    std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X - V%X, VF = !borrow", x, y, x);
                    res.description = buf;
                    break;
                case 0xE:
                    res.mnemonic = "SHL";
                    std::snprintf(buf, sizeof(buf), "V%X {, V%X}", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = V%X << 1, VF = msb", x, x);
                    res.description = buf;
                    break;
                default:
                    res.mnemonic = "DATA";
                    std::snprintf(buf, sizeof(buf), "0x%04X", opcode);
                    res.operands = buf;
                    res.description = "Unknown opcode";
                    break;
            }
            break;

        case 0x9:
            if (n == 0) {
                res.mnemonic = "SNE";
                std::snprintf(buf, sizeof(buf), "V%X, V%X", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Skip next if V%X != V%X", x, y);
                res.description = buf;
            } else {
                res.mnemonic = "DATA";
                std::snprintf(buf, sizeof(buf), "0x%04X", opcode);
                res.operands = buf;
                res.description = "Unknown opcode";
            }
            break;

        case 0xA:
            res.mnemonic = "LD";
            std::snprintf(buf, sizeof(buf), "I, 0x%03X", nnn);
            res.operands = buf;
            res.description = "Set index I = 0x" + std::string(buf).substr(std::string(buf).find("0x") + 2);
            break;

        case 0xB:
            res.mnemonic = "JP";
            if (quirk_jump_vx) {
                std::snprintf(buf, sizeof(buf), "V%X, 0x%03X", x, nnn);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Jump to 0x%03X + V%X (SCHIP)", nnn, x);
                res.description = buf;
                res.is_schip = true;
            } else {
                std::snprintf(buf, sizeof(buf), "V0, 0x%03X", nnn);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Jump to 0x%03X + V0", nnn);
                res.description = buf;
            }
            res.is_jump_or_call = true;
            break;

        case 0xC:
            res.mnemonic = "RND";
            std::snprintf(buf, sizeof(buf), "V%X, 0x%02X", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "Set V%X = random_byte & 0x%02X", x, kk);
            res.description = buf;
            break;

        case 0xD:
            res.mnemonic = "DRW";
            if (n == 0) {
                std::snprintf(buf, sizeof(buf), "V%X, V%X, 0", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Draw 16x16 sprite at (V%X, V%X)", x, y);
                res.description = buf;
                res.is_schip = true;
            } else {
                std::snprintf(buf, sizeof(buf), "V%X, V%X, %u", x, y, n);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Draw 8x%u sprite at (V%X, V%X)", n, x, y);
                res.description = buf;
            }
            break;

        case 0xE:
            if (kk == 0x9E) {
                res.mnemonic = "SKP";
                std::snprintf(buf, sizeof(buf), "V%X", x);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Skip next if key in V%X is pressed", x);
                res.description = buf;
            } else if (kk == 0xA1) {
                res.mnemonic = "SKNP";
                std::snprintf(buf, sizeof(buf), "V%X", x);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "Skip next if key in V%X not pressed", x);
                res.description = buf;
            } else {
                res.mnemonic = "DATA";
                std::snprintf(buf, sizeof(buf), "0x%04X", opcode);
                res.operands = buf;
                res.description = "Unknown opcode";
            }
            break;

        case 0xF:
            switch (kk) {
                case 0x07:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "V%X, DT", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set V%X = delay timer", x);
                    res.description = buf;
                    break;
                case 0x0A:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "V%X, K", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Wait for key press, store in V%X", x);
                    res.description = buf;
                    break;
                case 0x15:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "DT, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set delay timer = V%X", x);
                    res.description = buf;
                    break;
                case 0x18:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "ST, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set sound timer = V%X", x);
                    res.description = buf;
                    break;
                case 0x1E:
                    res.mnemonic = "ADD";
                    std::snprintf(buf, sizeof(buf), "I, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set index I = I + V%X", x);
                    res.description = buf;
                    break;
                case 0x29:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "F, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set I = 5-byte font digit in V%X", x);
                    res.description = buf;
                    break;
                case 0x30:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "HF, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Set I = 10-byte font digit in V%X", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                case 0x33:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "B, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Store BCD of V%X at I, I+1, I+2", x);
                    res.description = buf;
                    break;
                case 0x55:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "[I], V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Store V0..V%X to memory starting at I", x);
                    res.description = buf;
                    break;
                case 0x65:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "V%X, [I]", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Load V0..V%X from memory starting at I", x);
                    res.description = buf;
                    break;
                case 0x75:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "R, V%X", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Save V0..V%X to RPL user flags", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                case 0x85:
                    res.mnemonic = "LD";
                    std::snprintf(buf, sizeof(buf), "V%X, R", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "Load V0..V%X from RPL user flags", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                default:
                    res.mnemonic = "DATA";
                    std::snprintf(buf, sizeof(buf), "0x%04X", opcode);
                    res.operands = buf;
                    res.description = "Unknown opcode";
                    break;
            }
            break;
    }

    if (res.operands.empty()) {
        res.full_text = res.mnemonic;
    } else {
        res.full_text = res.mnemonic + " " + res.operands;
    }

    return res;
}

inline std::vector<DisasmResult> disassemble_memory_range(const uint8_t* memory, uint16_t start_addr, uint16_t end_addr, bool quirk_jump_vx = false) {
    std::vector<DisasmResult> results;
    if (!memory || start_addr > end_addr) return results;

    for (uint32_t addr = start_addr; addr + 1 <= end_addr && addr < 65536; addr += 2) {
        uint16_t op = (static_cast<uint16_t>(memory[addr]) << 8) | memory[addr + 1];
        results.push_back(disassemble_instruction(static_cast<uint16_t>(addr), op, quirk_jump_vx));
    }
    return results;
}

#endif 
