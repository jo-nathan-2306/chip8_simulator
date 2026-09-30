# chip-8 / super-chip emulator

a feature-complete chip-8 and super-chip (schip) emulator built in c++ with sdl2 and dear imgui.

## features

- **full chip-8 + super-chip (schip) opcode set** — all 35 standard + all schip extensions
- **128×64 high-resolution mode** (00ff/00fe), 16×16 sprite rendering (dxy0), 8×10 font sprites (fx30)
- **hardware scrolling**: down (00cn), right (00fb), left (00fc)
- **hp-48 rpl persistent user flags** (fx75 / fx85)
- **dear imgui ui overlay** — full menu bar, control panel, dialogs
- **live debugger** with disassembler, breakpoints, trace log, memory hex viewer, and register delta highlighting
- **built-in chip-8 assembler** with 4 template programs and syntax error reporting
- **rom file browser** with schip auto-detection, search/filter, and recent roms list
- **multi-waveform audio synthesizer** — sine, square (configurable duty cycle), sawtooth, triangle, noise
- **8 authentic retro color palettes** plus custom rgb color picker
- **savestate support** — 5 slots, versioned binary format (v1/v2)
- **custom keyboard mapping** — 3 layout presets (qwerty, numpad, wasd) + per-key rebind
- **5 configurable compatibility quirks** — shift, vf reset, load/store, jump, legacy scroll

## architecture

chip-8 is a virtual machine from the 1970s designed to make programming video games easier on early microcomputers.

### system specifications

- **memory**: up to 64 kb (65536 bytes)
  - `0x000-0x04f`: standard 5-byte font sprites (digits 0–f)
  - `0x050-0x0ef`: super-chip 10-byte high-res font sprites (digits 0–f)
  - `0x200-0xffff`: program/rom space
- **registers**:
  - 16 8-bit general-purpose registers (v0–vf)
  - vf doubles as a flag register for arithmetic operations
  - 16-bit index register (i)
  - 16-bit program counter (pc)
  - 8-bit stack pointer (sp) with 16-level call stack
  - 16 hp-48 rpl persistent user flags (r0–rf)
- **display**: 64×32 (chip-8) or 128×64 (super-chip), monochrome
- **timers**:
  - delay timer (counts down at 60 hz)
  - sound timer (beeps when > 0, counts down at 60 hz)
- **keypad**: 16-key hexadecimal input (customizable mapping)

## dependencies

- sdl2
- dear imgui (bundled in `external/imgui`)

### installation

**ubuntu/debian:**
```bash
sudo apt-get install libsdl2-dev
```

**arch linux:**
```bash
sudo pacman -s sdl2
```

**macos:**
```bash
brew install sdl2
```

## building

```bash
git clone <repo-url>
cd chip8-emulator
make
```

## usage

```bash
./chip8                   # opens rom browser automatically
./chip8 <path-to-rom>     # load a specific rom on startup
```

**example:**
```bash
./chip8 roms/pong.ch8
./chip8 roms/eaty.ch8
```

## keyboard mapping

default qwerty layout (rebindable via ctrl+k or tools → keyboard mapping):

