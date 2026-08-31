#include <cstring>
#include <thread>
#include <mutex>
#include <string>
#include <iostream>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "eval.h"
#include "nnue.h"
#include "position.h"
#include "tables.h"
#include "tt.h"
#include "uci.h"
#include "gui_viewer.h"
#include "datagen.h"

int main(int argc, char **argv) {
  initialise_all_databases();
  zobrist::initialise_zobrist_keys();

  // Automatically load NNUE model if present
  if (!nnue::load("xiangqi-net.nnue")) {
      nnue::load("../xiangqi-net.nnue");
  }
  
  if (argc > 1 && std::strcmp(argv[1], "--datagen") == 0) {
      uint64_t count = argc > 2 ? std::stoull(argv[2]) : 10000;
      std::string out = argc > 3 ? argv[3] : "data.bin";
      datagen::run(count, out, 5000, 0);
      return 0;
  }

  Position shared_pos;
  std::mutex pos_mutex;

  bool bench = false;
  bool use_gui = true;

  // Auto-detect if stdin is a pipe / redirected by an external GUI or bot script
#ifdef _WIN32
  if (!_isatty(_fileno(stdin))) {
      use_gui = false;
  }
#else
  if (!isatty(fileno(stdin))) {
      use_gui = false;
  }
#endif

  for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "--debug") == 0) bench = true;
      if (std::strcmp(argv[i], "--uci") == 0 || std::strcmp(argv[i], "--nogui") == 0) use_gui = false;
      if (std::strcmp(argv[i], "--gui") == 0) use_gui = true;
  }

  if (use_gui) {
      std::thread uci_thread(uci::loop, bench, std::ref(shared_pos), std::ref(pos_mutex));
      GuiViewer::run(shared_pos, pos_mutex);
      uci_thread.join();
  } else {
      uci::loop(bench, shared_pos, pos_mutex);
  }
  return 0;
}
