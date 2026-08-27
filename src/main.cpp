#include <cstring>
#include <thread>
#include <mutex>
#include <string>
#include "eval.h"
#include "position.h"
#include "tables.h"
#include "tt.h"
#include "uci.h"
#include "gui_viewer.h"
#include "datagen.h"

int main(int argc, char **argv) {
  initialise_all_databases();
  zobrist::initialise_zobrist_keys();
  
  if (argc > 1 && std::strcmp(argv[1], "--datagen") == 0) {
      uint64_t count = argc > 2 ? std::stoull(argv[2]) : 10000;
      std::string out = argc > 3 ? argv[3] : "data.bin";
      datagen::run(count, out, 5000, 0);
      return 0;
  }

  Position shared_pos;
  std::mutex pos_mutex;

  bool bench = argc > 1 && std::strcmp(argv[1], "--debug") == 0;
  std::thread uci_thread(uci::loop, bench, std::ref(shared_pos), std::ref(pos_mutex));

  GuiViewer::run(shared_pos, pos_mutex);

  uci_thread.join();
  return 0;
}
