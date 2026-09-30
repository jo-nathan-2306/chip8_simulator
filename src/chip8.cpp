#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>

uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 
    0x20, 0x60, 0x20, 0x20, 0x70, 
    0xF0, 0x10, 0xF0, 0x80, 0xF0, 
    0xF0, 0x10, 0xF0, 0x10, 0xF0, 
    0x90, 0x90, 0xF0, 0x10, 0x10, 
    0xF0, 0x80, 0xF0, 0x10, 0xF0, 
    0xF0, 0x80, 0xF0, 0x90, 0xF0, 
    0xF0, 0x10, 0x20, 0x40, 0x40, 
    0xF0, 0x90, 0xF0, 0x90, 0xF0, 
    0xF0, 0x90, 0xF0, 0x10, 0xF0, 
    0xF0, 0x90, 0xF0, 0x90, 0x90, 
    0xE0, 0x90, 0xE0, 0x90, 0xE0, 
    0xF0, 0x80, 0x80, 0x80, 0xF0, 
    0xE0, 0x90, 0x90, 0x90, 0xE0, 
    0xF0, 0x80, 0xF0, 0x80, 0xF0, 
    0xF0, 0x80, 0xF0, 0x80, 0x80  
};


uint8_t schip_fontset[160] = {
    0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 
    0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 
    0x3E, 0x66, 0x06, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x66, 0x7E, 
    0x3C, 0x66, 0x06, 0x06, 0x1C, 0x06, 0x06, 0x06, 0x66, 0x3C, 
    0x34, 0x3C, 0x6C, 0x6C, 0x66, 0x66, 0x7E, 0x06, 0x06, 0x0F, 
    0x7E, 0x60, 0x60, 0x60, 0x7C, 0x06, 0x06, 0x06, 0x66, 0x3C, 
    0x1C, 0x30, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x3C, 
    0x7E, 0x66, 0x06, 0x06, 0x0C, 0x18, 0x18, 0x30, 0x30, 0x30, 
    0x3C, 0x66, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 
    0x3C, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x06, 0x0C, 0x38, 
    0x18, 0x3C, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x66, 
    0x7C, 0x66, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x7C, 
    0x3C, 0x66, 0x66, 0x60, 0x60, 0x60, 0x60, 0x66, 0x66, 0x3C, 
    0x78, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6C, 0x78, 
    0x7E, 0x60, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x60, 0x7E, 
    0x7E, 0x60, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x60, 0x60  
};

Chip8::Chip8(){
    initialise();
}

void Chip8::initialise(){
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;
    extended_mode = false;
    halted = false;

    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));
    memset(v, 0, sizeof(v));
    memset(memory, 0, sizeof(memory));
    memset(key, 0, sizeof(key));
    memset(rpl_flags, 0, sizeof(rpl_flags));

    load_fonts();
    delay_timer = 0;
    sound_timer = 0;
    draw_flag = false;
}

void Chip8::load_fonts(){
    
    for(int i=0; i<80; i++) memory[i] = chip8_fontset[i];
    
    for(int i=0; i<160; i++) memory[80+i] = schip_fontset[i];
}

void Chip8::clear_screen() {
    std::memset(display, 0, sizeof(display));
    draw_flag = true;
}

void Chip8::scroll_up(int lines) {
    if (lines <= 0) return;
    if (quirk_legacy_scroll && !extended_mode) {
        lines = lines / 2;
        if (lines <= 0) return;
    }
    int w = get_display_width();
    int h = get_display_height();
    if (lines > h) lines = h;

    for (int y = 0; y < h - lines; y++) {
        for (int x = 0; x < w; x++) {
            display[x + y * w] = display[x + (y + lines) * w];
        }
    }
    for (int y = h - lines; y < h; y++) {
        for (int x = 0; x < w; x++) {
            display[x + y * w] = 0;
        }
    }
    draw_flag = true;
}

void Chip8::scroll_down(int lines) {
    if (lines <= 0) return;
    if (quirk_legacy_scroll && !extended_mode) {
        lines = lines / 2;
        if (lines <= 0) return;
    }
    int w = get_display_width();
    int h = get_display_height();
    if (lines > h) lines = h;

    for (int y = h - 1; y >= lines; y--) {
        for (int x = 0; x < w; x++) {
            display[x + y * w] = display[x + (y - lines) * w];
        }
    }
    for (int y = 0; y < lines; y++) {
        for (int x = 0; x < w; x++) {
            display[x + y * w] = 0;
        }
    }
    draw_flag = true;
}

