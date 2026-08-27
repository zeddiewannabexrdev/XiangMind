#include "gui_viewer.h"

#define Color rlColor
#include <raylib.h>
#undef Color
#undef WHITE
#undef BLACK

#include <string>
#include <vector>

void GuiViewer::run(Position& shared_pos, std::mutex& pos_mutex) {
    const int square_size = 60;
    const int margin_x = 40;
    const int margin_y = 40;
    const int screen_width = margin_x * 2 + 8 * square_size;
    const int screen_height = margin_y * 2 + 9 * square_size;

    InitWindow(screen_width, screen_height, "Xiangqi Zeddie Engine - Board Viewer");
    SetTargetFPS(60);

    const char* piece_names[15] = {
        "P", "A", "E", "H", "C", "R", "G",
        "",
        "p", "a", "e", "h", "c", "r", "g"
    };
    
    rlColor rl_black = { 0, 0, 0, 255 };
    rlColor rl_raywhite = { 245, 245, 245, 255 };
    rlColor rl_red = { 230, 41, 55, 255 };

    while (!WindowShouldClose()) {
        Position pos;
        {
            std::lock_guard<std::mutex> lock(pos_mutex);
            pos = shared_pos;
        }

        BeginDrawing();
        ClearBackground(rl_raywhite);

        for (int r = 0; r < 10; ++r) {
            int y = margin_y + (9 - r) * square_size;
            DrawLine(margin_x, y, margin_x + 8 * square_size, y, rl_black);
        }
        for (int f = 0; f < 9; ++f) {
            int x = margin_x + f * square_size;
            DrawLine(x, margin_y, x, margin_y + 4 * square_size, rl_black);
            DrawLine(x, margin_y + 5 * square_size, x, margin_y + 9 * square_size, rl_black);
        }
        DrawLine(margin_x, margin_y + 4 * square_size, margin_x, margin_y + 5 * square_size, rl_black);
        DrawLine(margin_x + 8 * square_size, margin_y + 4 * square_size, margin_x + 8 * square_size, margin_y + 5 * square_size, rl_black);
        
        DrawLine(margin_x + 3 * square_size, margin_y + 9 * square_size, margin_x + 5 * square_size, margin_y + 7 * square_size, rl_black);
        DrawLine(margin_x + 5 * square_size, margin_y + 9 * square_size, margin_x + 3 * square_size, margin_y + 7 * square_size, rl_black);
        DrawLine(margin_x + 3 * square_size, margin_y + 0 * square_size, margin_x + 5 * square_size, margin_y + 2 * square_size, rl_black);
        DrawLine(margin_x + 5 * square_size, margin_y + 0 * square_size, margin_x + 3 * square_size, margin_y + 2 * square_size, rl_black);

        for (int r = 0; r < 10; ++r) {
            for (int f = 0; f < 9; ++f) {
                Square sq = create_square((File)f, (Rank)r);
                Piece pc = pos.piece_on(sq);
                if (pc != NO_PIECE) {
                    int x = margin_x + f * square_size;
                    int y = margin_y + (9 - r) * square_size;
                    
                    rlColor piece_color = color_of(pc) == WHITE ? rl_red : rl_black;
                    
                    DrawCircle(x, y, 25, rl_raywhite);
                    DrawCircleLines(x, y, 25, piece_color);
                    DrawCircleLines(x, y, 24, piece_color);
                    
                    const char* txt = piece_names[pc];
                    int text_width = MeasureText(txt, 20);
                    DrawText(txt, x - text_width/2, y - 10, 20, piece_color);
                }
            }
        }

        EndDrawing();
    }

    CloseWindow();
    std::exit(0);
}
