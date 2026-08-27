#pragma once
#include "position.h"
#include <mutex>
namespace uci {
    void loop(bool bench, Position& pos, std::mutex& pos_mutex);
}
