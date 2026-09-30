#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <string>

class Chip8{
    public:
        Chip8();
        bool load_rom(const std::string& filename); 
        bool load_from_bytes(const uint8_t* data, size_t size, const std::string& name = "assembled_code"); 
        bool save_state(const std::string& filename) const; 
        bool load_state(const std::string& filename); 
        void emulate_cycle(); 
        void update_timers(); 
        bool draw_flag; 
        uint8_t display[128 * 64]; 
        uint8_t key[16]; 

        
        size_t get_rom_size() const { return rom_size; }
        const std::string& get_rom_filename() const { return rom_path; }

        
        bool is_extended_mode() const { return extended_mode; }
        void set_extended_mode(bool ext) { extended_mode = ext; }
        int get_display_width() const { return extended_mode ? 128 : 64; }
        int get_display_height() const { return extended_mode ? 64 : 32; }
        bool is_halted() const { return halted; }
        void set_halted(bool h) { halted = h; }
        uint8_t get_rpl_flag(int i) const { return (i >= 0 && i < 16) ? rpl_flags[i] : 0; }
        void set_rpl_flag(int i, uint8_t val) { if (i >= 0 && i < 16) rpl_flags[i] = val; }

        void scroll_up(int lines);
        void scroll_down(int lines);
        void scroll_right();
        void scroll_left();
        void clear_screen();

        uint8_t get_sound_timer() const { return sound_timer; } 
        uint8_t get_delay_timer() const { return delay_timer; }
        uint16_t get_pc() const { return pc; }
        uint8_t get_v(int i) const { return (i >= 0 && i < 16) ? v[i] : 0; }
        uint16_t get_index() const { return index; }
        uint8_t get_sp() const { return sp; }
        uint16_t get_stack(int i) const { return (i >= 0 && i < 16) ? stack[i] : 0; }
        const uint16_t* get_stack_array() const { return stack; }
        uint16_t get_opcode() const { return opcode; }
        const uint8_t* get_memory() const { return memory; }

        
        void set_pc(uint16_t new_pc) { pc = new_pc; }
        void set_index(uint16_t new_idx) { index = new_idx; }
        void set_sp(uint8_t new_sp) { sp = (new_sp <= 16) ? new_sp : 16; }
        void set_v(int i, uint8_t val) { if (i >= 0 && i < 16) v[i] = val; }
        void set_delay_timer(uint8_t val) { delay_timer = val; }
        void set_sound_timer(uint8_t val) { sound_timer = val; }
        void set_stack(int i, uint16_t val) { if (i >= 0 && i < 16) stack[i] = val; }
        void set_memory_byte(uint16_t addr, uint8_t val) { memory[addr & 0xFFFF] = val; }

        
        bool get_quirk_shift_vx() const { return quirk_shift_vx; }
        void set_quirk_shift_vx(bool val) { quirk_shift_vx = val; }
        bool get_quirk_load_store_no_i() const { return quirk_load_store_no_i; }
        void set_quirk_load_store_no_i(bool val) { quirk_load_store_no_i = val; }
        bool get_quirk_jump_vx() const { return quirk_jump_vx; }
        void set_quirk_jump_vx(bool val) { quirk_jump_vx = val; }
        bool get_quirk_legacy_scroll() const { return quirk_legacy_scroll; }
        void set_quirk_legacy_scroll(bool val) { quirk_legacy_scroll = val; }
        bool get_quirk_vf_reset() const { return quirk_vf_reset; }
        void set_quirk_vf_reset(bool val) { quirk_vf_reset = val; }

        void reset() { initialise(); }
    private:
        uint8_t memory[65536]; 
        uint8_t v[16]; 
        uint16_t index; 
        uint16_t pc; 
        uint16_t stack[16];
        uint8_t sp; 
        uint8_t delay_timer; 
        uint8_t sound_timer; 
        uint16_t opcode; 
        bool extended_mode; 
        bool halted; 
        uint8_t rpl_flags[16]; 
        size_t rom_size = 0; 
        std::string rom_path = ""; 

        
        bool quirk_shift_vx = true;        
        bool quirk_load_store_no_i = true; 
        bool quirk_jump_vx = false;        
        bool quirk_legacy_scroll = false;  
        bool quirk_vf_reset = false;       

        void initialise(); 
        void load_fonts(); 
};

#endif