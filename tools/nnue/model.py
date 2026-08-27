import torch
import torch.nn as nn

KING_BUCKETS = 9
FEATURES = KING_BUCKETS * (14 * 90)  # 11340
HL = 512
OUT_BUCKETS = 8
SCALE = 400
CLIP = 1.98


class Net(nn.Module):
    def __init__(self, hl=HL):
        super().__init__()
        self.hl = hl
        self.ft = nn.Embedding(FEATURES + 1, hl, padding_idx=FEATURES)
        self.ft_bias = nn.Parameter(torch.zeros(hl))
        self.out = nn.Linear(2 * hl, OUT_BUCKETS)
        with torch.no_grad():
            self.ft.weight.normal_(0.0, 0.05)
            self.ft.weight[FEATURES].zero_()  # the padding row stays zero

    def forward(self, stm_idx, opp_idx, obkt):
        acc_stm = self.ft(stm_idx).sum(dim=1) + self.ft_bias
        acc_opp = self.ft(opp_idx).sum(dim=1) + self.ft_bias
        h = torch.cat([acc_stm, acc_opp], dim=1).clamp(0, 1).pow(2)  # SCReLU
        return self.out(h).gather(1, obkt.view(-1, 1))

    @torch.no_grad()
    def clip(self):
        self.ft.weight.clamp_(-CLIP, CLIP)
        self.ft_bias.clamp_(-CLIP, CLIP)
        self.out.weight.clamp_(-CLIP, CLIP)
        self.ft.weight[FEATURES].zero_()
