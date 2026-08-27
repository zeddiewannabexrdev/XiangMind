#!/usr/bin/env python3
import argparse
import struct
import sys
import numpy as np

KING_BUCKETS = 9
FEATURES = KING_BUCKETS * (14 * 90)  # 11340
HL = 512
OUT_BUCKETS = 8
QA, QB, SCALE = 255, 64, 400
CLIP = 1.98

def write_net(path: str, ft_w: np.ndarray, ft_b: np.ndarray, out_w: np.ndarray, out_b: np.ndarray) -> None:
    assert ft_w.shape == (FEATURES, HL) and ft_b.shape == (HL,)
    assert out_w.shape == (OUT_BUCKETS, 2 * HL) and out_b.shape == (OUT_BUCKETS,)

    ft_w_q: np.ndarray = np.round(ft_w * QA).astype(np.int64)
    ft_b_q: np.ndarray = np.round(ft_b * QA).astype(np.int64)
    out_w_q: np.ndarray = np.round(out_w * QB).astype(np.int64)
    out_b_q: np.ndarray = np.round(out_b.astype(np.float64) * QA * QB).astype(np.int64)

    assert np.abs(out_w_q).max() <= 127, f"|out_w_q| max {np.abs(out_w_q).max()} > 127 - clip violated"
    assert np.abs(ft_w_q).max() <= round(CLIP * QA) + 1, "ft weight clip violated"
    assert np.abs(ft_b_q).max() < 32768 and np.abs(ft_w_q).max() < 32768
    assert np.abs(out_b_q).max() < 2**31

    header: bytes = struct.pack("<4sIIIIHHHB5x", b"AKNN", 2, FEATURES, HL, OUT_BUCKETS, QA, QB, SCALE, 1)
    assert len(header) == 32
    with open(path, "wb") as f:
        f.write(header)
        f.write(ft_w_q.astype("<i2").tobytes())
        f.write(ft_b_q.astype("<i2").tobytes())
        f.write(out_w_q.astype("<i2").tobytes())
        f.write(out_b_q.astype("<i4").tobytes())
    size = 32 + 2 * (FEATURES * HL + HL + OUT_BUCKETS * 2 * HL) + 4 * OUT_BUCKETS
    print(f"wrote {path} ({size} bytes)")

def random_net(seed: int) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    rng = np.random.default_rng(seed)
    ft_w = rng.uniform(-0.05, 0.05, (FEATURES, HL)).astype(np.float32)
    ft_b = rng.uniform(0.0, 0.1, HL).astype(np.float32)
    out_w = rng.uniform(-0.5, 0.5, (OUT_BUCKETS, 2 * HL)).astype(np.float32)
    out_b = np.zeros(OUT_BUCKETS, np.float32)
    return ft_w, ft_b, out_w, out_b

def from_checkpoint(path: str) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    import torch
    sd = torch.load(path, map_location="cpu", weights_only=False)
    if "model" in sd:
        sd = sd["model"]
    ft_w = sd["ft.weight"].numpy()[:FEATURES]
    assert np.abs(sd["ft.weight"].numpy()[FEATURES]).max() == 0, "padding row nonzero"
    ft_b = sd["ft_bias"].numpy()
    out_w = sd["out.weight"].numpy()
    out_b = sd["out.bias"].numpy()
    assert np.abs(ft_w).max() <= CLIP + 1e-6 and np.abs(out_w).max() <= CLIP + 1e-6, "checkpoint not clipped"
    return ft_w.astype(np.float32), ft_b.astype(np.float32), out_w.astype(np.float32), out_b.astype(np.float32)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?", help="checkpoint .pt (omit with --random)")
    ap.add_argument("out", help="output .nnue path")
    ap.add_argument("--random", action="store_true", help="emit a random test net")
    ap.add_argument("--seed", type=int, default=42)
    args = ap.parse_args()

    if args.random:
        parts = random_net(args.seed)
    else:
        if not args.src:
            ap.error("checkpoint path required without --random")
        parts = from_checkpoint(args.src)
    write_net(args.out, *parts)
    return 0

if __name__ == "__main__":
    sys.exit(main())