void Chip8::scroll_right() {
    int w = get_display_width();
    int h = get_display_height();
    int shift = (quirk_legacy_scroll && !extended_mode) ? 2 : 4;
    for (int y = 0; y < h; y++) {
        for (int x = w - 1; x >= shift; x--) {
            display[x + y * w] = display[(x - shift) + y * w];
        }
        for (int x = 0; x < shift && x < w; x++) {
            display[x + y * w] = 0;
        }
    }
    draw_flag = true;
}

void Chip8::scroll_left() {
    int w = get_display_width();
    int h = get_display_height();
    int shift = (quirk_legacy_scroll && !extended_mode) ? 2 : 4;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x <= w - (shift + 1); x++) {
            display[x + y * w] = display[(x + shift) + y * w];
        }
        for (int x = std::max(0, w - shift); x < w; x++) {
            display[x + y * w] = 0;
        }
    }
    draw_flag = true;
}

bool Chip8::load_rom(const std::string& filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file.is_open()){
        std::cerr << "failed to open rom: " << filename << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if(size > (std::streamsize)(sizeof(memory) - 512)){ 
        std::cerr << "rom too large to fit in memory (" << size << " bytes)" << std::endl;
        return false;
    }

    initialise();
    file.read((char*)(memory+512), size);
    file.close();

    rom_size = static_cast<size_t>(size);
    rom_path = filename;

    std::cout << "loaded rom: " << filename << " (" << size << " bytes)" << std::endl;
    return true;
}

bool Chip8::load_from_bytes(const uint8_t* data, size_t size, const std::string& name){
    if (!data || size == 0 || size > (sizeof(memory) - 512)) {
        std::cerr << "invalid buffer or size to load (" << size << " bytes)" << std::endl;
        return false;
    }
    initialise();
    std::memcpy(memory + 512, data, size);
    rom_size = size;
    rom_path = name;
    std::cout << "loaded rom from memory buffer: " << name << " (" << size << " bytes)" << std::endl;
    return true;
}

