# Xiangqi Zeddie Engine

**Xiangqi Zeddie Engine** is an ultra-fast Xiangqi (Chinese Chess) engine written in modern C++26. It utilizes a custom 128-bit Bitboard representation and modern SIMD instructions to achieve extreme move generation speeds. 

The engine features a **built-in Raylib Graphical Viewer** for interactive play and a complete **PyTorch NNUE Toolchain** for training modern neural network evaluations from scratch.

## Features
- **128-bit Bitboard Architecture:** Custom dual-64-bit Bitboard struct mapping the 90 squares of the 9x10 Xiangqi board perfectly and efficiently.
- **Advanced Move Generation:** Pre-computed magic-like lookup tables for Chariots and Cannons. Applies fast bitwise logic to handle Xiangqi's complex asymmetric rules such as leg-blocking (Horses), eye-blocking (Elephants), and the Flying General rule.
- **Built-in Graphical Viewer:** An integrated, multi-threaded GUI built with Raylib. Renders the board state in real-time alongside a cheat sheet for easy terminal interaction.
- **NNUE Neural Network Training:** Complete end-to-end training pipeline. Generates 72-byte Record formats through self-play (Datagen), with a fully customized PyTorch trainer (11,340 features) capable of exporting `.nnue` weights.
- **UCCI / UCI Protocol:** Fully compatible with standard protocols, allowing seamless integration with professional GUIs like Pengfei Chess and Binghe2000.

---

## 🛠 Build Instructions

### Requirements
- **C++26 Compiler** (MinGW GCC 14+ on Windows)
- **CMake** (3.28+)
- **Python 3.10+** (Optional, for NNUE training and Tournament script)

### Building on Windows (MinGW64)
Ensure your MinGW `bin` directory is in your system PATH. Open Terminal / PowerShell and run:
```powershell
# Configure CMake (Max Optimization Mode)
cmake -S . -B build-xq -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build the engine (using 4 CPU threads)
cmake --build build-xq -j 4
```
Once successfully built, the executable will be generated at `build-xq/xiangqi-zeddieengine.exe`.

---

## 🚀 How to Play (3 Methods)

### Method 1: AI Tournament Mode (Demo)
This is the best way to see the AI in action. Run the included Python script to launch two AI instances that will automatically play against each other on the graphical board.
```powershell
python -u tournament.py
```

### Method 2: Play via Professional GUI (Recommended)
To play against the engine manually using your mouse:
1. Download a Xiangqi GUI like **Pengfei Chess** or **Binghe2000**.
2. Add a new Engine in the software and point it to the `xiangqi-zeddieengine.exe` file.
3. Start a new game and enjoy playing against your NNUE-powered AI!

### Method 3: Console Direct Interaction (For Developers)
For debugging and manual command input:
```powershell
.\xiangqi-zeddieengine.exe
```
**Basic Commands:**
- `ucci`: Initialize the engine.
- `position startpos`: Set up the initial board state.
- `position startpos moves h2e2`: Move a piece (e.g., Cannon from h2 to e2).
- `go depth 3`: Command the AI to search 3 plies deep and return the `bestmove`.
- `quit`: Exit.

---

## 🧠 Training the Neural Network (NNUE)

If you want to train a smarter AI, follow this pipeline:

**1. Generate Self-Play Data (Datagen):**
Command the engine to play thousands of games against itself to collect evaluation data (Records).
```powershell
.\xiangqi-zeddieengine.exe --datagen 100000 data_train.bf
```

**2. Train with PyTorch:**
```powershell
pip install torch numpy
python tools/nnue/train.py data_train.bf xiangqi-net.pt
```

**3. Export to C++ Engine format:**
Convert the PyTorch `.pt` checkpoint into the binary `.nnue` format used directly by the C++ engine.
```powershell
python tools/nnue/export.py xiangqi-net.pt xiangqi-net.nnue
```
