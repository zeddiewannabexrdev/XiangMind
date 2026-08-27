#include "smp.h"
#include <cstring>

namespace search {

  SMP g_smp;

  SMP::SMP() {}
  SMP::~SMP() { stop(); }

  void SMP::resize(int num_threads) {
    stop();
    tds.clear();
    for (int i = 0; i < num_threads; ++i) {
      tds.push_back(std::make_unique<ThreadData>());
      tds.back()->id = i;
    }
  }

  void SMP::start(const Position &pos) {
    stop();
    run = true;
    idle = tds.size();
    std::memcpy(&root, &pos, sizeof(Position));
    for (size_t i = 0; i < tds.size(); ++i) {
      threads.emplace_back(&SMP::worker, this, i);
    }
  }

  void SMP::stop() {
    exit = true;
    cv.notify_all();
    for (auto &th : threads) {
      if (th.joinable()) th.join();
    }
    threads.clear();
    exit = false;
    run = false;
  }

  void SMP::wait() {
    std::unique_lock<std::mutex> lk(mtx);
    cv_idle.wait(lk, [this] { return idle == 0; });
  }

  void SMP::worker(int id) {
    ThreadData &t = *tds[id];
    while (true) {
      {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this] { return exit || run; });
        if (exit) break;
      }
      
      // Dummy work for now
      // smp_worker_iterate(t, root, 256, id);

      {
        std::lock_guard<std::mutex> lk(mtx);
        idle--;
        if (idle == 0) cv_idle.notify_all();
      }
      
      // Wait for next start
      {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [this] { return exit || !run; });
        if (exit) break;
      }
    }
  }

} // namespace search