void Chip8::emulate_cycle(){
    if (halted) return;
    opcode = memory[pc] << 8 | memory[pc+1]; 

    switch(opcode & 0xF000){ 
        case 0x0000:
            if ((opcode & 0x00F0) == 0x00C0) {
                
                uint8_t lines = opcode & 0x000F;
                scroll_down(lines);
                pc += 2;
                break;
            }
            if ((opcode & 0x00F0) == 0x00D0) {
                
                uint8_t lines = opcode & 0x000F;
                scroll_up(lines);
                pc += 2;
                break;
            }
            switch(opcode & 0x00FF){
                case 0x00E0: 
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE: 
                    if(sp > 0){
                        sp--;
                        pc = stack[sp];
                    }
                    pc += 2;
                    break;
                case 0x00FB: 
                    scroll_right();
                    pc += 2;
                    break;
                case 0x00FC: 
                    scroll_left();
                    pc += 2;
                    break;
                case 0x00FD: 
                    halted = true;
                    std::cout << "[schip] opcode 00fd: program halted / exit" << std::endl;
                    pc += 2;
                    break;
                case 0x00FE: 
                    extended_mode = false;
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00FF: 
                    extended_mode = true;
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                default:
                    std::cerr << "unknown opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
            }
            break;
        case 0x1000: 
            pc = opcode & 0x0FFF;
            break;
        case 0x2000: 
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000: 
            if(v[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;
        case 0x4000:  
            if (v[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
            else pc += 2;
            break;
        case 0x5000:  
            if (v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4]) pc += 4;
            else pc += 2;
            break;
        case 0x6000: 
            v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            pc += 2;
            break;
        case 0x7000: 
            v[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            pc += 2;
            break;
        case 0x8000: 
            switch(opcode & 0x000F){ 
                case 0x0000: 
                    v[(opcode & 0x0F00) >> 8] = v[(opcode & 0x00F0) >> 4];
                    if (quirk_vf_reset) v[0xF] = 0; 
                    pc += 2;
                    break;
                case 0x0001: 
                    v[(opcode & 0x0F00) >> 8] |= v[(opcode & 0x00F0) >> 4];
                    if (quirk_vf_reset) v[0xF] = 0; 
                    pc += 2;
                    break;
                case 0x0002: 
                    v[(opcode & 0x0F00) >> 8] &= v[(opcode & 0x00F0) >> 4];
                    if (quirk_vf_reset) v[0xF] = 0; 
                    pc += 2;
                    break;
                case 0x0003: 
                    v[(opcode & 0x0F00) >> 8] ^= v[(opcode & 0x00F0) >> 4];
                    if (quirk_vf_reset) v[0xF] = 0; 
                    pc += 2;
                    break;
                case 0x0004:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint16_t sum = v[x] + v[y];
                    v[x] = sum & 0xFF;
                    v[0xF] = (sum > 0xFF) ? 1 : 0;
                    pc += 2;
                }
                    break;
                case 0x0005:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t flag = (v[x] >= v[y]) ? 1 : 0;
                    v[x] -= v[y];
                    v[0xF] = flag;
                    pc += 2;
                }
                    break;
                case 0x0006:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t val = quirk_shift_vx ? v[x] : v[y];
                    uint8_t flag = val & 0x1;
                    v[x] = val >> 1;
                    v[0xF] = flag;
                    pc += 2;
                }
                    break;
                case 0x0007:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t flag = (v[y] >= v[x]) ? 1 : 0;
                    v[x] = v[y] - v[x];
                    v[0xF] = flag;
                    pc += 2;
                }
                    break;
                case 0x000E:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    uint8_t y = (opcode & 0x00F0) >> 4;
                    uint8_t val = quirk_shift_vx ? v[x] : v[y];
                    uint8_t flag = (val >> 7) & 0x1;
                    v[x] = val << 1;
                    v[0xF] = flag;
                    pc += 2;
                }
                    break;
                default:
                    std::cerr << "unknown opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0x9000: 
            if (v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4])
                pc += 4;
            else
                pc += 2;
            break;
        case 0xA000: 
            index = opcode & 0x0FFF;
            pc += 2;
            break;
        case 0xB000: 
            if (quirk_jump_vx) {
                uint8_t x = (opcode & 0x0F00) >> 8;
                pc = (opcode & 0x0FFF) + v[x];
            } else {
                pc = (opcode & 0x0FFF) + v[0];
            }
            break;
        case 0xC000:{  
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, 255);
            v[(opcode & 0x0F00) >> 8] = dist(gen) & (opcode & 0x00FF);
            pc += 2;
        }
            break;
        case 0xD000:{ 
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t vy = (opcode & 0x00F0) >> 4;
            uint8_t height = opcode & 0x000F;
            int screen_w = get_display_width();
            int screen_h = get_display_height();

            int start_x = v[vx] % screen_w;
            int start_y = v[vy] % screen_h;

            v[0xF] = 0; 

            if (height == 0) {
                
                for (int row = 0; row < 16; row++) {
                    int screen_y = start_y + row;
                    if (screen_y >= screen_h) continue;

                    uint8_t byte1 = memory[index + (row * 2)];
                    uint8_t byte2 = memory[index + (row * 2) + 1];
                    uint16_t row_bits = (static_cast<uint16_t>(byte1) << 8) | byte2;

                    for (int col = 0; col < 16; col++) {
                        int screen_x = start_x + col;
                        if (screen_x >= screen_w) continue;

                        if ((row_bits & (0x8000 >> col)) != 0) {
                            int screen_index = screen_x + (screen_y * screen_w);
                            if (display[screen_index] == 1) {
                                v[0xF] = 1;
                            }
                            display[screen_index] ^= 1;
                        }
                    }
                }
            } else {
                
                for (int row = 0; row < height; row++) {
                    int screen_y = start_y + row;
                    if (screen_y >= screen_h) continue;

                    uint8_t pixel_byte = memory[index + row];
                    for (int col = 0; col < 8; col++) {
                        int screen_x = start_x + col;
                        if (screen_x >= screen_w) continue;

                        if ((pixel_byte & (0x80 >> col)) != 0) {
                            int screen_index = screen_x + (screen_y * screen_w);
                            if (display[screen_index] == 1) {
                                v[0xF] = 1;
                            }
                            display[screen_index] ^= 1;
                        }
                    }
                }
            }
            draw_flag = true;
            pc += 2;
        }
            break;
        case 0xE000: 
            switch(opcode & 0x00FF){
                case 0x009E: 
                    if(key[v[(opcode & 0x0F00) >> 8] & 0x0F] != 0) pc += 4;
                    else pc += 2;
                    break;
                case 0x00A1: 
                    if(key[v[(opcode & 0x0F00) >> 8] & 0x0F] == 0) pc += 4;
                    else pc += 2;
                    break;
                default:
                    std::cerr << "unknown opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
            }
            break;
        case 0xF000: 
            switch(opcode & 0x00FF){
                case 0x0007: 
                    v[(opcode & 0x0F00) >> 8] = delay_timer;
                    pc += 2;
                    break;
                case 0x000A:{ 
                    bool key_pressed = false;
                    for(int i=0; i<16; i++){
                        if(key[i] != 0){
                            v[(opcode & 0x0F00) >> 8] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if(!key_pressed){
                        return; 
                    }
                    pc += 2;
                }
                    break;
                case 0x0015: 
                    delay_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0018: 
                    sound_timer = v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x001E: 
                    index += v[(opcode & 0x0F00) >> 8];
                    pc += 2;
                    break;
                case 0x0029: 
                    index = (v[(opcode & 0x0F00) >> 8] & 0x0F) * 5;
                    pc += 2;
                    break;
                case 0x0030: 
                    index = 80 + ((v[(opcode & 0x0F00) >> 8] & 0x0F) * 10);
                    pc += 2;
                    break;
                case 0x0033:{ 
                    uint8_t value = v[(opcode & 0x0F00) >> 8];
                    memory[index] = value/100;
                    memory[index+1] = (value/10) % 10;
                    memory[index+2] = value%10;
                    pc += 2;
                }
                    break;
                case 0x0055:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i=0; i<=x; i++){
                        memory[(index+i) & 0xFFFF] = v[i];
                    }
                    if (!quirk_load_store_no_i) {
                        index = (index + x + 1) & 0xFFFF;
                    }
                    pc += 2;
                }
                    break;
                case 0x0065:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i=0; i<=x; i++){
                        v[i] = memory[(index+i) & 0xFFFF];
                    }
                    if (!quirk_load_store_no_i) {
                        index = (index + x + 1) & 0xFFFF;
                    }
                    pc += 2;
                }
                    break;
                case 0x0075:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i=0; i<=x && i<16; i++){
                        rpl_flags[i] = v[i];
                    }
                    pc += 2;
                }
                    break;
                case 0x0085:{ 
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    for(int i=0; i<=x && i<16; i++){
                        v[i] = rpl_flags[i];
                    }
                    pc += 2;
                }
                    break;
                default:
                    std::cerr << "unknown opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
            }
            break;
        default:
            std::cerr << "unknown opcode: 0x" << std::hex << opcode << std::dec << std::endl;
            pc += 2;
            break;
    }
}

