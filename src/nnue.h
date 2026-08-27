#pragma once
#include "position.h"

namespace nnue {
    bool load(const char* path);
    int evaluate(const Position& pos);
}
