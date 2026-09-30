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
                
                res.mnemonic = "scd";
                std::snprintf(buf, sizeof(buf), "%u", n);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "scroll display down %u lines", n);
                res.description = buf;
                res.is_schip = true;
            } else {
                switch (opcode & 0x00FF) {
                    case 0xE0:
                        res.mnemonic = "cls";
                        res.operands = "";
                        res.description = "clear screen display";
                        break;
                    case 0xEE:
                        res.mnemonic = "ret";
                        res.operands = "";
                        res.description = "return from subroutine";
                        break;
                    case 0xFB:
                        res.mnemonic = "scr";
                        res.operands = "";
                        res.description = "scroll display right 4 pixels";
                        res.is_schip = true;
                        break;
                    case 0xFC:
                        res.mnemonic = "scl";
                        res.operands = "";
                        res.description = "scroll display left 4 pixels";
                        res.is_schip = true;
                        break;
                    case 0xFD:
                        res.mnemonic = "exit";
                        res.operands = "";
                        res.description = "halt / exit interpreter";
                        res.is_schip = true;
                        break;
                    case 0xFE:
                        res.mnemonic = "low";
                        res.operands = "";
                        res.description = "disable extended mode (set 64x32)";
                        res.is_schip = true;
                        break;
                    case 0xFF:
                        res.mnemonic = "high";
                        res.operands = "";
                        res.description = "enable extended mode (set 128x64)";
                        res.is_schip = true;
                        break;
                    default:
                        res.mnemonic = "sys";
                        std::snprintf(buf, sizeof(buf), "0x%03x", nnn);
                        res.operands = buf;
                        res.description = "call machine routine at " + res.operands;
                        break;
                }
            }
            break;

        case 0x1:
            res.mnemonic = "jp";
            std::snprintf(buf, sizeof(buf), "0x%03x", nnn);
            res.operands = buf;
            res.description = "jump to address " + res.operands;
            res.is_jump_or_call = true;
            res.target_addr = nnn;
            break;

        case 0x2:
            res.mnemonic = "call";
            std::snprintf(buf, sizeof(buf), "0x%03x", nnn);
            res.operands = buf;
            res.description = "call subroutine at " + res.operands;
            res.is_jump_or_call = true;
            res.target_addr = nnn;
            break;

        case 0x3:
            res.mnemonic = "se";
            std::snprintf(buf, sizeof(buf), "v%x, 0x%02x", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "skip next if v%x == 0x%02x (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x4:
            res.mnemonic = "sne";
            std::snprintf(buf, sizeof(buf), "v%x, 0x%02x", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "skip next if v%x != 0x%02x (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x5:
            if (n == 0) {
                res.mnemonic = "se";
                std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "skip next if v%x == v%x", x, y);
                res.description = buf;
            } else {
                res.mnemonic = "data";
                std::snprintf(buf, sizeof(buf), "0x%04x", opcode);
                res.operands = buf;
                res.description = "unknown / raw data";
            }
            break;

        case 0x6:
            res.mnemonic = "ld";
            std::snprintf(buf, sizeof(buf), "v%x, 0x%02x", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "set v%x = 0x%02x (%u)", x, kk, kk);
            res.description = buf;
            break;

        case 0x7:
            res.mnemonic = "add";
            std::snprintf(buf, sizeof(buf), "v%x, 0x%02x", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "set v%x = v%x + 0x%02x (%u)", x, x, kk, kk);
            res.description = buf;
            break;

        case 0x8:
            switch (n) {
                case 0x0:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x", x, y);
                    res.description = buf;
                    break;
                case 0x1:
                    res.mnemonic = "or";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x | v%x", x, x, y);
                    res.description = buf;
                    break;
                case 0x2:
                    res.mnemonic = "and";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x & v%x", x, x, y);
                    res.description = buf;
                    break;
                case 0x3:
                    res.mnemonic = "xor";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x ^ v%x", x, x, y);
                    res.description = buf;
                    break;
                case 0x4:
                    res.mnemonic = "add";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x + v%x, vf = carry", x, x, y);
                    res.description = buf;
                    break;
                case 0x5:
                    res.mnemonic = "sub";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x - v%x, vf = !borrow", x, x, y);
                    res.description = buf;
                    break;
                case 0x6:
                    res.mnemonic = "shr";
                    std::snprintf(buf, sizeof(buf), "v%x {, v%x}", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x >> 1, vf = lsb", x, x);
                    res.description = buf;
                    break;
                case 0x7:
                    res.mnemonic = "subn";
                    std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x - v%x, vf = !borrow", x, y, x);
                    res.description = buf;
                    break;
                case 0xE:
                    res.mnemonic = "shl";
                    std::snprintf(buf, sizeof(buf), "v%x {, v%x}", x, y);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = v%x << 1, vf = msb", x, x);
                    res.description = buf;
                    break;
                default:
                    res.mnemonic = "data";
                    std::snprintf(buf, sizeof(buf), "0x%04x", opcode);
                    res.operands = buf;
                    res.description = "unknown opcode";
                    break;
            }
            break;

        case 0x9:
            if (n == 0) {
                res.mnemonic = "sne";
                std::snprintf(buf, sizeof(buf), "v%x, v%x", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "skip next if v%x != v%x", x, y);
                res.description = buf;
            } else {
                res.mnemonic = "data";
                std::snprintf(buf, sizeof(buf), "0x%04x", opcode);
                res.operands = buf;
                res.description = "unknown opcode";
            }
            break;

        case 0xA:
            res.mnemonic = "ld";
            std::snprintf(buf, sizeof(buf), "i, 0x%03x", nnn);
            res.operands = buf;
            res.description = "set index i = 0x" + std::string(buf).substr(std::string(buf).find("0x") + 2);
            break;

        case 0xB:
            res.mnemonic = "jp";
            if (quirk_jump_vx) {
                std::snprintf(buf, sizeof(buf), "v%x, 0x%03x", x, nnn);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "jump to 0x%03x + v%x (schip)", nnn, x);
                res.description = buf;
                res.is_schip = true;
            } else {
                std::snprintf(buf, sizeof(buf), "v0, 0x%03x", nnn);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "jump to 0x%03x + v0", nnn);
                res.description = buf;
            }
            res.is_jump_or_call = true;
            break;

        case 0xC:
            res.mnemonic = "rnd";
            std::snprintf(buf, sizeof(buf), "v%x, 0x%02x", x, kk);
            res.operands = buf;
            std::snprintf(buf, sizeof(buf), "set v%x = random_byte & 0x%02x", x, kk);
            res.description = buf;
            break;

        case 0xD:
            res.mnemonic = "drw";
            if (n == 0) {
                std::snprintf(buf, sizeof(buf), "v%x, v%x, 0", x, y);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "draw 16x16 sprite at (v%x, v%x)", x, y);
                res.description = buf;
                res.is_schip = true;
            } else {
                std::snprintf(buf, sizeof(buf), "v%x, v%x, %u", x, y, n);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "draw 8x%u sprite at (v%x, v%x)", n, x, y);
                res.description = buf;
            }
            break;

        case 0xE:
            if (kk == 0x9E) {
                res.mnemonic = "skp";
                std::snprintf(buf, sizeof(buf), "v%x", x);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "skip next if key in v%x is pressed", x);
                res.description = buf;
            } else if (kk == 0xA1) {
                res.mnemonic = "sknp";
                std::snprintf(buf, sizeof(buf), "v%x", x);
                res.operands = buf;
                std::snprintf(buf, sizeof(buf), "skip next if key in v%x not pressed", x);
                res.description = buf;
            } else {
                res.mnemonic = "data";
                std::snprintf(buf, sizeof(buf), "0x%04x", opcode);
                res.operands = buf;
                res.description = "unknown opcode";
            }
            break;

        case 0xF:
            switch (kk) {
                case 0x07:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "v%x, dt", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set v%x = delay timer", x);
                    res.description = buf;
                    break;
                case 0x0A:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "v%x, k", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "wait for key press, store in v%x", x);
                    res.description = buf;
                    break;
                case 0x15:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "dt, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set delay timer = v%x", x);
                    res.description = buf;
                    break;
                case 0x18:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "st, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set sound timer = v%x", x);
                    res.description = buf;
                    break;
                case 0x1E:
                    res.mnemonic = "add";
                    std::snprintf(buf, sizeof(buf), "i, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set index i = i + v%x", x);
                    res.description = buf;
                    break;
                case 0x29:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "f, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set i = 5-byte font digit in v%x", x);
                    res.description = buf;
                    break;
                case 0x30:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "hf, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "set i = 10-byte font digit in v%x", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                case 0x33:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "b, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "store bcd of v%x at i, i+1, i+2", x);
                    res.description = buf;
                    break;
                case 0x55:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "[i], v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "store v0..v%x to memory starting at i", x);
                    res.description = buf;
                    break;
                case 0x65:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "v%x, [i]", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "load v0..v%x from memory starting at i", x);
                    res.description = buf;
                    break;
                case 0x75:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "r, v%x", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "save v0..v%x to rpl user flags", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                case 0x85:
                    res.mnemonic = "ld";
                    std::snprintf(buf, sizeof(buf), "v%x, r", x);
                    res.operands = buf;
                    std::snprintf(buf, sizeof(buf), "load v0..v%x from rpl user flags", x);
                    res.description = buf;
                    res.is_schip = true;
                    break;
                default:
                    res.mnemonic = "data";
                    std::snprintf(buf, sizeof(buf), "0x%04x", opcode);
                    res.operands = buf;
                    res.description = "unknown opcode";
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
