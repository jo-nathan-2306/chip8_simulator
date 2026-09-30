# CHIP-8 / Super-CHIP Emulator

A feature-complete CHIP-8 and Super-CHIP (SCHIP) emulator built in C++ with SDL2 and Dear ImGui.

## Features

- **Full CHIP-8 + Super-CHIP (SCHIP) Opcode Set** — all 35 standard + all SCHIP extensions
- **128×64 High-Resolution Mode** (`00FF`/`00FE`), 16×16 sprite rendering (`DXY0`), 8×10 font sprites (`FX30`)
- **Hardware Scrolling**: Down (`00CN`), Right (`00FB`), Left (`00FC`)
- **HP-48 RPL Persistent User Flags** (`FX75` / `FX85`)
- **Dear ImGui UI Overlay** — full menu bar, control panel, dialogs
- **Live Debugger** with disassembler, breakpoints, trace log, memory hex viewer, and register delta highlighting
- **Built-in CHIP-8 Assembler** with 4 template programs and syntax error reporting
- **ROM File Browser** with SCHIP auto-detection, search/filter, and recent ROMs list
- **Multi-waveform Audio Synthesizer** — Sine, Square (configurable duty cycle), Sawtooth, Triangle, Noise
- **8 Authentic Retro Color Palettes** plus custom RGB color picker
- **Savestate Support** — 5 slots, versioned binary format (v1/v2)
- **Custom Keyboard Mapping** — 3 layout presets (QWERTY, Numpad, WASD) + per-key rebind
- **5 Configurable Compatibility Quirks** — Shift, VF reset, Load/Store, Jump, Legacy scroll

## Architecture

CHIP-8 is a virtual machine from the 1970s designed to make programming video games easier on early microcomputers.

### System Specifications

- **Memory**: up to 64 KB (65536 bytes)
  - `0x000-0x04F`: Standard 5-byte font sprites (digits 0–F)
  - `0x050-0x0EF`: Super-CHIP 10-byte high-res font sprites (digits 0–F)
  - `0x200-0xFFFF`: Program / ROM space
- **Registers**:
  - 16 8-bit general-purpose registers (V0–VF)
  - VF doubles as a flag register for arithmetic operations
  - 16-bit index register (I)
  - 16-bit program counter (PC)
  - 8-bit stack pointer (SP) with 16-level call stack
  - 16 HP-48 RPL persistent user flags (R0–RF)
- **Display**: 64×32 (CHIP-8) or 128×64 (Super-CHIP), monochrome
- **Timers**:
  - Delay timer (counts down at 60 Hz)
  - Sound timer (beeps when > 0, counts down at 60 Hz)
- **Keypad**: 16-key hexadecimal input (customizable mapping)

## Dependencies

- SDL2
- Dear ImGui (bundled in `external/imgui`)

### Installation

**Ubuntu / Debian:**
```bash
sudo apt-get install libsdl2-dev
```

**Arch Linux:**
```bash
sudo pacman -S sdl2
```

**macOS:**
```bash
brew install sdl2
```

## Building

```bash
git clone <repo-url>
cd chip8-emulator
make
```

## Usage

```bash
./chip8                   # Opens ROM browser automatically
./chip8 <path-to-rom>     # Load a specific ROM on startup
```

**Examples:**
```bash
./chip8 roms/pong.ch8
./chip8 roms/eaty.ch8
```

## Keyboard Mapping

Default QWERTY layout (rebindable via Ctrl+K or Tools → Keyboard Mapping):

