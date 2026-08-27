import numpy as np
import torch

# Xiangqi board is 90 squares.
# King buckets: The General can only be in the 3x3 palace (9 squares). 
# If mirrored, we can reduce it to 6 buckets (files 3, 4, 5 mirrored -> 3, 4).
# We'll just use 9 buckets to keep it simple.
KING_BUCKETS = 9
# 14 piece types (7 White, 7 Black) * 90 squares
FEATURES = KING_BUCKETS * (14 * 90)

# 0-8 for the 9 palace squares.
# Assuming normalized king square is always in the bottom palace (0-89).
# Bottom palace: files 3,4,5. ranks 0,1,2.
# Squares: r*9+f. 
# Rank 0: 3, 4, 5
# Rank 1: 12, 13, 14
# Rank 2: 21, 22, 23
KING_BUCKET = np.zeros(90, dtype=np.int64)
KING_BUCKET[3] = 0; KING_BUCKET[4] = 1; KING_BUCKET[5] = 2;
KING_BUCKET[12] = 3; KING_BUCKET[13] = 4; KING_BUCKET[14] = 5;
KING_BUCKET[21] = 6; KING_BUCKET[22] = 7; KING_BUCKET[23] = 8;

# Record struct: 72 bytes
BF_DTYPE = np.dtype([
    ("sqs", "u1", 32),
    ("pcs", "u1", 32),
    ("score", "<i2"),
    ("result", "u1"),
    ("ksq", "u1"),
    ("opp_ksq", "u1"),
    ("pad", "u1", 3),
])
assert BF_DTYPE.itemsize == 72

def open_bf(path):
    return np.memmap(path, dtype=BF_DTYPE, mode="r")

def king_ctx(oriented_ksq):
    mir = ((oriented_ksq % 9) >= 5).astype(np.int64)
    # If mirrored, mirror the square across the center file (file 4).
    # file = sq % 9
    # mirrored_file = 8 - file
    # rank = sq // 9
    # mirrored_sq = rank * 9 + mirrored_file
    r = oriented_ksq // 9
    f = oriented_ksq % 9
    mirrored_sq = r * 9 + (8 - f)
    
    actual_sq = np.where(mir, mirrored_sq, oriented_ksq)
    bucket = KING_BUCKET[actual_sq]
    return bucket, mir

def decode_batch(recs):
    B = len(recs)
    sqs = recs["sqs"]
    pcs = recs["pcs"]
    
    # Valid pieces have sqs != 255
    valid_mask = sqs != 255
    rows, cols = np.nonzero(valid_mask)
    
    sq = sqs[rows, cols].astype(np.int64)
    pc = pcs[rows, cols].astype(np.int64)
    
    typ = pc & 7
    col = pc >> 3
    
    # stm_idx / opp_idx
    b_s, m_s = king_ctx(recs["ksq"].astype(np.int64))
    b_o, m_o = king_ctx(recs["opp_ksq"].astype(np.int64))
    b_s, m_s = b_s[rows], m_s[rows]
    b_o, m_o = b_o[rows], m_o[rows]
    
    # Mirroring a square: r*9 + (8 - f)
    # This is equivalent to sq + 8 - 2*(sq%9)
    def mirror_sq(s, m):
        f = s % 9
        mirrored = s + 8 - 2 * f
        return np.where(m, mirrored, s)
    
    stm_sq = mirror_sq(sq, m_s)
    # 14 piece types = col*7 + typ
    stm_pc = col * 7 + typ
    stm_feat = (14 * 90) * b_s + 90 * stm_pc + stm_sq
    
    # Opponent perspective: we must flip the rank (r -> 9-r) and also mirror if needed.
    # rank flip: sq_opp = (9 - r)*9 + f = 81 - r*9 + f = 81 - sq + 2*(sq%9)
    opp_base_sq = 81 - sq + 2 * (sq % 9)
    opp_sq = mirror_sq(opp_base_sq, m_o)
    opp_pc = (1 - col) * 7 + typ
    opp_feat = (14 * 90) * b_o + 90 * opp_pc + opp_sq

    stm_idx = np.full((B, 32), FEATURES, np.int64)
    opp_idx = np.full((B, 32), FEATURES, np.int64)
    
    # We need to map rows and cols back to 0..N per row
    counts = valid_mask.sum(axis=1)
    starts = np.concatenate(([0], np.cumsum(counts)[:-1])).astype(np.int64)
    ordinal = np.arange(len(rows)) - np.repeat(starts, counts)
    
    stm_idx[rows, ordinal] = stm_feat
    opp_idx[rows, ordinal] = opp_feat

    score = recs["score"].astype(np.float32)
    result = recs["result"].astype(np.float32) / 2.0
    obkt = (counts - 2) // 4
    return stm_idx, opp_idx, score, result, obkt

class Batches:
    def __init__(self, path, batch_size, device, start=0, stop=None):
        self.arr = open_bf(path)
        self.batch_size = batch_size
        self.device = device
        self.start = start
        self.stop = len(self.arr) if stop is None else stop

    def __len__(self):
        return (self.stop - self.start) // self.batch_size

    def __iter__(self):
        for lo in range(self.start, self.stop - self.batch_size + 1, self.batch_size):
            stm, opp, score, result, obkt = decode_batch(self.arr[lo:lo + self.batch_size])
            yield (torch.from_numpy(stm).to(self.device),
                   torch.from_numpy(opp).to(self.device),
                   torch.from_numpy(score).to(self.device),
                   torch.from_numpy(result).to(self.device),
                   torch.from_numpy(obkt).to(self.device))