void Chip8::update_timers(){
    if(delay_timer > 0) delay_timer--;
    if(sound_timer > 0){
        sound_timer--;
    }
}

bool Chip8::save_state(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[savestate] failed to open file for writing: " << filename << std::endl;
        return false;
    }

    const char magic[4] = {'c', '8', 's', 't'};
    const uint32_t version = 2; 
    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));

    
    out.write(reinterpret_cast<const char*>(&pc), sizeof(pc));
    out.write(reinterpret_cast<const char*>(&opcode), sizeof(opcode));
    out.write(reinterpret_cast<const char*>(&index), sizeof(index));
    out.write(reinterpret_cast<const char*>(&sp), sizeof(sp));
    out.write(reinterpret_cast<const char*>(&delay_timer), sizeof(delay_timer));
    out.write(reinterpret_cast<const char*>(&sound_timer), sizeof(sound_timer));
    uint8_t df = draw_flag ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&df), sizeof(df));

    
    uint8_t ext = extended_mode ? 1 : 0;
    uint8_t hlt = halted ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&ext), sizeof(ext));
    out.write(reinterpret_cast<const char*>(&hlt), sizeof(hlt));

    
    out.write(reinterpret_cast<const char*>(v), sizeof(v));

    
    out.write(reinterpret_cast<const char*>(stack), sizeof(stack));

    
    out.write(reinterpret_cast<const char*>(key), sizeof(key));

    
    out.write(reinterpret_cast<const char*>(rpl_flags), sizeof(rpl_flags));

    
    out.write(reinterpret_cast<const char*>(display), sizeof(display));

    
    out.write(reinterpret_cast<const char*>(memory), sizeof(memory));

    if (!out.good()) {
        std::cerr << "[savestate] error writing state to file: " << filename << std::endl;
        return false;
    }
    out.close();
    std::cout << "[savestate] saved state to: " << filename << " (v2)" << std::endl;
    return true;
}