```
CHIP-8 Keypad:          QWERTY Keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│C│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│D│               │Q│W│E│R│
├─┼─┼─┼─┤      =        ├─┼─┼─┼─┤
│7│8│9│E│               │A│S│D│F│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│A│0│B│F│               │Z│X│C│V│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

### Keyboard Shortcuts

| Feature | Key Shortcut | Menu / GUI |
|:---|:---|:---|
| **Open ROM Browser** | `Ctrl+O` | File → Open ROM |
| **Debugger / Disassembler** | `F12` / `Ctrl+D` | Debug → Disassembler |
| **Code Editor / Assembler** | `Ctrl+E` | Tools → Code Editor |
| **Keyboard Mapping** | `Ctrl+K` | Tools → Keyboard Mapping |
| **Reset VM / Reload ROM** | `Ctrl+R` | File → Reset VM |
| **Pause / Resume** | `Space` / `P` | Emulation → Pause |
| **Step 1 Frame** | `.` / `N` / `F11` | Debug → Step Frame |
| **Step Single Instruction** | `F10` | Debug → Step Cycle |
| **Step Over Subroutine** | `Shift+F10` | Debug → Step Over |
| **Toggle Breakpoint at PC** | `F9` | Debug → Toggle Breakpoint |
| **Speed Up** | `+` / `=` / `Up` / `]` | Control Panel Slider |
| **Slow Down** | `-` / `Down` / `[` | Control Panel Slider |
| **Fast Speed Step** | `PageUp` / `PageDown` | — |
| **Reset Speed (600 Hz)** | `0` / `Backspace` | Control Panel → 600 Hz |
| **Control Panel** | `F2` | Tools → Control Panel |
| **Cycle Color Palette** | `Tab` / `T` | Color Themes Menu |
| **Prev Color Palette** | `Shift+Tab` | — |
| **Save State** | `F5` / `Ctrl+S` | File → Quick Save |
| **Load State** | `F6` / `Ctrl+L` | File → Quick Load |
| **Cycle Save Slot (1–5)** | `F7` | Control Panel |
| **Help / Shortcuts** | `H` / `F1` | Help Menu |
| **Quit** | `Esc` | File → Quit |

## Compatibility Quirks

Configure CHIP-8 vs Super-CHIP compatibility via Debugger → **SCHIP & Quirks** tab:

| Quirk | Default | Description |
|:---|:---|:---|
| **Bit Shift** | `true` (SCHIP) | `8XY6`/`8XYE` shifts VX in place; when off, copies VY→VX first (VIP) |
| **VF Reset** | `false` (SCHIP) | When on, `8XY0/1/2/3` reset VF=0 after the operation (COSMAC VIP) |
| **Load/Store** | `true` (SCHIP) | `FX55`/`FX65` leaves I unchanged; when off, I += X+1 (VIP) |
| **Jump** | `false` (CHIP-8) | When on, `BXNN` jumps to XNN+VX instead of NNN+V0 (SCHIP) |
| **Legacy Scroll** | `false` | When on, lores scroll moves half-pixels (original HP-48 hardware) |

> Test ROM compatibility using the included `roms/4-flags.ch8` and `roms/5-quirks.ch8`.

## Implementation Details

### Instruction Set

Full CHIP-8 + Super-CHIP instruction set:
- **Arithmetic**: ADD, SUB, SUBN, AND, OR, XOR, SHR, SHL — with configurable quirks
- **Graphics**: XOR sprite drawing with collision detection; 8×N standard + 16×16 SCHIP mode
- **Flow Control**: JP, CALL/RET, SE/SNE conditional skips
- **Memory**: LD/ST registers, BCD conversion, RPL flags store/restore
- **Timers**: Delay (FX07/FX15) and Sound (FX18) timer operations
- **Input**: FX0A (blocking wait for key), EX9E/EXA1 (skip on key state)
- **Super-CHIP**: High/low resolution switch, hardware scrolling, large sprites, 10-byte fonts

### Display

Graphics rendered via SDL2 + ImGui:
- CHIP-8: 64×32 pixels, each scaled 10× → 640×320 viewport
- SCHIP: 128×64 pixels, each scaled 5× → 640×320 viewport (same physical size)
- XOR-mode sprite drawing with VF collision flag
- 60 FPS rendering with frame-timing cap

### Audio

Multi-waveform SDL2 audio synthesizer (44100 Hz, 16-bit mono):
- **Sine** — smooth warm tone
- **Square / Pulse** — classic 8-bit beep (configurable duty cycle 10%–90%)
- **Sawtooth** — arcade buzz
- **Triangle** — retro bass
- **Noise** — 16-bit Galois LFSR static

### Savestate Format

Binary format (`C8ST` magic, version 2):
- Saves: PC, Opcode, I, SP, Timers, Draw_Flag, Extended_Mode, Halted, V0–VF, Stack, Keypad, RPL Flags, Display Buffer (128×64), full 64 KB RAM

## Resources

- [CHIP-8 Technical Reference](http://devernay.free.fr/hacks/chip8/c8tech10.htm)
- [Super-CHIP Specification](https://github.com/trapexit/chip-8_documentation)
- [CHIP-8 Test Suite ROMs](https://github.com/timendus/chip8-test-suite)
- [CHIP-8 ROMs Archive](https://github.com/kripod/chip8-roms)
