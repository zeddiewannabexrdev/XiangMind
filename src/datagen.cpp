#include "datagen.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "position.h"
#include "search.h"
#include "smp.h"
#include "tt.h"
#include "types.h"

class PRNG {
  uint64_t s;
public:
  PRNG(uint64_t seed) : s(seed) {}
  template<typename T> T rand() {
    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return static_cast<T>(s * 2685821657736338717LL);
  }
};

namespace {

  using search::do_move;
  using search::is_quiet;
  using search::stm_in_check;
  using Clock = std::chrono::steady_clock;

  constexpr int OPENING_GATE_CP = 400; 
  constexpr int SCORE_CAP_CP    = 1000; 
  constexpr int WIN_ADJ_PLIES   = 4; 
  constexpr int DRAW_ADJ_PLIES  = 8; 
  constexpr int MAX_PLIES       = 400;

  struct Record {
    uint64_t occ; // Warning: 64 bits only! Not enough for 90 squares. Kept for struct size compat.
    uint8_t  pcs[16];
    int16_t  score; 
    uint8_t  result; 
    uint8_t  ksq; 
    uint8_t  opp_ksq; 
    uint8_t  pad[3];
  };
  static_assert(sizeof(Record) == 32);

  Record encode(const Position &pos, int score) {
    Record     r{};
    const bool black = pos.stm() == BLACK;
    r.score          = int16_t(score);

    int idx = 0;
    for (int sq = 0; sq < int(NSQUARES); ++sq) {
      Square s = Square(sq);
      const Square orig = black ? create_square(file_of(s), Rank(9 - rank_of(s))) : s;
      const Piece  pc   = pos.piece_on(orig);
      if (pc == NO_PIECE)
        continue;
      const int type = type_of(pc);
      const int col  = (color_of(pc) == pos.stm()) ? 0 : 1; 
      if (sq < 64) r.occ |= 1ull << sq; // Prevent overflow for sq >= 64
      if (idx < 32) r.pcs[idx / 2] |= uint8_t(((col << 3) | type) << (4 * (idx % 2)));
      if (type == GENERAL) {
        if (col == 0)
          r.ksq = uint8_t(sq);
        else {
          Square opp_s = black ? create_square(file_of(s), Rank(9 - rank_of(s))) : s;
          r.opp_ksq = uint8_t(opp_s);
        }
      }
      ++idx;
    }
    return r;
  }

  size_t count_legals(Position &pos) {
    Move buf[218];
    return pos.stm() == WHITE ? size_t(pos.generate_legals<WHITE, false>(buf) - buf)
                               : size_t(pos.generate_legals<BLACK, false>(buf) - buf);
  }

  Move random_legal(Position &pos, PRNG &rng) {
    Move         buf[218];
    const size_t n = pos.stm() == WHITE ? size_t(pos.generate_legals<WHITE, false>(buf) - buf)
                                         : size_t(pos.generate_legals<BLACK, false>(buf) - buf);
    return n == 0 ? Move() : buf[rng.rand<uint64_t>() % n];
  }

  int search_once(Position &pos, Move *best) {
    const search::Result r = search::think(pos, search::MAX_PLY - 1, nullptr, 0, 0);
    if (best)
      *best = r.best;
    return r.score;
  }

  std::string fmt_hms(double s) {
    if (!(s >= 0) || s > 3.15e9)
      return "?";
    const uint64_t t = uint64_t(s);
    char           buf[32];
    if (t >= 3600)
      std::snprintf(buf, sizeof buf, "%lluh%02llum", (unsigned long long) (t / 3600),
                    (unsigned long long) ((t % 3600) / 60));
    else if (t >= 60)
      std::snprintf(buf, sizeof buf, "%llum%02llus", (unsigned long long) (t / 60), (unsigned long long) (t % 60));
    else
      std::snprintf(buf, sizeof buf, "%llus", (unsigned long long) t);
    return buf;
  }

} // namespace

