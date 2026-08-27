#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "history.h"
#include "position.h"
#include "search.h"
#include "types.h"

namespace search {


  struct ThreadData {
    int                     id   = 0;
    int                     seld = 0;
    int                     nodes = 0;
    ContTable              *cont = nullptr;
    Histories               hist;
    Position                pos;
  };

  class SMP {
  private:
    std::vector<std::unique_ptr<ThreadData>> tds;
    std::vector<std::thread>                 threads;
    std::mutex                               mtx;
    std::condition_variable                  cv;
    std::condition_variable                  cv_idle;
    Position                                 root;
    std::atomic<bool>                        exit = false;
    std::atomic<bool>                        run  = false;
    std::atomic<int>                         idle = 0;

    void worker(int id);

  public:
    SMP();
    ~SMP();
    void        resize(int threads);
    void        start(const Position &pos);
    void        stop();
    void        wait();
    ThreadData *thread(int id) { return tds[id].get(); }
    int         thread_count() const { return static_cast<int>(tds.size()); }
  };

  extern SMP g_smp;

  inline void do_move(Position &p, Move m) {
    if (p.stm() == WHITE)
      p.play<WHITE>(m);
    else
      p.play<BLACK>(m);
  }

  inline void undo_move(Position &p, Move m) {
    if (p.stm() == WHITE)
      p.undo<BLACK>(m); // stm was already flipped by play!
    else
      p.undo<WHITE>(m);
  }

  inline bool is_quiet(Move m) {
    const MoveFlags f = m.flags();
    return f == QUIET;
  }

  inline bool stm_in_check(const Position &p) {
    return p.stm() == WHITE ? p.in_check<WHITE>() : p.in_check<BLACK>();
  }

  inline uint64_t tt_key(const Position &p) {
    return p.get_hash();
  }

} // namespace search
