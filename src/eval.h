#pragma once
#include "position.h"

namespace eval {

constexpr int PIECE_VALUE[NPIECE_TYPES] = {
    100, // SOLDIER
    200, // ADVISOR
    200, // ELEPHANT
    400, // HORSE
    450, // CANNON
    900, // CHARIOT
    0    // GENERAL (infinite)
};

int evaluate(const Position& pos);

} // namespace eval
