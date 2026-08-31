#include "search.h"
#include "eval.h"
#include "nnue.h"
#include "position.h"
#include "uci.h"
#include <chrono>
#include <iostream>
#include <atomic>
#include <algorithm>

namespace search {

std::atomic<bool> stop_flag{false};

void init() {}
void clear() {}
void request_stop() { stop_flag.store(true, std::memory_order_relaxed); }
void clear_stop() { stop_flag.store(false, std::memory_order_relaxed); }
void new_game() {}
void set_threads(int) {}
void set_contempt(int) {}
void set_node_limit(uint64_t) {}

const int MATE_SCORE = 29000;

// Order moves: captures first, sorted by victim piece value
void score_and_sort_moves(const Position& pos, Move* list, Move* end, Move tt_best = Move()) {
    int scores[256];
    int n = static_cast<int>(end - list);
    for (int i = 0; i < n; ++i) {
        if (list[i] == tt_best) {
            scores[i] = 1000000;
        } else if (list[i].is_capture()) {
            Piece victim = pos.piece_on(list[i].to());
            int victim_val = (victim != NO_PIECE) ? eval::PIECE_VALUE[type_of(victim)] : 0;
            scores[i] = 10000 + victim_val;
        } else {
            scores[i] = 0;
        }
    }
    // Selection sort
    for (int i = 0; i < n - 1; ++i) {
        int best_idx = i;
        for (int j = i + 1; j < n; ++j) {
            if (scores[j] > scores[best_idx]) {
                best_idx = j;
            }
        }
        if (best_idx != i) {
            std::swap(list[i], list[best_idx]);
            std::swap(scores[i], scores[best_idx]);
        }
    }
}

int quiescence(Position& pos, int alpha, int beta, uint64_t& nodes) {
    nodes++;
    if (pos.is_draw()) return 0;

    int stand_pat = eval::evaluate(pos) + nnue::evaluate(pos);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, true>(list) : pos.generate_legals<BLACK, true>(list);
    score_and_sort_moves(pos, list, end);

    for (Move* m = list; m != end; ++m) {
        if (pos.stm() == WHITE) pos.play<WHITE>(*m); else pos.play<BLACK>(*m);
        int score = -quiescence(pos, -beta, -alpha, nodes);
        if (pos.stm() == WHITE) pos.undo<BLACK>(*m); else pos.undo<WHITE>(*m);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int negamax(Position& pos, int depth, int alpha, int beta, uint64_t& nodes, bool& aborted) {
    if (stop_flag.load(std::memory_order_relaxed)) {
        aborted = true;
        return 0;
    }

    nodes++;
    if (depth <= 0) {
        return quiescence(pos, alpha, beta, nodes);
    }
    
    if (pos.is_draw()) return 0;

    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
    
    if (list == end) {
        // Stalemate / Checkmate is a loss in Xiangqi
        return -MATE_SCORE + (100 - depth);
    }
    
    score_and_sort_moves(pos, list, end);

    int best = -INF;
    for (Move* m = list; m != end; ++m) {
        if (pos.stm() == WHITE) pos.play<WHITE>(*m); else pos.play<BLACK>(*m);
        
        int score = -negamax(pos, depth - 1, -beta, -alpha, nodes, aborted);
        
        if (pos.stm() == WHITE) pos.undo<BLACK>(*m); else pos.undo<WHITE>(*m);

        if (aborted) return 0;
        
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break; // Alpha-beta cutoff
    }
    return best;
}

Result think(Position& pos, int max_depth, const InfoFn& info, int64_t soft_ms, int64_t hard_ms) {
    clear_stop();
    Result res;
    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
    
    if (list == end) return res;
    
    res.best = list[0];
    res.score = 0;
    res.nodes = 0;

    auto start_time = std::chrono::steady_clock::now();

    int target_depth = (max_depth > 0) ? max_depth : 64;
    if (max_depth <= 0 && soft_ms <= 0 && hard_ms <= 0) {
        target_depth = 5;
    }

    Move prev_best = list[0];

    for (int d = 1; d <= target_depth; ++d) {
        score_and_sort_moves(pos, list, end, prev_best);

        Move best_at_d = list[0];
        int best_score_at_d = -INF;
        int alpha = -INF;
        int beta = INF;
        bool aborted = false;

        for (Move* m = list; m != end; ++m) {
            if (pos.stm() == WHITE) pos.play<WHITE>(*m); else pos.play<BLACK>(*m);
            
            int score = -negamax(pos, d - 1, -beta, -alpha, res.nodes, aborted);
            
            if (pos.stm() == WHITE) pos.undo<BLACK>(*m); else pos.undo<WHITE>(*m);

            if (aborted || stop_flag.load(std::memory_order_relaxed)) {
                aborted = true;
                break;
            }

            if (score > best_score_at_d) {
                best_score_at_d = score;
                best_at_d = *m;
            }
            if (score > alpha) {
                alpha = score;
            }

            // Check hard time limit after each root move if hard_ms > 0
            if (hard_ms > 0 && d > 1) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
                if (elapsed >= hard_ms) {
                    aborted = true;
                    break;
                }
            }
        }

        if (aborted && d > 1) {
            break;
        }

        res.best = best_at_d;
        res.score = best_score_at_d;
        prev_best = best_at_d;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        uint64_t nps = elapsed > 0 ? (res.nodes * 1000) / elapsed : 0;
        
        std::cout << "info depth " << d 
                  << " score cp " << res.score 
                  << " time " << elapsed 
                  << " nodes " << res.nodes 
                  << " nps " << nps 
                  << " pv " << uci::move_to_string(res.best, uci::use_1_indexed) << std::endl;

        if (info) {
            info(d, res, res.nodes, elapsed);
        }

        // Soft time limit check
        if (soft_ms > 0 && d >= 1) {
            if (elapsed >= soft_ms) {
                break;
            }
        }
    }

    return res;
}

} // namespace search
