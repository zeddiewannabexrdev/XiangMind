# Xiangqi Zeddie Engine

**Xiangqi Zeddie Engine** is an ultra-fast Xiangqi (Chinese Chess) engine written in C++26. Converted and heavily re-architected from the Askaig chess engine framework, it utilizes a custom 128-bit Bitboard representation and modern SIMD instructions to achieve extreme move generation speeds. 

The engine now features a **built-in Raylib Graphical Viewer** for interactive play and a complete **PyTorch NNUE Toolchain** for training modern neural network evaluations.

## Features
- **128-bit Bitboard Architecture:** Custom dual-64-bit Bitboard struct mapping the 90 squares of the 9x10 Xiangqi board.
- **Advanced Move Generation:** Pre-computed magic-like lookup tables for Chariots and Cannons. Applies fast bitwise logic to perfectly handle Xiangqi's asymmetric leg-blocking (Horses) and eye-blocking (Elephants) rules.
- **Built-in Graphical Viewer:** An integrated, multi-threaded GUI built with Raylib. Renders the board state in real-time alongside a cheat sheet for easy terminal interaction.
- **NNUE Neural Network Training:** Completely upgraded training pipeline. Generates 72-byte Record formats through self-play (Datagen), with a fully customized PyTorch trainer (11,340 features) capable of exporting .nnue weights.
- **UCCI / UCI Protocol:** Fully compatible with standard protocols.

## Build Instructions

### Requirements
- **C++26 Compiler** (MinGW GCC 14+ on Windows)
- **CMake** (3.28+)
- **Python 3.10+** (Optional, for NNUE training)

### Building on Windows (MinGW64)
Ensure your MinGW in directory is in your system PATH, or specify it directly.
`ash
# Configure CMake
cmake -S . -B build-xq -G "MinGW Makefiles"

# Build the engine (using 4 CPU threads)
cmake --build build-xq -j 4
`
This process will automatically download and link Raylib 5.5.

## How to Play

Once successfully built, run the executable:
`ash
.\build-xq\xiangqi-zeddieengine.exe
`
This will open **two windows**:
1. **The Raylib GUI Viewer:** Displays the board visually along with coordinate labels and a cheat sheet.
2. **The Terminal:** Where you type commands to interact with the engine.

**Basic Commands:**
- position startpos moves h2e2: Move a piece (e.g., Cannon from h2 to e2).
- go: Command the AI to think and return the best move.
- d: Print the text-based board and FEN to the terminal.
- quit: Exit the game.

## Training the NNUE AI

To train your own neural network for the engine, follow these steps:

**1. Generate Self-Play Data (Datagen):**
Run the engine in datagen mode to simulate millions of games and record evaluations.
`ash
.\build-xq\xiangqi-zeddieengine.exe --datagen 1000000 data.bin
`

**2. Train the Model (PyTorch):**
Shuffle the generated data, then train the neural network.
`ash
# Install requirements
pip install torch numpy

# Shuffle data for better training
python tools/nnue/shuffle_bin.py data.bin shuffled.bin

# Train the network (Outputs xiangqi.pt)
python tools/nnue/train.py shuffled.bin xiangqi.pt
`

**3. Export to C++:**
Convert the PyTorch .pt checkpoint into the binary .nnue format used by the engine.
`ash
python tools/nnue/export.py xiangqi.pt xiangqi.nnue
`

## Acknowledgements
This project is built upon the foundational framework of the Askaig chess engine. The core engine components (Board representation, Move generation, Evaluation, and Search) were completely re-architected to accommodate the complex rules and dimensions of Xiangqi.
