#include "search.h"
#include "eval.h"
#include "nnue.h"
#include "position.h"

namespace search {

void init() {}
void clear() {}


const int MATE_SCORE = 29000;

int negamax(Position& pos, int depth, int alpha, int beta) {
    if (depth <= 0) {
        return eval::evaluate(pos) + nnue::evaluate(pos);
    }
    
    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
    
    if (list == end) {
        // In Xiangqi, stalemate is a loss, not a draw. So no legal moves = lose.
        return -MATE_SCORE + (100 - depth); // Prefer faster mates
    }
    
    if (pos.is_draw()) return 0;
    
    int best = -INF;
    for (Move* m = list; m != end; ++m) {
        if (pos.stm() == WHITE) pos.play<WHITE>(*m); else pos.play<BLACK>(*m);
        
        int score = -negamax(pos, depth - 1, -beta, -alpha);
        
        if (pos.stm() == WHITE) pos.undo<BLACK>(*m); else pos.undo<WHITE>(*m); // play() toggles stm, so we use opposite
        
        if (score > best) best = score;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break; // Alpha-beta pruning
    }
    return best;
}

Result think(Position& pos, int threads, const InfoFn& info, int64_t tm, int64_t inc) {
    Result res;
    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
    
    if (list == end) return res;
    
    Move best_move = list[0];
    int best_score = -INF;
    int depth = 3; // Nâng lên depth 3 để AI đánh thông minh hơn một chút, mất thời gian suy nghĩ hơn
    
    for (Move* m = list; m != end; ++m) {
        
        int score = -negamax(pos, depth - 1, -INF, INF);
        
        if (pos.stm() == WHITE) pos.undo<BLACK>(*m); else pos.undo<WHITE>(*m);
        
        if (score > best_score) {
            best_score = score;
            best_move = *m;
        }
    }
    
    res.best = best_move;
    return res;
}

} // namespace search
