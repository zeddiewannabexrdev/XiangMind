#include <cstring>
#include "eval.h"
#include "position.h"
#include "tables.h"
#include "tt.h"
#include "uci.h"

int main(int argc, char **argv) {
  initialise_all_databases();
  zobrist::initialise_zobrist_keys();
  
  

  uci::loop(argc > 1 && std::strcmp(argv[1], "--debug") == 0);

  return 0;
}