void datagen::run(uint64_t count, const std::string &out, uint64_t nodes, uint64_t seed) {
  std::FILE *f = std::fopen(out.c_str(), "ab");
  if (!f) {
    std::printf("datagen: cannot open %s\n", out.c_str());
    return;
  }
  tt::resize(16);
  PRNG rng(seed ? seed : 0x9E3779B97F4A7C15ull);

  struct Pending {
    Record rec;
    bool   black; 
  };
  std::vector<Pending> game;
  game.reserve(MAX_PLIES);

  uint64_t   written = 0, games = 0;
  const auto t0 = Clock::now();

  while (written < count) {
    tt::clear();
    game.clear();

    Position pos;
    Position::set(DEFAULT_FEN, pos);

    const int plies = 8 + int(games & 1);
    bool      ok    = true;
    for (int i = 0; i < plies; ++i) {
      const Move m = random_legal(pos, rng);
      if (m.to_from() == 0) {
        ok = false;
        break;
      }
      do_move(pos, m);
    }
    ++games;
    if (!ok || count_legals(pos) == 0)
      continue;
    if (std::abs(search_once(pos, nullptr)) > OPENING_GATE_CP)
      continue;

    int wr         = -1; 
    int win_plies = 0, draw_plies = 0;
    for (int ply = 0; ply < MAX_PLIES; ++ply) {
      if (count_legals(pos) == 0) {
        wr = stm_in_check(pos) ? (pos.stm() == WHITE ? 0 : 2) : 1; 
        break;
      }
      if (pos.is_draw()) {
        wr = 1;
        break;
      }

      Move      best;
      const int score = search_once(pos, &best);
      if (best.to_from() == 0)
        break; 

      if (!stm_in_check(pos) && is_quiet(best) && std::abs(score) < SCORE_CAP_CP)
        game.push_back({encode(pos, score), pos.stm() == BLACK});

      win_plies  = std::abs(score) >= SCORE_CAP_CP ? win_plies + 1 : 0;
      draw_plies = std::abs(score) <= 10 && ply >= 80 ? draw_plies + 1 : 0;
      if (win_plies >= WIN_ADJ_PLIES) {
        const bool stm_wins = score > 0;
        wr                  = (pos.stm() == WHITE) == stm_wins ? 2 : 0;
        break;
      }
      if (draw_plies >= DRAW_ADJ_PLIES) {
        wr = 1;
        break;
      }

      do_move(pos, best);
    }
    if (wr < 0)
      wr = 1; 

    for (const Pending &p: game) {
      Record rec = p.rec;
      rec.result = uint8_t(p.black ? 2 - wr : wr);
      std::fwrite(&rec, sizeof(rec), 1, f);
      if (++written % 10000 == 0) {
        const double el   = std::chrono::duration<double>(Clock::now() - t0).count();
        const double rate = el > 0 ? double(written) / el : 0;
        const double eta  = rate > 0 ? double(count - written) / rate : -1;
        std::printf("datagen: %llu/%llu (%.1f%%) | %llu games, %.1f rec/game | %.0f pos/s | %lluMB | ETA %s\n",
                    (unsigned long long) written, (unsigned long long) count, 100.0 * double(written) / double(count),
                    (unsigned long long) games, games ? double(written) / double(games) : 0.0, rate,
                    (unsigned long long) (written * sizeof(Record) / (1024 * 1024)), fmt_hms(eta).c_str());
        std::fflush(stdout);
      }
      if (written >= count)
        break;
    }
    std::fflush(f);
  }

  const double el = std::chrono::duration<double>(Clock::now() - t0).count();
  std::printf("datagen done: %llu positions, %llu games (%.1f rec/game) in %s (%.0f pos/s) | %lluMB -> %s\n",
              (unsigned long long) written, (unsigned long long) games, games ? double(written) / double(games) : 0.0,
              fmt_hms(el).c_str(), el > 0 ? double(written) / el : 0.0,
              (unsigned long long) (written * sizeof(Record) / (1024 * 1024)), out.c_str());
  std::fflush(stdout);
  std::fclose(f);
}