```
chip-8 keypad:          qwerty keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│c│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│d│               │q│w│e│r│
├─┼─┼─┼─┤      =        ├─┼─┼─┼─┤
│7│8│9│e│               │a│s│d│f│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│a│0│b│f│               │z│x│c│v│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

### keyboard shortcuts

| feature | key shortcut | menu / gui |
|:---|:---|:---|
| **open rom browser** | `ctrl+o` | file → open rom |
| **debugger / disassembler** | `f12` / `ctrl+d` | debug → disassembler |
| **code editor / assembler** | `ctrl+e` | tools → code editor |
| **keyboard mapping** | `ctrl+k` | tools → keyboard mapping |
| **reset vm / reload rom** | `ctrl+r` | file → reset vm |
| **pause / resume** | `space` / `p` | emulation → pause |
| **step 1 frame** | `.` / `n` / `f11` | debug → step frame |
| **step single instruction** | `f10` | debug → step cycle |
| **step over subroutine** | `shift+f10` | debug → step over |
| **toggle breakpoint at pc** | `f9` | debug → toggle breakpoint |
| **speed up** | `+` / `=` / `up` / `]` | control panel slider |
| **slow down** | `-` / `down` / `[` | control panel slider |
| **fast speed step** | `pageup` / `pagedown` | — |
| **reset speed (600 hz)** | `0` / `backspace` | control panel → 600hz |
| **control panel** | `f2` | tools → control panel |
| **cycle color palette** | `tab` / `t` | color themes menu |
| **prev color palette** | `shift+tab` | — |
| **save state** | `f5` / `ctrl+s` | file → quick save |
| **load state** | `f6` / `ctrl+l` | file → quick load |
| **cycle save slot (1–5)** | `f7` | control panel |
| **help / shortcuts** | `h` / `f1` | help menu |
| **quit** | `esc` | file → quit |

## compatibility quirks

configure chip-8 vs super-chip compatibility via the debugger → **schip & quirks** tab:

| quirk | default | description |
|:---|:---|:---|
| **bit shift** | `true` (schip) | `8xy6`/`8xye` shifts vx in place; when off, copies vy→vx first (vip) |
| **vf reset** | `false` (schip) | when on, `8xy0/1/2/3` reset vf=0 after the operation (cosmac vip) |
| **load/store** | `true` (schip) | `fx55`/`fx65` leaves i unchanged; when off, i += x+1 (vip) |
| **jump** | `false` (chip-8) | when on, `bxnn` jumps to xnn+vx instead of nnn+v0 (schip) |
| **legacy scroll** | `false` | when on, lores scroll moves half-pixels (original hp-48 hardware) |

> test rom compatibility using the included `roms/4-flags.ch8` and `roms/5-quirks.ch8`.

## implementation details

### instruction set

full chip-8 + super-chip instruction set:
- **arithmetic**: add, sub, subn, and, or, xor, shr, shl — with configurable quirks
- **graphics**: xor sprite drawing with collision detection; 8×n standard + 16×16 schip mode
- **flow control**: jp, call/ret, se/sne conditional skips
- **memory**: ld/st registers, bcd conversion, rpl flags store/restore
- **timers**: delay (fx07/fx15) and sound (fx18) timer operations
- **input**: fx0a (blocking wait for key), ex9e/exa1 (skip on key state)
- **super-chip**: high/low resolution switch, hardware scrolling, large sprites, 10-byte fonts

### display

graphics rendered via sdl2 + imgui:
- chip-8: 64×32 pixels, each scaled 10× → 640×320 viewport
- schip: 128×64 pixels, each scaled 5× → 640×320 viewport (same physical size)
- xor-mode sprite drawing with vf collision flag
- 60 fps rendering with frame-timing cap

### audio

multi-waveform sdl2 audio synthesizer (44100 hz, 16-bit mono):
- **sine** — smooth warm tone
- **square / pulse** — classic 8-bit beep (configurable duty cycle 10%–90%)
- **sawtooth** — arcade buzz
- **triangle** — retro bass
- **noise** — 16-bit galois lfsr static

### savestate format

binary format (`c8st` magic, version 2):
- saves: pc, opcode, i, sp, timers, draw_flag, extended_mode, halted, v0–vf, stack, keypad, rpl flags, display buffer (128×64), full 64 kb ram

## resources

- [chip-8 technical reference](http://devernay.free.fr/hacks/chip8/c8tech10.htm)
- [super-chip specification](https://github.com/trapexit/chip-8_documentation)
- [chip-8 test suite roms](https://github.com/timendus/chip8-test-suite)
- [chip-8 roms archive](https://github.com/kripod/chip8-roms)
