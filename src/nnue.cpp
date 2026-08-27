#include "nnue.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>

namespace nnue {
    constexpr int KING_BUCKETS = 9;
    constexpr int FEATURES = KING_BUCKETS * 14 * 90; // 11340
    constexpr int HL = 512;
    constexpr int OUT_BUCKETS = 8;
    constexpr int QA = 255;
    constexpr int QB = 64;
    constexpr int SCALE = 400;

    int16_t ft_w[FEATURES][HL];
    int16_t ft_b[HL];
    int16_t out_w[OUT_BUCKETS][HL * 2];
    int32_t out_b[OUT_BUCKETS];
    bool is_loaded = false;

    bool load(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        char header[32];
        f.read(header, 32);
        
        for (int i = 0; i < FEATURES; ++i) {
            for (int j = 0; j < HL; ++j) {
                f.read((char*)&ft_w[i][j], 2);
            }
        }
        for (int j = 0; j < HL; ++j) f.read((char*)&ft_b[j], 2);
        for (int i = 0; i < HL * 2; ++i) {
            for (int j = 0; j < OUT_BUCKETS; ++j) {
                f.read((char*)&out_w[j][i], 2);
            }
        }
        for (int j = 0; j < OUT_BUCKETS; ++j) f.read((char*)&out_b[j], 4);
        is_loaded = true;
        return true;
    }

    int get_bucket(int sq, bool& mirrored) {
        int f = sq % 9;
        int r = sq / 9;
        mirrored = (f >= 5);
        int mf = mirrored ? 8 - f : f;
        int msq = r * 9 + mf;
        if (msq == 3) return 0;
        if (msq == 4) return 1;
        if (msq == 5) return 2;
        if (msq == 12) return 3;
        if (msq == 13) return 4;
        if (msq == 14) return 5;
        if (msq == 21) return 6;
        if (msq == 22) return 7;
        if (msq == 23) return 8;
        return 0;
    }

    int evaluate(const Position& pos) {
        if (!is_loaded) return 0;

        int32_t acc_stm[HL];
        int32_t acc_opp[HL];
        for (int i = 0; i < HL; ++i) {
            acc_stm[i] = ft_b[i];
            acc_opp[i] = ft_b[i];
        }

        bool black = pos.stm() == BLACK;
        Square ksq = pos.king_sq(pos.stm());
        Square opp_ksq = pos.king_sq(~pos.stm());

        int k_stm = black ? (9 - rank_of(ksq))*9 + file_of(ksq) : ksq;
        int k_opp = !black ? (9 - rank_of(opp_ksq))*9 + file_of(opp_ksq) : opp_ksq;

        bool mir_stm, mir_opp;
        int b_stm = get_bucket(k_stm, mir_stm);
        int b_opp = get_bucket(k_opp, mir_opp);

        int count = 0;
        for (int sq = 0; sq < 90; ++sq) {
            Square s = Square(sq);
            Piece pc = pos.piece_on(s);
            if (pc == NO_PIECE) continue;

            int type = type_of(pc);
            int col = (color_of(pc) == pos.stm()) ? 0 : 1;

            int s_stm = black ? (9 - rank_of(s))*9 + file_of(s) : sq;
            int s_opp = !black ? (9 - rank_of(s))*9 + file_of(s) : sq;

            int sq_stm = mir_stm ? s_stm + 8 - 2*(s_stm%9) : s_stm;
            int sq_opp = mir_opp ? s_opp + 8 - 2*(s_opp%9) : s_opp;

            int pc_stm = col * 7 + type;
            int pc_opp = (1 - col) * 7 + type;

            int feat_stm = (14 * 90) * b_stm + 90 * pc_stm + sq_stm;
            int feat_opp = (14 * 90) * b_opp + 90 * pc_opp + sq_opp;

            for (int i = 0; i < HL; ++i) {
                acc_stm[i] += ft_w[feat_stm][i];
                acc_opp[i] += ft_w[feat_opp][i];
            }
            count++;
        }

        int32_t out = out_b[0] * QA; // assuming obkt=0 for now
        for (int i = 0; i < HL; ++i) {
            int32_t c_stm = std::max(0, std::min((int)acc_stm[i], QA));
            out += (c_stm * c_stm) * out_w[0][i];
            
            int32_t c_opp = std::max(0, std::min((int)acc_opp[i], QA));
            out += (c_opp * c_opp) * out_w[0][HL + i];
        }

        return (out * SCALE) / (QA * QA * QB);
    }
}
