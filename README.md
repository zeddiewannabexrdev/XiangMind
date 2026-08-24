# Xiangqi Zeddie Engine

**Xiangqi Zeddie Engine** is an ultra-fast Xiangqi (Chinese Chess) engine written in C++26. Converted and heavily re-architected from the Askaig chess engine framework, it utilizes a custom 128-bit Bitboard representation and modern SIMD instructions (AVX2/NEON) to achieve extreme move generation speeds.

## Features
- **128-bit Bitboard Architecture:** Designed a custom dual-64-bit `Bitboard` struct to cover the entire 9x10 Xiangqi board (90 squares).
- **Advanced Move Generation:** Uses pre-computed rank and file lookup tables for Chariots and Cannons. Applies fast bitwise logic to perfectly handle Xiangqi's asymmetric leg-blocking (Horses) and eye-blocking (Elephants) rules.
- **Flying General Rule:** Instantaneous detection of exposed generals via bitboard geometric masks.
- **Search & Evaluation:** Negamax Alpha-Beta search combined with a hand-crafted evaluation function (incorporating material values and positional bonuses, e.g., crossed-river soldiers).
- **UCCI / UCI Protocol:** Fully compatible with modern Chinese Chess GUIs such as Pengfei Chess, Binghe2000, and PyChess.org.

## Build Instructions

### Requirements
- **C++26 Compiler** (GCC 14+ or Clang 18+)
- **CMake** (3.28+)
- A 64-bit CPU with AVX2 (Windows/Linux) or NEON (macOS/ARM) support.

*Note: MSVC (Visual Studio) is currently not supported due to the engine's reliance on GNU extensions and modern compiler intrinsics.*

### Building on Windows (MinGW64)
```bash
# Configure CMake
cmake -S . -B build-xq -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="C:/Program Files/mingw64/bin/g++.exe"

# Build the engine (using 4 CPU threads)
cmake --build build-xq -j 4
```

## How to Test & Play

Once successfully built, the executable will be located at `build-xq/xiangqi-zeddieengine.exe`.

**1. Play directly in the Terminal:**
You can launch the `.exe` directly and interact with it using these commands:
- `ucci`: Initialize the connection protocol.
- `d`: Print the current 9x10 board to the terminal.
- `go`: Command the engine to think and return the best move.

**2. Play using a GUI (Recommended):**
For the best experience, hook the engine up to a graphical Xiangqi interface:
- **Pengfei Chess / Binghe2000 (Windows):** Add the `.exe` to the "Load Engine" menu and set the protocol to UCI or UCCI.
- **PyChess.org (Web):** Use the "PyChess Local Engine Client" to bridge your local engine to the PyChess web interface and play online seamlessly.

## Acknowledgements
This project is built upon the foundational framework of the Askaig chess engine. The core engine components (Board representation, Move generation, Evaluation, and Search) were completely re-architected to accommodate the complex rules of Xiangqi.
