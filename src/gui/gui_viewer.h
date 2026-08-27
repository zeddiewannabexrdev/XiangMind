#pragma once
#include "position.h"
#include <mutex>

class GuiViewer {
public:
    static void run(Position& pos, std::mutex& pos_mutex);
};