bool Chip8::load_state(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[savestate] failed to open file for reading: " << filename << std::endl;
        return false;
    }

    char magic[4];
    in.read(magic, 4);
    if (!in.good() || std::memcmp(magic, "c8st", 4) != 0) {
        std::cerr << "[savestate] invalid savestate header in: " << filename << std::endl;
        return false;
    }

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1 && version != 2) {
        std::cerr << "[savestate] unsupported savestate version (" << version << ") in: " << filename << std::endl;
        return false;
    }

    
    uint16_t new_pc, new_opcode, new_index;
    uint8_t new_sp, new_delay, new_sound, new_df;
    uint8_t new_ext = 0, new_hlt = 0;
    uint8_t new_v[16];
    uint16_t new_stack[16];
    uint8_t new_key[16];
    uint8_t new_rpl[16] = {0};
    uint8_t new_display[128 * 64];
    uint8_t new_memory[65536];

    memset(new_display, 0, sizeof(new_display));
    memset(new_memory, 0, sizeof(new_memory));

    in.read(reinterpret_cast<char*>(&new_pc), sizeof(new_pc));
    in.read(reinterpret_cast<char*>(&new_opcode), sizeof(new_opcode));
    in.read(reinterpret_cast<char*>(&new_index), sizeof(new_index));
    in.read(reinterpret_cast<char*>(&new_sp), sizeof(new_sp));
    in.read(reinterpret_cast<char*>(&new_delay), sizeof(new_delay));
    in.read(reinterpret_cast<char*>(&new_sound), sizeof(new_sound));
    in.read(reinterpret_cast<char*>(&new_df), sizeof(new_df));

    if (version == 1) {
        in.read(reinterpret_cast<char*>(new_v), sizeof(new_v));
        in.read(reinterpret_cast<char*>(new_stack), sizeof(new_stack));
        in.read(reinterpret_cast<char*>(new_key), sizeof(new_key));
        in.read(reinterpret_cast<char*>(new_display), 64 * 32);
        in.read(reinterpret_cast<char*>(new_memory), 4096);
    } else { 
        in.read(reinterpret_cast<char*>(&new_ext), sizeof(new_ext));
        in.read(reinterpret_cast<char*>(&new_hlt), sizeof(new_hlt));
        in.read(reinterpret_cast<char*>(new_v), sizeof(new_v));
        in.read(reinterpret_cast<char*>(new_stack), sizeof(new_stack));
        in.read(reinterpret_cast<char*>(new_key), sizeof(new_key));
        in.read(reinterpret_cast<char*>(new_rpl), sizeof(new_rpl));
        in.read(reinterpret_cast<char*>(new_display), sizeof(new_display));
        in.read(reinterpret_cast<char*>(new_memory), sizeof(new_memory));
    }

    if (!in.good()) {
        std::cerr << "[savestate] corrupted or incomplete savestate file: " << filename << std::endl;
        return false;
    }
    in.close();

    
    pc = new_pc;
    opcode = new_opcode;
    index = new_index;
    sp = (new_sp <= 16) ? new_sp : 0;
    delay_timer = new_delay;
    sound_timer = new_sound;
    draw_flag = (new_df != 0);
    extended_mode = (new_ext != 0);
    halted = (new_hlt != 0);

    std::memcpy(v, new_v, sizeof(v));
    std::memcpy(stack, new_stack, sizeof(stack));
    std::memcpy(rpl_flags, new_rpl, sizeof(rpl_flags));
    
    std::memset(key, 0, sizeof(key));
    std::memcpy(display, new_display, sizeof(display));
    std::memcpy(memory, new_memory, sizeof(memory));

    std::cout << "[savestate] loaded state from: " << filename << " (pc: 0x" << std::hex << pc << std::dec 
              << ", mode: " << (extended_mode ? "128x64 schip" : "64x32 chip-8") << ")" << std::endl;
    return true;
}